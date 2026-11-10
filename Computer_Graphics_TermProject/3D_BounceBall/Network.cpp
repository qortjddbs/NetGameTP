#include "Network.h"
#include "Protocol.h"
#include "GameWorld.h"
#include <stdio.h>

// 게임이 시작될 때 이 포인터에 게임 객체를 연결
// Network.cpp에서도 같은 객체를 사용해야 내 캐릭터의 현재 위치를 읽을 수 있음
extern GameWorld* gameWorld;

SOCKET g_sock = INVALID_SOCKET;
HANDLE g_hRecvThread = NULL;

int    g_myId = -1;
HANDLE g_hLoginEvent = NULL;

char g_playerName[MAX_PLAYER][NAME_LEN] = {};
bool g_playerReady[MAX_PLAYER] = { false, };
bool g_playerConnected[MAX_PLAYER] = { false, };

bool ConnectServer(const char* ip)
{
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return false;

    g_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (g_sock == INVALID_SOCKET) { WSACleanup(); return false; }

    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, ip, &addr.sin_addr);

    if (connect(g_sock, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        printf("Failed to connect to server (%d)\n", WSAGetLastError());
        closesocket(g_sock); g_sock = INVALID_SOCKET;
        WSACleanup();
        return false;
    }
    printf("Connected to server: %s:%d\n", ip, SERVER_PORT);

    g_hLoginEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    g_hRecvThread = CreateThread(NULL, 0, ClientRecvThread, NULL, 0, NULL);
    return true;
}

// 헤더(3byte) → 본문 순서로 패킷 1개를 정확히 받음
bool RecvPacket(char* buf)
{
    // 1) 헤더
    int r = recv(g_sock, buf, sizeof(PacketHeader), MSG_WAITALL);
    if (r != sizeof(PacketHeader)) return false;          // 0: 정상 종료, 그 외: 오류

    PacketHeader* header = (PacketHeader*)buf;
    if (header->size < sizeof(PacketHeader) || header->size > RECV_BUF_SIZE) {
        printf("Invalid packet size: %d\n", header->size);
        return false;
    }

    // 2) 본문 (본문 없는 패킷이면 건너뜀)
    int bodySize = header->size - sizeof(PacketHeader);
    if (bodySize > 0) {
        r = recv(g_sock, buf + sizeof(PacketHeader), bodySize, MSG_WAITALL);
        if (r != bodySize) return false;
    }
    return true;
}

DWORD WINAPI ClientRecvThread(LPVOID arg)
{
    char buf[RECV_BUF_SIZE];

    while (true) {
        if (!RecvPacket(buf)) {
            printf("Disconnected from server\n");
            break;
        }
        ProcessServerPacket(buf);
    }
    return 0;
}

void ProcessServerPacket(char* buf)
{
    PacketHeader* header = (PacketHeader*)buf;

    switch (header->type) {
    case SC_LOGIN_OK: {
        SC_LOGIN_OK_PACKET* p = (SC_LOGIN_OK_PACKET*)buf;
        g_myId = p->id;
        printf("login ok: my id = %d\n", g_myId);
        SetEvent(g_hLoginEvent);
        break;
    }
    case SC_PLAYER_INFO: {
        SC_PLAYER_INFO_PACKET* p = (SC_PLAYER_INFO_PACKET*)buf;
        if (p->id < 0 || p->id >= MAX_PLAYER) break;
        strncpy_s(g_playerName[(int)p->id], NAME_LEN, p->name, _TRUNCATE);
        g_playerConnected[(int)p->id] = true;
        g_playerReady[(int)p->id] = false;          // 새로 들어온 사람은 준비 전
        printf("[RECV] player info: id=%d name=%s\n", p->id, p->name);
        break;
    }
    case SC_READY: {
        SC_READY_PACKET* p = (SC_READY_PACKET*)buf;
        if (p->id < 0 || p->id >= MAX_PLAYER) break;
        g_playerReady[(int)p->id] = p->isReady;
        printf("[RECV] ready: id=%d %s\n", p->id, p->isReady ? "ON" : "OFF");
        break;
    }
    default:
        printf("[RECV] type=%d size=%d\n", header->type, header->size);
        break;
    }
}

void SendLogin(const char* name)
{
    CS_LOGIN_PACKET p;
    memset(&p, 0, sizeof(p));
    p.header.size = sizeof(p);
    p.header.type = CS_LOGIN;
    strncpy_s(p.name, NAME_LEN, name, _TRUNCATE);
    send(g_sock, (char*)&p, sizeof(p), 0);
}

void SendReady(bool isReady)
{
    CS_READY_PACKET p;
    p.header.size = sizeof(p);
    p.header.type = CS_READY;
    p.isReady = isReady;
    send(g_sock, (char*)&p, sizeof(p), 0);
}

// 좌표 읽기 -> 패킷 작성 -> 서버에 전송 -> 다음 호출 예약
void SendMove(int value)
{
    // 게임 객체 존재 서버에 접속 중 플레이 중일 때만 위치 전송
    if (gameWorld != nullptr &&
        g_sock != INVALID_SOCKET &&
        gameWorld->getGameState() == GameState::PLAYING)
    {

        const auto position = gameWorld->getPlayer()->getPosition();
       
        CS_MOVE_PACKET packet{};
        packet.header.size = static_cast<unsigned char>(sizeof(packet));
        packet.header.type = CS_MOVE;
        packet.x = position.x;
        packet.y = position.y;
        packet.z = position.z;

        const char* data = reinterpret_cast<const char*>(&packet);
        const int packetSize = static_cast<int>(sizeof(packet));
        int totalSent = 0;

        
        while (totalSent < packetSize) {
            int sent = send(
                g_sock,
                data + totalSent,
                packetSize - totalSent,
                0
            );

            if (sent == SOCKET_ERROR || sent == 0) {
                printf("[SendMove] 위치 전송 실패\n");

                // 불완전한 패킷 전송 시 통신 중단
                shutdown(g_sock, SD_BOTH);
                return;
            }

            totalSent += sent;
        }
    }

    // 다음 호출 예약
    glutTimerFunc(33, SendMove, 0);
}