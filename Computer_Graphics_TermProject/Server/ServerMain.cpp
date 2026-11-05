#include "Common.h"
#include "Protocol.h"
#include "ServerData.h"

Player g_players[MAX_PLAYER];
CRITICAL_SECTION g_csPlayers;

// 패킷 전송에 필요한 플레이어 정보
struct ServerPlayer {
	SOCKET socket = INVALID_SOCKET; // 연결된 소켓
	bool connected = false;        // 접속 여부
};

ServerPlayer g_players[MAX_PLAYER];
CRITICAL_SECTION g_csPlayers;          // 플레이어 정보 보호
CRITICAL_SECTION g_csSend[MAX_PLAYER]; // 같은 플레이어에게 보내는 패킷이 섞이지 않도록 보호

// 한 명에게 전송
void SendPacket(int id, void* packet, int size)
{
	// 잘못된 대상이나 패킷은 보내지 않음 현재 헤더 크기 필드는 1바이트
	if (id < 0 || id >= MAX_PLAYER || packet == nullptr ||
		size < static_cast<int>(sizeof(PacketHeader)) || size > 255) {
		return;
	}

	const PacketHeader* header = static_cast<const PacketHeader*>(packet);
	if (header->size != size) return;

	// 패킷 하나를 다 보낼 때까지 다른 스레드의 전송을 막음 임계영역
	EnterCriticalSection(&g_csSend[id]);

	// 접속 정보 복사
	EnterCriticalSection(&g_csPlayers);
	SOCKET clientSocket = g_players[id].socket;
	bool connected = g_players[id].connected;
	LeaveCriticalSection(&g_csPlayers);

	if (connected && clientSocket != INVALID_SOCKET) {
		const char* data = static_cast<const char*>(packet);
		int totalSent = 0;

		// 한 번에 전부 보내지 못하면 남은 데이터를 이어서 전송
		while (totalSent < size) {
			int sent = send(clientSocket, data + totalSent, size - totalSent, 0);
			if (sent == SOCKET_ERROR || sent == 0) {
				if (sent == SOCKET_ERROR) {
					printf("[Server] send failed: id=%d, error=%d\n",
						id, WSAGetLastError());
				}

				// 전송 실패 시 통신을 중단
				shutdown(clientSocket, SD_BOTH);
				break;
			}

			totalSent += sent;
		}
	}

	LeaveCriticalSection(&g_csSend[id]);
}

// 접속 중인 모든 플레이어에게 전송합니다.
void SendPacketToAll(void* packet, int size)
{
	// 접속 여부는 SendPacket 안에서 확인합니다.
	for (int id = 0; id < MAX_PLAYER; ++id) {
		SendPacket(id, packet, size);
	}
}

int main(int argc, char* argv[])
{
	int retval;

	// 윈속 초기화
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;

	InitializeCriticalSection(&g_csPlayers);

	// 소켓 생성
	SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sock == INVALID_SOCKET) err_quit("socket()");

	// bind()
	struct sockaddr_in serveraddr;
	memset(&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(SERVER_PORT);
	retval = bind(listen_sock, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR) err_quit("bind()");

	// listen()
	retval = listen(listen_sock, SOMAXCONN);
	if (retval == SOCKET_ERROR) err_quit("listen()");

	// 임계영역 초기화
	InitializeCriticalSection(&g_csPlayers);
	for (int id = 0; id < MAX_PLAYER; ++id) {
		InitializeCriticalSection(&g_csSend[id]);
	}

	printf("[서버] %d번 포트에서 대기 중...\n", SERVER_PORT);

	// 데이터 통신에 사용할 변수
	SOCKET client_sock;
	struct sockaddr_in clientaddr;
	int addrlen;
	while (1) {
		// accept()
		addrlen = sizeof(clientaddr);
		client_sock = accept(listen_sock, (struct sockaddr*)&clientaddr, &addrlen);
		if (client_sock == INVALID_SOCKET) {
			err_display("accept()");
			continue;
		}

		// 접속한 클라이언트 정보 출력
		char addr[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &clientaddr.sin_addr, addr, sizeof(addr));
		printf("\n[TCP 서버] 클라이언트 접속: IP 주소=%s, 포트 번호=%d\n",
			addr, ntohs(clientaddr.sin_port));

		// 나중에 쓰레드 생성으로 바꿈
		closesocket(client_sock);// 추후에 접속·수신 스레드 구현 때 연결
	}

	// 정리
	// 모든 스레드가 종료된 뒤 잠금 객체를 정리합니다.
	for (int id = 0; id < MAX_PLAYER; ++id) {
		DeleteCriticalSection(&g_csSend[id]);
	}
	DeleteCriticalSection(&g_csPlayers);
	closesocket(listen_sock);
	WSACleanup();
	DeleteCriticalSection(&g_csPlayers);
	return 0;
}
