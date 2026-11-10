#pragma once

// ===== 공용 상수 =====
#define SERVER_PORT   9000
#define MAX_PLAYER    3
#define NAME_LEN      16

// ===== 패킷 종류 =====
enum PACKET_TYPE : unsigned char {
    // Client -> Server
    CS_LOGIN = 1,
    CS_READY,
    CS_MOVE,
    CS_STAR_GET,
    CS_ITEM_GET,
    // Server -> Client
    SC_LOGIN_OK,
    SC_PLAYER_INFO,
    SC_READY,
    SC_GAME_START,
    SC_GHOST,
    SC_STAR_RESULT,
    SC_ITEM_RESULT,
    SC_GAME_END,
    SC_LEAVE
};

#pragma pack(push, 1)

struct PacketHeader {
    unsigned char size;
    unsigned char type;
};

// ===== Client -> Server =====
struct CS_LOGIN_PACKET : PacketHeader {
    char name[NAME_LEN];
};
struct CS_READY_PACKET : PacketHeader {
    bool isReady;
};
struct CS_MOVE_PACKET : PacketHeader {
    float x, y, z;
};
struct CS_STAR_GET_PACKET : PacketHeader {
    char starId;
};
struct CS_ITEM_GET_PACKET : PacketHeader {
    char itemId;
};

// ===== Server -> Client =====
struct SC_LOGIN_OK_PACKET : PacketHeader {
    char id;
};
struct SC_PLAYER_INFO_PACKET : PacketHeader {
    char id;
    char name[NAME_LEN];
};
struct SC_READY_PACKET : PacketHeader {
    char id;
    bool isReady;
};
struct SC_GAME_START_PACKET : PacketHeader {
    char countdown;
};
// SC_GHOST_PACKET 으로 보내는 Ghost Data
struct GhostData { 
    char id;
    float x, y, z;
};
struct SC_GHOST_PACKET : PacketHeader {
    char count;
    GhostData ghost[MAX_PLAYER];
};

struct SC_STAR_RESULT_PACKET : PacketHeader { 
    char id; 
    char starId; 
    char starCount; 
    bool success; 
};
struct SC_ITEM_RESULT_PACKET : PacketHeader { 
    char id; 
    char itemId; 
    char itemType; 
};
// SC_GAME_END_PACKET 으로 보내는 Rank Data
struct RankData {
    char id;
    char starCount;
};
struct SC_GAME_END_PACKET : PacketHeader {
    char count;
    RankData rank[MAX_PLAYER];
};
struct SC_LEAVE_PACKET : PacketHeader {
    char id;
};

#pragma pack(pop)