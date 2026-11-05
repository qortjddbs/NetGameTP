#pragma once
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32")

extern SOCKET g_sock;
extern HANDLE g_hRecvThread;

bool ConnectServer(const char* ip);
DWORD WINAPI ClientRecvThread(LPVOID arg);

#define RECV_BUF_SIZE 256

bool RecvPacket(char* buf);
void ProcessServerPacket(char* buf);