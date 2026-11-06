#include "Common.h"
#include "Protocol.h"

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

// 접속 중인 모든 플레이어에게 전송
void SendPacketToAll(void* packet, int size)
{
	// 접속 여부는 SendPacket 안에서 확인.
	for (int id = 0; id < MAX_PLAYER; ++id) {
		SendPacket(id, packet, size);
	}
}

// 패킷 하나를 끝까지 받으면 true, 실패하면 false를 반환
// sock: 데이터를 받을 소켓
// buf: 받은 데이터를 저장할 버퍼
bool RecvPacket(SOCKET sock, char* buf)
{
	if (buf == nullptr) return false;

	// PacketHeader의 크기를 int로 저장. 현재 프로젝트에서는 2바이트
	const int headerSize = static_cast<int>(sizeof(PacketHeader));

	// 지금까지 받은 바이트 수. 아직 아무것도 안 받았으므로 0
	int totalReceived = 0;

	// 헤더 크기만큼 받을 때까지 반복
	while (totalReceived < headerSize) {

		// recv()가 실제로 받은 바이트 수를 received에 저장
		int received = recv(
			sock,                         // 이 소켓에서 받음
			buf + totalReceived,          // 이미 받은 데이터 다음 위치에 저장
			headerSize - totalReceived,   // 헤더에서 아직 못 받은 길이만큼 요청
			0                             // 기본 수신 옵션
		);

		// 0이면 상대가 연결 종료, SOCKET_ERROR이면 수신 오류
		if (received == 0 || received == SOCKET_ERROR)
			return false;

		// 이번에 받은 바이트 수를 누적
		totalReceived += received;
	}
	// 여기까지 왔다면 buf에 헤더 2바이트가 들어 있음

	// 헤더 정보를 담을 변수를 만들고 0으로 초기화
	PacketHeader header{};

	// buf에 받은 헤더 데이터를 header 변수로 복사
	// 이제 header.size, header.type으로 값을 읽을 수 있음
	memcpy(&header, buf, sizeof(header));

	// 전체 패킷 크기가 헤더 크기보다 작으면 잘못된 패킷
	if (header.size < headerSize)
		return false;

	// 헤더에 적힌 전체 패킷 크기를 저장. 헤더 + 본문 크기임
	const int packetSize = header.size;

	// 받은 양이 전체 패킷 크기가 될 때까지 반복
	// totalReceived에는 이미 받은 헤더 크기인 2가 들어 있음
	while (totalReceived < packetSize) {
		int received = recv(
			sock,                         // 같은 소켓에서 받음
			buf + totalReceived,          // 이미 받은 데이터 바로 뒤에 저장
			packetSize - totalReceived,   // 전체 패킷 중 아직 못 받은 길이만큼 요청
			0                             // 기본 수신 옵션
		);

		// 연결 종료 또는 수신 오류면 실패
		if (received == 0 || received == SOCKET_ERROR)
			return false;

		// 이번에 받은 바이트 수를 누적
		totalReceived += received;
	}

	return true;
}

int main(int argc, char* argv[])
{
	int retval;

	// 윈속 초기화
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;

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
	return 0;
}
