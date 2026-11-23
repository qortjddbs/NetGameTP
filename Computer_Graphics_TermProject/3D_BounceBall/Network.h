#pragma once
#include "Protocol.h"
#include <queue>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32")

#define RECV_BUF_SIZE 256

extern SOCKET g_sock;
extern HANDLE g_hRecvThread;

extern int    g_myId;
extern HANDLE g_hLoginEvent;

extern char g_playerName[MAX_PLAYER][NAME_LEN];
extern bool g_playerReady[MAX_PLAYER];
extern bool g_playerConnected[MAX_PLAYER];

extern CRITICAL_SECTION g_csNet;      // 수신 스레드 <-> 메인 스레드 공유 데이터 보호
extern int g_startCountdown;          // SC_GAME_START 수신 값 (-1이면 X)

// SC_GHOST로 받은 다른 플레이어 위치 (수신 스레드가 쓰고, 메인 스레드가 g_csNet 안에서 읽음)
struct GhostNetData {
    bool  active;        // 위치를 한 번이라도 받았는가
    bool  updated;       // 메인이 아직 안 읽은 새 위치가 있는가
    float x, y, z;       // 가장 최근 위치
};
extern GhostNetData g_ghostNet[MAX_PLAYER];

#define NET_EVENT_SIZE 64               // 가장 큰 이벤트 패킷이 들어갈 크기

// 수신 스레드 → 메인 스레드로 넘기는 이벤트 (패킷 전체를 복사해 둠)
struct NetEvent {
    unsigned char type;                 // 패킷 종류 (switch용)
    char data[NET_EVENT_SIZE];          // 헤더 포함 패킷 원본
};

void PopNetEvents(std::queue<NetEvent>& out);   // 메인 스레드: 쌓인 이벤트를 한 번에 꺼냄
void InitNet();                       // g_csNet 초기화 (main 시작 시 1번 호출)

bool ConnectServer(const char* ip);
void CloseNet();
DWORD WINAPI ClientRecvThread(LPVOID arg);
bool RecvPacket(char* buf);
void ProcessServerPacket(char* buf);
void SendLogin(const char* name);
void SendReady(bool isReady);
void SendMove(int value);
void SendStarGet(int starId);
void SendItemGet(int itemId);