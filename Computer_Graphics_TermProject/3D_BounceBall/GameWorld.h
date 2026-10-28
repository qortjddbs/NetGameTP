#pragma once
#include "Headers.h"
#include "Player.h"
#include "Block.h"
#include <vector>
#include "PathTrace.h"

enum class GameState {
	TITLE,
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
	std::vector<BLOCK*> blocks_;
	std::vector<BOUNCE_BLOCK*> bounceBlocks_;
	std::vector<BREAKABLE_BLOCK*> breakableBlocks_;
	std::vector<SPIKE_BLOCK*> spikeBlocks_;
	std::vector<ARROW_BLOCK*> arrowBlocks_;
	std::vector<STAR*> stars_;

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
	int totalStars_;

	// 세이브 포인트
	glm::vec3 spawnPoint_;

	void createFloorBlocks();
	void createBounceBlocks();
	void createBreakableBlocks();
	void createSpikeBlocks();
	void createArrowBlocks();
	void createStars();

	bool loadTitleTexture(const char* filepath);
	bool loadEndingTexture(const char* filepath);
	void drawTitleScreen();
	void drawEndingScreen();

	TrajectoryPredictor trajectoryPredictor_;
};