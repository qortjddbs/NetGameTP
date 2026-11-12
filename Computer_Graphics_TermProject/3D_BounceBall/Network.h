#pragma once
#include "Protocol.h"
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

void InitNet();                       // g_csNet 초기화 (main 시작 시 1번 호출)

bool ConnectServer(const char* ip);
DWORD WINAPI ClientRecvThread(LPVOID arg);
bool RecvPacket(char* buf);
void ProcessServerPacket(char* buf);
void SendLogin(const char* name);
void SendReady(bool isReady);
void SendMove(int value);