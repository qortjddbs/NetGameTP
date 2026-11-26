#include "Network.h"
#include "Protocol.h"
#include "MapData.h"
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

CRITICAL_SECTION g_csNet;
int g_startCountdown = -1;

GhostNetData g_ghostNet[MAX_PLAYER] = {};

static std::queue<NetEvent> g_eventQueue;

void InitNet()  // 필요한 초기화 작업이 생기면 여기에 추가
{
    InitializeCriticalSection(&g_csNet);
}

bool ConnectServer(const char* ip)
{
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("WSAStartup failed\n");
        return false;
    }

    g_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (g_sock == INVALID_SOCKET) {
        printf("socket() failed (%d)\n", WSAGetLastError());
        WSACleanup();
        return false;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, ip, &addr.sin_addr);

    if (connect(g_sock, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        printf("Failed to connect to server (%d)\n", WSAGetLastError());
        closesocket(g_sock);
        g_sock = INVALID_SOCKET;
        WSACleanup();
        return false;
    }
    printf("Connected to server: %s:%d\n", ip, SERVER_PORT);

    // 로그인 응답 대기용 이벤트 (자동 리셋)
    g_hLoginEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

    // 받기 스레드 시작
    g_hRecvThread = CreateThread(NULL, 0, ClientRecvThread, NULL, 0, NULL);
    if (g_hRecvThread == NULL) {
        printf("CreateThread failed\n");
        CloseNet();
        return false;
    }
    return true;
}

// 접속 정리 (로그인 실패 시 / 종료 시 사용)
void CloseNet()
{
    if (g_sock != INVALID_SOCKET) {
        closesocket(g_sock);              // 받기 스레드의 recv가 실패하면서 스레드 종료
        g_sock = INVALID_SOCKET;
    }
    if (g_hRecvThread) {
        WaitForSingleObject(g_hRecvThread, INFINITE);
        CloseHandle(g_hRecvThread);
        g_hRecvThread = NULL;
    }
    if (g_hLoginEvent) {
        CloseHandle(g_hLoginEvent);
        g_hLoginEvent = NULL;
    }
    WSACleanup();
}

// 헤더(2byte) -> 본문 순서로 패킷 1개를 정확히 받음
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

static void PushNetEvent(const char* buf)
{
    const PacketHeader* header = (const PacketHeader*)buf;
    if (header->size > NET_EVENT_SIZE) {    // 사이즈를 64로 넉넉잡아서 실행될 일은 없을 듯함, 만약 실행되면 패킷을 줄이거나 사이즈를 키울 것
        printf("NetEvent too large: type=%d size=%d\n", header->type, header->size);
        return;
    }

    NetEvent ev;
    ev.type = header->type;
    memcpy(ev.data, buf, header->size);         // 패킷 원본 복사 (buf는 다음 recv에 덮어써짐)

    EnterCriticalSection(&g_csNet);
    g_eventQueue.push(ev);
    LeaveCriticalSection(&g_csNet);
}

void PopNetEvents(std::queue<NetEvent>& out)
{
    EnterCriticalSection(&g_csNet);
	std::swap(out, g_eventQueue);               // 통째로 교환: 락을 아주 짧게만 잡음, 락을 잡은채로 pop을 반복하면 메인 스레드가 오래 기다리게 될수도 있음.
    LeaveCriticalSection(&g_csNet);
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
        EnterCriticalSection(&g_csNet);  // 추가
        strncpy_s(g_playerName[(int)p->id], NAME_LEN, p->name, _TRUNCATE);
        g_playerConnected[(int)p->id] = true;
        g_playerReady[(int)p->id] = false;          // 새로 들어온 사람은 준비 전
        LeaveCriticalSection(&g_csNet);  // 추가
        printf("[RECV] player info: id=%d name=%s\n", p->id, p->name);
        break;
    }
    case SC_READY: {
        SC_READY_PACKET* p = (SC_READY_PACKET*)buf;
        if (p->id < 0 || p->id >= MAX_PLAYER) {
            break;
        }
        EnterCriticalSection(&g_csNet);
        g_playerReady[(int)p->id] = p->isReady;
        LeaveCriticalSection(&g_csNet);
        printf("[RECV] ready: id=%d %s\n", p->id, p->isReady ? "ON" : "OFF");
        break;
    }
    case SC_GAME_START: {
        SC_GAME_START_PACKET* p = (SC_GAME_START_PACKET*)buf;
        EnterCriticalSection(&g_csNet);
        g_startCountdown = p->countdown;
        LeaveCriticalSection(&g_csNet);
        printf("[RECV] game start: countdown %d\n", p->countdown);
        break;
    }
    case SC_GHOST: {
        SC_GHOST_PACKET* p = (SC_GHOST_PACKET*)buf;
        int count = p->count;
        if (count < 0 || count > MAX_PLAYER) break;

        EnterCriticalSection(&g_csNet);
        for (int i = 0; i < count; ++i) {
            int id = p->ghost[i].id;
            if (id < 0 || id >= MAX_PLAYER) continue;
            if (id == g_myId) continue;              // 내 위치는 로컬 물리로 움직이므로 무시

            g_ghostNet[id].x = p->ghost[i].x;
            g_ghostNet[id].y = p->ghost[i].y;
            g_ghostNet[id].z = p->ghost[i].z;
            g_ghostNet[id].active = true;
            g_ghostNet[id].updated = true;
        }
        LeaveCriticalSection(&g_csNet);
        break;
    }
    case SC_STAR_RESULT: {
        PushNetEvent(buf);
        break;
    }
    case SC_ITEM_RESULT: {

        // 획득에 성공한 경우에만 패킷 전송
        if (header->size != sizeof(SC_ITEM_RESULT_PACKET)) break;

        SC_ITEM_RESULT_PACKET result{};
        memcpy(&result, buf, sizeof(result));

        const int id = result.id;
        const int itemId = result.itemId;
        const int itemType = result.itemType;

        if (id < 0 || id >= MAX_PLAYER) break;
        if (itemId < 0 || itemId >= static_cast<int>(ITEM_COUNT)) break;
        if (itemType < 0 || itemType >= static_cast<int>(ITEM_TYPE::COUNT)) break;

        printf("[ItemResult] id=%d, itemId=%d, itemType=%d\n",
            id, itemId, itemType);

        PushNetEvent(buf);
        break;
    }
    case SC_GAME_END: {
        if (header->size != sizeof(SC_GAME_END_PACKET)) break;
        PushNetEvent(buf);
        break;
    }
    case SC_LEAVE: {
        SC_LEAVE_PACKET* p = (SC_LEAVE_PACKET*)buf;
        int id = p->id;
        if (id < 0 || id >= MAX_PLAYER || id == g_myId) break;

        EnterCriticalSection(&g_csNet);
        g_ghostNet[id].active = false;    // 고스트 제거
        g_ghostNet[id].updated = false;
        g_playerConnected[id] = false;    // 로비 목록에서 제거
        g_playerReady[id] = false;
        LeaveCriticalSection(&g_csNet);

        printf("[LEAVE] player %d left\n", id);
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
    p.size = sizeof(p);
    p.type = CS_LOGIN;
    strncpy_s(p.name, NAME_LEN, name, _TRUNCATE);
    send(g_sock, (char*)&p, sizeof(p), 0);
}

void SendReady(bool isReady)
{
    CS_READY_PACKET p;
    p.size = sizeof(p);
    p.type = CS_READY;
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
        packet.size = static_cast<unsigned char>(sizeof(packet));
        packet.type = CS_MOVE;
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

void SendStarGet(int starId)
{
    if (g_sock == INVALID_SOCKET ||
        starId < 0 ||
        starId >= static_cast<int>(STAR_COUNT))
        return;

    CS_STAR_GET_PACKET packet{};
    packet.size = static_cast<unsigned char>(sizeof(packet));
    packet.type = CS_STAR_GET;
    packet.starId = static_cast<char>(starId);

    const char* data =
        reinterpret_cast<const char*>(&packet);

    const int size = static_cast<int>(sizeof(packet));
    int totalSent = 0;

    while (totalSent < size) {
        int sent = send(
            g_sock, data + totalSent, size - totalSent, 0);

        if (sent == SOCKET_ERROR || sent == 0) {
            printf("[SendStarGet] 전송 실패: starId=%d\n", starId);
            shutdown(g_sock, SD_BOTH);
            return;
        }

        totalSent += sent;
    }

    printf("[SendStarGet] 전송 완료: starId=%d\n", starId);
}

// 획득한 아이템 ID를 서버에 알림
void SendItemGet(int itemId)
{
    if (g_sock == INVALID_SOCKET ||
        itemId < 0 ||
        itemId >= static_cast<int>(ITEM_COUNT))
        return;

    CS_ITEM_GET_PACKET packet{};
    packet.size = static_cast<unsigned char>(sizeof(packet));
    packet.type = CS_ITEM_GET;
    packet.itemId = static_cast<char>(itemId);

    const char* data =
        reinterpret_cast<const char*>(&packet);

    const int size = static_cast<int>(sizeof(packet));
    int totalSent = 0;

    while (totalSent < size) {
        int sent = send(
            g_sock,
            data + totalSent,
            size - totalSent,
            0
        );

        if (sent == SOCKET_ERROR || sent == 0) {
            printf("[SendItemGet] 전송 실패: itemId=%d\n", itemId);
            shutdown(g_sock, SD_BOTH);
            return;
        }

        totalSent += sent;
    }

    printf("[SendItemGet] 전송 완료: itemId=%d\n", itemId);
}