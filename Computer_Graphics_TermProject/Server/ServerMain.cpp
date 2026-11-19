#include "Common.h"
#include "Protocol.h"
#include "ServerData.h"
#include "MapData.h"
#include <cmath>

Player g_players[MAX_PLAYER];
CRITICAL_SECTION g_csPlayers;
CRITICAL_SECTION g_csSend[MAX_PLAYER]; // 같은 플레이어에게 보내는 패킷이 섞이지 않도록 보호
HANDLE g_hStartEvent;

enum class ServerState { LOBBY, PLAYING, END };
ServerState g_gameState = ServerState::LOBBY;
CRITICAL_SECTION g_csGame;

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

	// 패킷 하나를 다 보낼 때까지 다른 스레드의 전송을 막음 (임계영역)
	EnterCriticalSection(&g_csSend[id]);

	// 접속 정보 복사
	EnterCriticalSection(&g_csPlayers);
	SOCKET clientSocket = g_players[id].m_sock;
	bool connected = g_players[id].m_connect;
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
	// 접속 여부는 SendPacket 안에서 확인
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

// 로그인 처리: 닉네임 저장 → SC_LOGIN_OK(요청자) → 기존 접속자 정보(요청자) → 새 플레이어 정보(전원)
void OnLogin(int id, CS_LOGIN_PACKET* p)
{
	// 1) 닉네임 저장
	EnterCriticalSection(&g_csPlayers);
	strncpy_s(g_players[id].m_name, NAME_LEN, p->name, _TRUNCATE);
	LeaveCriticalSection(&g_csPlayers);
	printf("[%d] 로그인: %s\n", id, p->name);

	// 2) 요청자에게 id 알려주기
	SC_LOGIN_OK_PACKET ok;
	ok.size = sizeof(ok);
	ok.type = SC_LOGIN_OK;
	ok.id = (char)id;
	SendPacket(id, &ok, sizeof(ok));

	// 3) 이미 접속해 있던 사람들 정보를 새 플레이어에게 (락 안에서 복사만)
	SC_PLAYER_INFO_PACKET others[MAX_PLAYER];
	bool othersReady[MAX_PLAYER] = {};
	int count = 0;
	EnterCriticalSection(&g_csPlayers);
	for (int i = 0; i < MAX_PLAYER; ++i) {
		if (i == id || !g_players[i].m_connect) continue;
		others[count].size = sizeof(SC_PLAYER_INFO_PACKET);
		others[count].type = SC_PLAYER_INFO;
		others[count].id = (char)i;
		strncpy_s(others[count].name, NAME_LEN, g_players[i].m_name, _TRUNCATE);
		othersReady[count] = g_players[i].m_ready;
		++count;
	}
	LeaveCriticalSection(&g_csPlayers);
	for (int i = 0; i < count; ++i) {
		SendPacket(id, &others[i], sizeof(others[i]));
		if (othersReady[i]) {
			SC_READY_PACKET rp;
			rp.size = sizeof(rp);
			rp.type = SC_READY;
			rp.id = others[i].id;
			rp.isReady = true;
			SendPacket(id, &rp, sizeof(rp));
		}
	}

	// 4) 새 플레이어 정보를 전원에게
	SC_PLAYER_INFO_PACKET info;
	info.size = sizeof(info);
	info.type = SC_PLAYER_INFO;
	info.id = (char)id;
	strncpy_s(info.name, NAME_LEN, p->name, _TRUNCATE);
	SendPacketToAll(&info, sizeof(info));
}

// 별 획득 체크만 하는 함수
bool CheckStar(int id, int starId)
{
	
	if (id < 0 || id >= MAX_PLAYER) return false;
	if (starId < 0 || starId >= static_cast<int>(STAR_COUNT)) return false;

	// 검사할 별에 해당하는 비트
	const unsigned int starBit = 1u << starId;

	EnterCriticalSection(&g_csPlayers);

	const Player& player = g_players[id];

	const bool connected = player.m_connect;
	const bool alreadyCollected = (player.m_starMask & starBit) != 0;

	// 서버에 저장된 플레이어 위치와 별 위치의 차이
	const float dx = player.m_x - STAR_POS[starId][0];
	const float dy = player.m_y - STAR_POS[starId][1];
	const float dz = player.m_z - STAR_POS[starId][2];

	const float distanceSquared = dx * dx + dy * dy + dz * dz;

	LeaveCriticalSection(&g_csPlayers);

	if (!connected || alreadyCollected) return false;

	// 제곱 거리를 비교하므로 제곱근 계산이 필요 없음
	return std::isfinite(distanceSquared) && distanceSquared <= STAR_RADIUS_CHECK * STAR_RADIUS_CHECK;
}

// 아이템 획득 가능 여부만 검사
bool CheckItem(int id, int itemId)
{
	if (id < 0 || id >= MAX_PLAYER)
		return false;

	if (itemId < 0 ||
		itemId >= static_cast<int>(ITEM_COUNT))
		return false;

	const unsigned int itemBit = 1u << itemId;

	EnterCriticalSection(&g_csPlayers);

	const Player& player = g_players[id];

	const bool connected = player.m_connect;
	const bool alreadyCollected =
		(player.m_itemMask & itemBit) != 0;

	const ItemData& item = ITEM_LIST[itemId];

	const float dx = player.m_x - item.x;
	const float dy = player.m_y - item.y;
	const float dz = player.m_z - item.z;

	const float distanceSquared =
		dx * dx + dy * dy + dz * dz;

	LeaveCriticalSection(&g_csPlayers);

	if (!connected || alreadyCollected)
		return false;

	return std::isfinite(distanceSquared) &&
		distanceSquared <=
		ITEM_RADIUS_CHECK * ITEM_RADIUS_CHECK;
}

// 아이템 획득 요청 처리
void OnItemGet(int id, CS_ITEM_GET_PACKET* packet)
{
	if (id < 0 || id >= MAX_PLAYER || packet == nullptr)
		return;

	if (packet->type != CS_ITEM_GET ||
		packet->size != sizeof(CS_ITEM_GET_PACKET))
		return;

	const int itemId = packet->itemId;

	SC_ITEM_RESULT_PACKET result{};
	bool success = false;

	// 검사와 획득 기록을 같은 잠금 안에서 처리
	EnterCriticalSection(&g_csPlayers);

	if (CheckItem(id, itemId)) {
		g_players[id].m_itemMask |= (1u << itemId);

		result.size =
			static_cast<unsigned char>(sizeof(result));
		result.type = SC_ITEM_RESULT;
		result.id = static_cast<char>(id);
		result.itemId = static_cast<char>(itemId);
		result.itemType =
			static_cast<char>(ITEM_LIST[itemId].type);

		success = true;
	}

	LeaveCriticalSection(&g_csPlayers);

	if (!success) {
		printf("[OnItemGet] 거절: id=%d, itemId=%d\n",
			id, itemId);
		return;
	}

	printf("[OnItemGet] 성공: id=%d, itemId=%d, itemType=%d\n",
		id, itemId, static_cast<int>(result.itemType));

	// 패킷 전송은 잠금 밖에서 실행
	SendPacketToAll(&result, sizeof(result));
}

// 별 획득 요청 처리 및 결과 전송
void OnStarGet(int id, CS_STAR_GET_PACKET* packet)
{
	if (id < 0 || id >= MAX_PLAYER || packet == nullptr) return;
	if (packet->type != CS_STAR_GET || packet->size != sizeof(CS_STAR_GET_PACKET)) return;

	const int starId = packet->starId;

	SC_STAR_RESULT_PACKET result{};
	result.size = static_cast<unsigned char>(sizeof(result));
	result.type = SC_STAR_RESULT;
	result.id = static_cast<char>(id);
	result.starId = packet->starId;

	// 검사부터 기록 과정을 임계영역으로 보호
	EnterCriticalSection(&g_csPlayers);

	if (!g_players[id].m_connect) {
		LeaveCriticalSection(&g_csPlayers);
		return;
	}

	result.success = CheckStar(id, starId);

	if (result.success) {
		// 별 획득 기록
		g_players[id].m_starMask |= (1u << starId);

		// 별 개수와 마지막 획득 시각 기록
		++g_players[id].m_starCount;
		g_players[id].m_lastStarTime = GetTickCount();
	}

	// 실패했을 때도 현재 서버의 별 개수를 전달
	result.starCount = static_cast<char>(g_players[id].m_starCount);

	LeaveCriticalSection(&g_csPlayers);

	printf("[OnStarGet] id=%d, starId=%d, success=%d, count=%d\n", id, starId, static_cast<int>(result.success), static_cast<int>(result.starCount));

	SendPacketToAll(&result, sizeof(result));
}

// 클라이언트가 보낸 위치를 서버에 저장
void OnMove(int id, CS_MOVE_PACKET* packet)
{
	// 잘못된 id나 패킷은 리턴
	if (id < 0 || id >= MAX_PLAYER || packet == nullptr) return;
	if (packet->type != CS_MOVE || packet->size < sizeof(CS_MOVE_PACKET)) return;

	EnterCriticalSection(&g_csPlayers);

	// 공유 데이터 임계영역으로 보호 및 접속 중인 플레이어만 위치 갱신
	if (g_players[id].m_connect) {
		g_players[id].m_x = packet->x;
		g_players[id].m_y = packet->y;
		g_players[id].m_z = packet->z;
	}

	// 좌표 출력
	// printf("[player %d], x=%.2f, y=%.2f, z=%.2f\n", id, packet->x, packet->y, packet->z);

	LeaveCriticalSection(&g_csPlayers);
}

// 플레이어 준비 처리
void OnReady(int id, CS_READY_PACKET* packet)
{
	// 잘못된 id나 패킷은 리턴
	if (id < 0 || id >= MAX_PLAYER || packet == nullptr) return;
	if (packet->type != CS_READY || packet->size != sizeof(CS_READY_PACKET)) return;

	int readyPlayers = 0;
	bool inLobby = false;
	bool startNow = false;

	EnterCriticalSection(&g_csPlayers);
	EnterCriticalSection(&g_csGame);

	if (g_gameState == ServerState::LOBBY) inLobby = true;

	if (inLobby) {
		// 공유 데이터 임계영역으로 보호 및 접속 중인 플레이어만 준비 상태 갱신
		if (g_players[id].m_connect) {
			g_players[id].m_ready = packet->isReady;
		}

		for (int i = 0; i < MAX_PLAYER; ++i) {
			if (g_players[i].m_connect && g_players[i].m_ready) readyPlayers++;
		}

		if (readyPlayers == MAX_PLAYER) {
			g_gameState = ServerState::PLAYING;
			startNow = true;
		}
	}

	LeaveCriticalSection(&g_csGame);
	LeaveCriticalSection(&g_csPlayers);

	if (!inLobby) return;

	// 준비 상태 출력 (준비 인원 n/3)
	printf("[player %d], Ready - %s (%d/%d)\n", id, packet->isReady ? "ON" : "OFF", readyPlayers, MAX_PLAYER);

	SC_READY_PACKET rp;
	rp.size = sizeof(rp);
	rp.type = SC_READY;
	rp.id = (char)id;
	rp.isReady = packet->isReady;
	SendPacketToAll(&rp, sizeof(rp));

	if (startNow) {
		printf("3명 모두 준비 -> 시작 신호 on\n");
		SetEvent(g_hStartEvent);
	}
}

// 패킷 type별 분기
void ProcessPacket(int id, char* buf)
{
	PacketHeader* header = (PacketHeader*)buf;

	switch (header->type) {
	case CS_LOGIN:
		OnLogin(id, (CS_LOGIN_PACKET*)buf);
		break;
	case CS_READY:
		OnReady(id, (CS_READY_PACKET*)buf);
		break;
	case CS_MOVE:
		OnMove(id, (CS_MOVE_PACKET*)buf);
		break;
	case CS_STAR_GET:
		OnStarGet(id, (CS_STAR_GET_PACKET*)buf);
		break;
	case CS_ITEM_GET:
		OnItemGet(id, (CS_ITEM_GET_PACKET*)buf);
		break;
	default:
		printf("[%d] 알 수 없는 패킷 type=%d\n", id, header->type);
		break;
	}
}

// 클라이언트 id할당 후 리턴. 빈자리가 없으면 -1
int AssignPlayerId(SOCKET sock)
{
	int id = -1;

	EnterCriticalSection(&g_csPlayers);
	EnterCriticalSection(&g_csGame);

	if (g_gameState == ServerState::LOBBY) {
		for (int i = 0; i < MAX_PLAYER; ++i) {
			if (false == g_players[i].m_connect) {
				memset(&g_players[i], 0, sizeof(Player));		// 이전 사람 정보 지우기
				g_players[i].m_sock = sock;
				g_players[i].m_id = i;
				g_players[i].m_connect = true;
				id = i;
				break;
			}
		}
	}

	LeaveCriticalSection(&g_csGame);
	LeaveCriticalSection(&g_csPlayers);

	return id;
}

void Disconnect(int id) {
	SOCKET sock;

	// 두 개의 락을 같이 잡을 때 -> 항상 g_csSend 먼저 잡기
	// SendPacket이 보내는 도중에 소켓이 닫히지 않게 g_csSend까지 잡음
	EnterCriticalSection(&g_csSend[id]);
	EnterCriticalSection(&g_csPlayers);
	sock = g_players[id].m_sock;
	g_players[id].m_connect = false;
	g_players[id].m_sock = INVALID_SOCKET;
	LeaveCriticalSection(&g_csPlayers);
	closesocket(sock);
	LeaveCriticalSection(&g_csSend[id]);

	SC_LEAVE_PACKET lp;
	lp.type = SC_LEAVE;
	lp.size = sizeof(lp);
	lp.id = (char)id;
	SendPacketToAll(&lp, sizeof(lp));

	printf("[%d] 종료\n", id);

	// CheckGameEnd(): 남은 사람이 1명뿐이면 게임 종료 (11월 18일 구현 예정)
}

// 클라이언트 한 명 전용 쓰레드
DWORD WINAPI ProcessClient(LPVOID arg)
{
	int id = (int)(INT_PTR)arg;

	SOCKET sock;
	EnterCriticalSection(&g_csPlayers);
	sock = g_players[id].m_sock;
	LeaveCriticalSection(&g_csPlayers);

	printf("[쓰레드] Player %d 시작\n", id);

	char buf[256];		// 헤더 -> size가 1바이트 이므로 패킷은 최대 255바이트
	while (1) {
		if (!RecvPacket(sock, buf)) break;		// 끊기거나 오류면 종료
		PacketHeader* header = (PacketHeader*)buf;
		if (header->type != CS_MOVE)
			printf("[Player %d] 패킷 수신: type=%d, size=%d\n", id, header->type, header->size);
		ProcessPacket(id, buf);
	}

	Disconnect(id);
	printf("[쓰레드] Player %d 종료\n", id);
	return 0;
}

// 게임 로직 쓰레드
DWORD WINAPI LogicThread(LPVOID arg) {
	// 준비 완료 대기
	DWORD retval = WaitForSingleObject(g_hStartEvent, INFINITE);
	if (retval != WAIT_OBJECT_0) {
		printf("게임 로직 쓰레드 - 준비 완료 대기에서 문제 발생\n");
		return 1;
	}

	SC_GAME_START_PACKET gsp;
	gsp.size = sizeof(gsp);
	gsp.type = SC_GAME_START;
	gsp.countdown = 3;
	SendPacketToAll(&gsp, sizeof(gsp));

	// 게임 로직 쓰레드도 3초 기다리기
	Sleep(gsp.countdown * 1000);

	printf("게임 시작!!\n");

	int tick = 0;

	while (true) {
		SC_GHOST_PACKET gp{};

		EnterCriticalSection(&g_csPlayers);
		for (int i = 0; i < MAX_PLAYER; ++i) {
			if (g_players[i].m_connect) {
				gp.ghost[gp.count].id = (char)g_players[i].m_id;
				gp.ghost[gp.count].x = g_players[i].m_x;
				gp.ghost[gp.count].y = g_players[i].m_y;
				gp.ghost[gp.count].z = g_players[i].m_z;
				gp.count++;
			}
		}
		LeaveCriticalSection(&g_csPlayers);

		gp.type = SC_GHOST;
		gp.size = sizeof(gp);

		SendPacketToAll(&gp, sizeof(gp));

		if (++tick % 30 == 0) {
			printf("[GHOST] count=%d", gp.count);
			for (int i = 0; i < gp.count; ++i)
				printf(" | id%d (%.1f, %.1f, %.1f)", gp.ghost[i].id, gp.ghost[i].x, gp.ghost[i].y, gp.ghost[i].z);
			printf("\n");
		}

		bool end;
		EnterCriticalSection(&g_csGame);
		end = (g_gameState == ServerState::END);
		LeaveCriticalSection(&g_csGame);
		if (end) break;

		Sleep(33);
	}

	return 0;
}

int main(int argc, char* argv[])
{
	int retval;

	// 윈속 초기화
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;

	// 임계영역 초기화
	InitializeCriticalSection(&g_csPlayers);
	InitializeCriticalSection(&g_csGame);

	// 이벤트 초기화 (수동리셋, 비신호)
	g_hStartEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (NULL == g_hStartEvent)
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

	// 플레이어 각각의 send 임계영역 초기화
	for (int id = 0; id < MAX_PLAYER; ++id) {
		InitializeCriticalSection(&g_csSend[id]);
	}

	// LogicThread 생성 및 실행
	HANDLE hThread = CreateThread(NULL, 0, LogicThread, NULL, 0, NULL);
	if (NULL == hThread) {
		printf("LogicThread 생성 실패\n");
		return 1;
	}
	else {
		CloseHandle(hThread);
	}

	printf("[서버] %d번 포트에서 대기 중...\n", SERVER_PORT);

	// 데이터 통신에 사용할 변수
	SOCKET client_sock;
	struct sockaddr_in clientaddr;
	int addrlen;

	// accept()
	while (1) {
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

		int id = AssignPlayerId(client_sock);
		if (id == -1) {
			printf("[서버] 자리 없음 또는 게임 진행 중, 접속 거부\n");
			closesocket(client_sock);
			continue;
		}

		hThread = CreateThread(NULL, 0, ProcessClient, (LPVOID)(INT_PTR)id, 0, NULL);
		if (NULL == hThread) {
			EnterCriticalSection(&g_csPlayers);
			g_players[id].m_connect = false;
			LeaveCriticalSection(&g_csPlayers);
			closesocket(client_sock);
		}
		else {
			CloseHandle(hThread);
		}
	}

	// 정리
	// 모든 스레드가 종료된 뒤 잠금 객체를 정리합니다.
	for (int id = 0; id < MAX_PLAYER; ++id) {
		DeleteCriticalSection(&g_csSend[id]);
	}
	closesocket(listen_sock);
	WSACleanup();
	CloseHandle(g_hStartEvent);
	DeleteCriticalSection(&g_csGame);
	DeleteCriticalSection(&g_csPlayers);
	return 0;
}
