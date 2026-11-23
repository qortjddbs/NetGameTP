#pragma once
#include "Protocol.h"
#include "MapData.h"
#include "Headers.h"
#include "Player.h"
#include "Block.h"
#include <vector>
#include <array>
#include "PathTrace.h"

enum class GameState {
	TITLE,
	LOBBY,
	COUNTDOWN,
	PLAYING,
	ENDING
};

class GameWorld
{
public:
	GameWorld(GLuint shaderID);
	~GameWorld();

	void initialize();
	void cleanup();
	void reset(bool inputX = false);

	void update(float deltaTime);
	void draw();
	void drawUI();  // UI 그리기 추가
	void checkCollisions();
	void onStarResult(const SC_STAR_RESULT_PACKET& packet);

	void onItemResult(const SC_ITEM_RESULT_PACKET& packet);
	void startEffect(int type);
	void updateEffects(float deltaTime);

	PLAYER* getPlayer() { return &player_; }

	bool getThirdPersonView() const { return ThirdPersonView_; }
	void toggleThirdPersonView() { ThirdPersonView_ = !ThirdPersonView_; }

	void addBlock(BLOCK* block);
	void addBounceBlock(BOUNCE_BLOCK* block);
	void addBreakableBlock(BREAKABLE_BLOCK* block);
	void addSpikeBlock(SPIKE_BLOCK* block);
	void addArrowBlock(ARROW_BLOCK* block);
	void addStar(STAR* star);

	GameState getGameState() const { return gameState_; }
	void setGameState(GameState state) { gameState_ = state; }
	void startGame();

	int getScore() const { return score_; }
	void addScore(int points) { score_ += points; }
	void setGameStarted(bool started) { gameStarted_ = started; }
	bool isGameStarted() const { return gameStarted_; }

	void toggleTrajectory() { showTrajectory_ = !showTrajectory_; }
	bool isTrajectoryVisible() const { return showTrajectory_; }

	// 별 개수 관련
	int getCollectedStars() const { return collectedStars_; }
	int getTotalStars() const { return totalStars_; }

	// 부서진 블록 복구
	void resetBreakableBlocks();
	// 중력 비활성화 토글
	void toggleGravity() { activateGravity_ = !activateGravity_; }
	bool getGravityStatus() const { return activateGravity_; }

private:
	GLuint shaderProgramID_;

	PLAYER player_;

	// 고스트 (다른 플레이어)
	std::array<ParentModel, MAX_PLAYER> ghosts_;		// 고스트 공 모델 (색은 id마다 다르게)
	std::array<glm::vec3, MAX_PLAYER> ghostTarget_{};	// 서버가 알려준 최신 위치 (보간 목표)
	std::array<glm::vec3, MAX_PLAYER> ghostPos_{};		// 지금 그릴 위치
	std::array<bool, MAX_PLAYER> ghostActive_{};		// true인 칸만 그림

	std::vector<BLOCK*> blocks_;
	std::vector<BOUNCE_BLOCK*> bounceBlocks_;
	std::vector<BREAKABLE_BLOCK*> breakableBlocks_;
	std::vector<SPIKE_BLOCK*> spikeBlocks_;
	std::vector<ARROW_BLOCK*> arrowBlocks_;
	std::vector<STAR*> stars_;
	std::vector<ITEM*> items_;

	float effectTime_[(int)ITEM_TYPE::COUNT] = {};		// 효과별 남은 시간(초), 0 = 꺼짐

	GameState gameState_;
	bool gameStarted_;
	int score_;
	bool showTrajectory_;
	bool ThirdPersonView_;
	bool activateGravity_;

	GLuint titleTextureID_;
	GLuint endingTextureID_;

	// 별 개수 추적
	int collectedStars_;
	int playerStarCount_[MAX_PLAYER]{}; // 플레이어 별 개수 추적
	int totalStars_;

	// 서버가 정렬해서 보내준 최종 순위
	int endingRankCount_ = 0;
	RankData endingRanks_[MAX_PLAYER]{};


	// 세이브 포인트
	glm::vec3 spawnPoint_;

	void createFloorBlocks();
	void createBounceBlocks();
	void createBreakableBlocks();
	void createSpikeBlocks();
	void createArrowBlocks();
	void createStars();
	void createItems();

	bool loadTitleTexture(const char* filepath);
	bool loadEndingTexture(const char* filepath);
	void drawTitleScreen();
	void drawEndingScreen();
	void drawLobby();
	void initGhosts();
	void drawGhosts();
	void updateGhosts(float deltaTime);

	TrajectoryPredictor trajectoryPredictor_;

	void processNetEvents();

public:
	void  enterLobby() { gameState_ = GameState::LOBBY; isReady_ = false; }
	bool  toggleReady() { isReady_ = !isReady_; return isReady_; }
	bool  isReady() const { return isReady_; }
	void  startCountdown(float sec) { gameState_ = GameState::COUNTDOWN; countdownTime_ = sec; }
	float getCountdownTime() const { return countdownTime_; }

private:
	bool  isReady_ = false;
	float countdownTime_ = 0.0f;

	void onGameEnd(const SC_GAME_END_PACKET& packet);
};