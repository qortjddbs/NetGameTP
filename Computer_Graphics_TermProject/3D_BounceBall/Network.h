#pragma once
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32")

#define RECV_BUF_SIZE 256

extern SOCKET g_sock;
extern HANDLE g_hRecvThread;

extern int    g_myId;
extern HANDLE g_hLoginEvent;

bool ConnectServer(const char* ip);
DWORD WINAPI ClientRecvThread(LPVOID arg);
bool RecvPacket(char* buf);
void ProcessServerPacket(char* buf);
void SendLogin(const char* name);
void SendReady(bool isReady);
void SendMove(int value);