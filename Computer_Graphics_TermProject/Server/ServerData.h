#pragma once
#include "Common.h"
#include "Protocol.h"

// [Server] Player
struct Player {
	SOCKET m_sock;
	int m_id;
	char m_name[NAME_LEN];
	bool m_connect, m_ready;
	float m_x, m_y, m_z;
	unsigned int m_starMask;		// 별 획득 비트마스크 (같은 별 중복 획득 방지)
	unsigned int m_itemMask;		// 아이템 획득 비트마스크
	unsigned char m_starCount;
	DWORD m_lastStarTime;			// 동점 시 순위 판정용
};

extern Player g_players[MAX_PLAYER];
extern CRITICAL_SECTION g_csPlayers;			// g_players를 읽고 쓸 때는 항상 이 안에서
extern CRITICAL_SECTION g_csSend[MAX_PLAYER];