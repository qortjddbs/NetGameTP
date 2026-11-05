#include "Network.h"
#include "Protocol.h"
#include <stdio.h>

SOCKET g_sock = INVALID_SOCKET;
HANDLE g_hRecvThread = NULL;

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
        printf("서버 접속 실패 (%d)\n", WSAGetLastError());
        closesocket(g_sock); g_sock = INVALID_SOCKET;
        WSACleanup();
        return false;
    }
    printf("서버 접속 성공: %s:%d\n", ip, SERVER_PORT);

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
        printf("잘못된 패킷 크기: %d\n", header->size);
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
            printf("서버와 연결이 끊어졌습니다.\n");
            break;
        }
        ProcessServerPacket(buf);
    }
    return 0;
}

// 임시: 11/10에 type별 처리로 교체
void ProcessServerPacket(char* buf)
{
    PacketHeader* header = (PacketHeader*)buf;
    printf("[RECV] type=%d size=%d\n", header->type, header->size);
}