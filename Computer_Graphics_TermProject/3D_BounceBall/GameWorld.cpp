#include "GameWorld.h"
#include <iostream>
#include <windows.h>
#include <mmsystem.h>
#include <string>
#include "stb_image.h"
#pragma comment(lib, "winmm.lib")

GameWorld::GameWorld(GLuint shaderID)
	: shaderProgramID_(shaderID)
	, gameState_(GameState::TITLE)
	, gameStarted_(false)
	, score_(0)
	, titleTextureID_(0)
	, endingTextureID_(0)
	, collectedStars_(0)
	, totalStars_(0)
	, spawnPoint_(0.0f, 2.0f, 0.0f)  // 초기 스폰 위치
	, activateGravity_(true)
{
}


GameWorld::~GameWorld()
{
	cleanup();
	// 타이틀 텍스처 정리
	if (titleTextureID_ != 0) {
		glDeleteTextures(1, &titleTextureID_);
	}

	if (endingTextureID_ != 0) {
		glDeleteTextures(1, &endingTextureID_);
	}
}

bool GameWorld::loadTitleTexture(const char* filepath)
{
	int width, height, channels;
	unsigned char* data = stbi_load(filepath, &width, &height, &channels, 0);

	if (!data) {
		std::cerr << "Failed to load title image: " << filepath << std::endl;
		return false;
	}

	std::cout << "Title texture loaded: " << filepath
		<< " (" << width << "x" << height << ", " << channels << " channels)" << std::endl;

	glGenTextures(1, &titleTextureID_);
	glBindTexture(GL_TEXTURE_2D, titleTextureID_);

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	GLenum format = (channels == 4) ? GL_RGBA : GL_RGB;
	glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	stbi_image_free(data);
	glBindTexture(GL_TEXTURE_2D, 0);

	return true;
}

bool GameWorld::loadEndingTexture(const char* filepath)
{
	int width, height, channels;
	unsigned char* data = stbi_load(filepath, &width, &height, &channels, 0);

	if (!data) {
		std::cerr << "Failed to load ending image: " << filepath << std::endl;
		return false;
	}

	glGenTextures(1, &endingTextureID_);
	glBindTexture(GL_TEXTURE_2D, endingTextureID_);

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	GLenum format = (channels == 4) ? GL_RGBA : GL_RGB;
	glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	stbi_image_free(data);
	glBindTexture(GL_TEXTURE_2D, 0);

	return true;
}

void GameWorld::drawTitleScreen()
{
	if (titleTextureID_ == 0) {
		std::cout << "No title texture loaded!" << std::endl;
		return;
	}

	// 셰이더 비활성화
	glUseProgram(0);

	// 3D 렌더링 비활성화
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);
	glDisable(GL_CULL_FACE);

	// 2D 직교 투영으로 전환
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	gluOrtho2D(0, glutGet(GLUT_WINDOW_WIDTH), 0, glutGet(GLUT_WINDOW_HEIGHT));

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	// ========================================
	// 🔧 여기서 직접 조정 가능!
	// ========================================
	float windowWidth = (float)glutGet(GLUT_WINDOW_WIDTH);
	float windowHeight = (float)glutGet(GLUT_WINDOW_HEIGHT);

	// 이미지 크기 및 위치 조정 (원하는 대로 수정하세요!)
	float imageX = 0.0f;           // 이미지 X 시작 위치
	float imageY = 0.0f;           // 이미지 Y 시작 위치
	float imageWidth = windowWidth;   // 이미지 너비
	float imageHeight = windowHeight; // 이미지 높이

	// 텍스처 좌표 조정 (이미지 잘림 방지)
	float texLeft = 0.0f;    // 왼쪽 텍스처 좌표 (0.0 ~ 1.0)
	float texRight = 1.0f;   // 오른쪽 텍스처 좌표 (0.0 ~ 1.0)
	float texTop = 0.0f;     // 위쪽 텍스처 좌표 (0.0 ~ 1.0)
	float texBottom = 1.0f;  // 아래쪽 텍스처 좌표 (0.0 ~ 1.0)
	// ========================================

	// 텍스처 바인딩
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, titleTextureID_);

	// 이미지 그리기
	glColor3f(1.0f, 1.0f, 1.0f);
	glBegin(GL_QUADS);

	// 왼쪽 아래
	glTexCoord2f(texLeft, texBottom);
	glVertex2f(imageX, imageY);

	// 오른쪽 아래
	glTexCoord2f(texRight, texBottom);
	glVertex2f(imageX + imageWidth, imageY);

	// 오른쪽 위
	glTexCoord2f(texRight, texTop);
	glVertex2f(imageX + imageWidth, imageY + imageHeight);

	// 왼쪽 위
	glTexCoord2f(texLeft, texTop);
	glVertex2f(imageX, imageY + imageHeight);
	glEnd();

	glDisable(GL_TEXTURE_2D);

	// 텍스트
	//glColor3f(1.0f, 1.0f, 0.0f);
	//glRasterPos2f(windowWidth / 2.0f - 100.0f, 50.0f);
	//std::string text = "Press Any Key to Start";
	//for (char c : text) {
	//	glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
	//}

	// 원래 상태로 복원
	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);

	glEnable(GL_DEPTH_TEST);
}

void GameWorld::drawEndingScreen()
{
	if (endingTextureID_ == 0) return;

	glUseProgram(0);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);
	glDisable(GL_CULL_FACE);

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	gluOrtho2D(0, glutGet(GLUT_WINDOW_WIDTH), 0, glutGet(GLUT_WINDOW_HEIGHT));

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	// 전체 화면 꽉 차게 그리기
	float windowWidth = (float)glutGet(GLUT_WINDOW_WIDTH);
	float windowHeight = (float)glutGet(GLUT_WINDOW_HEIGHT);

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, endingTextureID_); // <--- 엔딩 텍스처 바인딩

	glColor3f(1.0f, 1.0f, 1.0f);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, 1.0f); glVertex2f(0.0f, 0.0f);
	glTexCoord2f(1.0f, 1.0f); glVertex2f(windowWidth, 0.0f);
	glTexCoord2f(1.0f, 0.0f); glVertex2f(windowWidth, windowHeight);
	glTexCoord2f(0.0f, 0.0f); glVertex2f(0.0f, windowHeight);
	glEnd();

	glDisable(GL_TEXTURE_2D);

	// 축하 메시지 출력
	glColor3f(1.0f, 1.0f, 0.0f);
	glRasterPos2f(windowWidth / 2.0f - 170.0f, 50.0f);
	std::string text = "2025 Computer Graphics Term Progect!";
	for (char c : text) {
		glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
	}

	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glEnable(GL_DEPTH_TEST);
}

void GameWorld::startGame()
{
	gameState_ = GameState::PLAYING;
	gameStarted_ = true;
	std::cout << "Game Started!" << std::endl;
}

void GameWorld::initialize()
{
	std::cout << "Initializing GameWorld..." << std::endl;

	loadTitleTexture("img/title.jpg");
	loadEndingTexture("img/clear.jpg");

	// 플레이어 초기화
	player_.init("obj/uv_sphere.obj", shaderProgramID_, 1.0f, 1.0f, 0.0f);
	player_.setTranslation(spawnPoint_);  // 스폰 포인트에서 시작
	player_.setSelfScale(glm::vec3(0.5f, 0.5f, 0.5f));
	player_.radius_ = 0.5f * player_.getScaleFactor();

	// 맵 생성 (순서대로 배치)
	createFloorBlocks();      // 기본 바닥 및 안전지대
	createBounceBlocks();     // 점프 구간
	createBreakableBlocks();  // 함정 다리
	createSpikeBlocks();      // 가시 장애물
	createArrowBlocks();     // 화살표 블럭
	createStars();            // 별 배치

	// 총 별 개수 설정
	totalStars_ = stars_.size();
	collectedStars_ = 0;

	trajectoryPredictor_.init(shaderProgramID_);

	std::cout << "** GameWorld initialized **" << std::endl;
	std::cout << "Total stars: " << totalStars_ << std::endl;
}

void GameWorld::cleanup()
{
	for (auto block : blocks_) delete block;
	for (auto block : bounceBlocks_) delete block;
	for (auto block : breakableBlocks_) delete block;
	for (auto block : spikeBlocks_) delete block;
	for (auto block : arrowBlocks_) delete block;
	for (auto star : stars_) delete star;

	blocks_.clear();
	bounceBlocks_.clear();
	breakableBlocks_.clear();
	spikeBlocks_.clear();
	arrowBlocks_.clear();
	stars_.clear();
}

void GameWorld::reset(bool inputX)
{
	cleanup();

	// 플레이어를 마지막 세이브 포인트에서 리셋
	player_.reset();
	if (inputX) {
		spawnPoint_ = glm::vec3(0.0f, 2.0f, 0.0f);  // X 키 입력 시 초기 위치로 리셋
	}
	player_.setTranslation(spawnPoint_);  // 세이브 포인트로 이동
	player_.setSelfScale(glm::vec3(0.5f, 0.5f, 0.5f));
	player_.radius_ = 0.5f * player_.getScaleFactor();
	player_.velocity_ = glm::vec3(0.0f, 0.0f, 0.0f);

	createFloorBlocks();
	createBounceBlocks();
	createBreakableBlocks();
	createSpikeBlocks();
	createArrowBlocks();
	createStars();

	score_ = 0;
	collectedStars_ = 0;  // 별 개수 초기화
	totalStars_ = stars_.size();
	gameStarted_ = true;

	std::cout << "GameWorld reset at spawn point: ("
		<< spawnPoint_.x << ", " << spawnPoint_.y << ", " << spawnPoint_.z << ")" << std::endl;
}

void GameWorld::update(float deltaTime)
{
	// 타이틀 화면에서는 업데이트 안 함
	if (gameState_ != GameState::PLAYING) return;
	if (!gameStarted_) return;

	if (activateGravity_) player_.update(deltaTime);

	// 별 회전 애니메이션
	for (auto star : stars_) {
		glm::vec3 currentRot = star->getRotation();
		star->setRotation(currentRot + glm::vec3(0.0f, 90.0f * deltaTime, 0.0f));
	}

	checkCollisions();

	// 낙사 처리 - 세이브 포인트로 리스폰
	if (player_.getTranslation().y < -15.0f)
	{
		std::cout << "Fell off! Respawning at save point..." << std::endl;
		PlaySound(L"sounds\\dead.wav", NULL, SND_FILENAME | SND_ASYNC);

		// 스폰 포인트로 이동 (맵 초기화 안 함)
		player_.setTranslation(spawnPoint_);
		player_.velocity_ = glm::vec3(0.0f, 0.0f, 0.0f);

		// 부서진 블록 복구
		resetBreakableBlocks();
	}
}

void GameWorld::draw()
{
	// 타이틀 화면이면 타이틀만 그리기
	if (gameState_ == GameState::TITLE) {
		drawTitleScreen();
		return;
	}

	if (gameState_ == GameState::ENDING) {
		drawEndingScreen();
		return;
	}

	if (ThirdPersonView_) player_.draw();

	// 궤적 표시가 활성화된 경우에만 그리기
	if (showTrajectory_) {
		// 모든 블록을 하나의 벡터로 수집
		std::vector<ParentModel*> allBlocks;
		for (auto block : blocks_) allBlocks.push_back(block);
		for (auto block : bounceBlocks_) allBlocks.push_back(block);
		// 부서지지 않은 블록만 추가
		for (auto block : breakableBlocks_) {
			if (!block->isBroken_) {
				allBlocks.push_back(block);
			}
		}

		// 플레이어 궤적 예측 표시 (블록 정보 전달)
		trajectoryPredictor_.draw(
			player_.getPosition(),
			player_.velocity_,
			allBlocks,
			50,
			0.05f
		);
	}

	for (auto block : blocks_) block->draw();
	for (auto block : bounceBlocks_) block->draw();

	// 부서지지 않은 블록만 그리기
	for (auto block : breakableBlocks_) {
		if (!block->isBroken_) {
			block->draw();
		}
	}

	for (auto block : spikeBlocks_) block->draw();
	for (auto block : arrowBlocks_) block->draw();
	for (auto star : stars_) star->draw();

	// UI 그리기
	drawUI();
}

// 새로운 함수: UI 그리기
void GameWorld::drawUI()
{
	if (gameState_ != GameState::PLAYING) return;

	// 셰이더 비활성화
	glUseProgram(0);

	// 2D 모드로 전환
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	gluOrtho2D(0, glutGet(GLUT_WINDOW_WIDTH), 0, glutGet(GLUT_WINDOW_HEIGHT));

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);

	// 별 개수 표시 (왼쪽 위)
	float windowHeight = (float)glutGet(GLUT_WINDOW_HEIGHT);

	glColor3f(1.0f, 1.0f, 0.0f);  // 노란색
	glRasterPos2f(20.0f, windowHeight - 30.0f);

	std::string starText = "Stars: " + std::to_string(collectedStars_) + "/" + std::to_string(totalStars_);
	for (char c : starText) {
		glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
	}

	// 점수 표시 (왼쪽 위 두 번째 줄)
	glColor3f(0.0f, 1.0f, 1.0f);  // 시안색
	glRasterPos2f(20.0f, windowHeight - 60.0f);

	// std::string scoreText = "Score: " + std::to_string(score_);
	// for (char c : scoreText) {
	// 	glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
	// }

	// 원래 상태로 복원
	glEnable(GL_DEPTH_TEST);

	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
}

void GameWorld::checkCollisions()
{
	// 일반 블럭
	for (auto block : blocks_) {
		if (player_.checkCollision(block)) {
			player_.onCollision(block);
			PlaySound(L"sounds\\bounce.wav", NULL, SND_FILENAME | SND_ASYNC);
		}
	}

	// 튕기는 블럭
	for (auto block : bounceBlocks_) {
		if (player_.checkCollision(block)) {
			player_.onCollision(block);
			// 바운스 효과음 재생
			PlaySound(L"sounds\\bounce.wav", NULL, SND_FILENAME | SND_ASYNC);
		}
	}

	// 부서지는 블럭
	for (auto block : breakableBlocks_) {
		// 이미 부서진 블록은 건너뛰기
		if (block->isBroken_) continue;

		if (player_.checkCollision(block)) {
			player_.onCollision(block);
			PlaySound(L"sounds\\bounce.wav", NULL, SND_FILENAME | SND_ASYNC);
			block->onCollision(&player_); // 부서짐 체크

			if (block->isBroken_) {
				addScore(10);
				// 블록을 삭제하지 않고 그냥 부서진 상태로 유지
			}
		}
	}

	// 가시 블럭 - 세이브 포인트로 리스폰
	for (auto block : spikeBlocks_) {
		if (player_.checkCollision(block)) {
			std::cout << "Hit spike! Respawning at save point..." << std::endl;
			PlaySound(L"sounds\\dead.wav", NULL, SND_FILENAME | SND_ASYNC);

			// 스폰 포인트로 이동
			player_.setTranslation(spawnPoint_);
			player_.velocity_ = glm::vec3(0.0f, 0.0f, 0.0f);

			// 부서진 블록 복구
			resetBreakableBlocks();
			return;
		}
	}

	// 화살표 블록 충돌 체크
	for (auto block : arrowBlocks_) {
		if (player_.checkCollision(block)) {
			player_.onCollision(block);
		}
	}

	// 별 수집 - 세이브 포인트 업데이트!
	for (auto it = stars_.begin(); it != stars_.end(); ) {
		STAR* star = *it;
		if (player_.checkCollision(star)) {
			player_.onCollision(star);
			star->onCollision(&player_);

			if (star->isCollected_) {
				PlaySound(L"sounds\\star.wav", NULL, SND_FILENAME | SND_ASYNC);
				addScore(50);
				collectedStars_++;  // 별 개수 증가

				if (collectedStars_ >= totalStars_) {
					std::cout << "All stars collected! You win!" << std::endl;
					gameState_ = GameState::ENDING;

					PlaySound(L"sounds\\clear.wav", NULL, SND_FILENAME | SND_ASYNC);
				}

				// 세이브 포인트 업데이트 (별 위치로)
				spawnPoint_ = star->getPosition();
				spawnPoint_.y += 2.0f;  // 약간 위로 (별보다 위에서 스폰)

				std::cout << "Star collected! (" << collectedStars_ << "/" << totalStars_ << ")" << std::endl;
				std::cout << "Save point updated: (" << spawnPoint_.x << ", " << spawnPoint_.y << ", " << spawnPoint_.z << ")" << std::endl;

				delete star;
				it = stars_.erase(it);
				continue;
			}
		}
		++it;
	}
}

void GameWorld::addBlock(BLOCK* block) { blocks_.push_back(block); }
void GameWorld::addBounceBlock(BOUNCE_BLOCK* block) { bounceBlocks_.push_back(block); }
void GameWorld::addBreakableBlock(BREAKABLE_BLOCK* block) { breakableBlocks_.push_back(block); }
void GameWorld::addSpikeBlock(SPIKE_BLOCK* block) { spikeBlocks_.push_back(block); }
void GameWorld::addArrowBlock(ARROW_BLOCK* block) { arrowBlocks_.push_back(block); }
void GameWorld::addStar(STAR* star) { stars_.push_back(star); }

// ==========================================
//              레벨 디자인 구현
// ==========================================

//

void GameWorld::createFloorBlocks()
{
	// ==========================================
	//          SECTION 1: 시작 구역
	// ==========================================
	// 1-1. 시작 플랫폼 (3x3 안전지대) - 바운스로 변경
	for (int x = -1; x <= 1; x++) {
		for (int z = -1; z <= 1; z++) {
			BOUNCE_BLOCK* block = new BOUNCE_BLOCK();
			block->init("obj/uv_cube.obj", shaderProgramID_, 0.3f, 0.3f, 0.3f);
			block->loadTexture("img/default.png");
			block->setTranslation(glm::vec3(x * 2.0f, -1.0f, z * 2.0f));
			block->setSelfScale(glm::vec3(1.0f, 1.0f, 1.0f));
			blocks_.push_back(block);
		}
	}

	// ==========================================
	//     SECTION 2: 좁은 징검다리 (난이도 상)
	// ==========================================
	// 2-1. 일직선 징검다리 (작은 발판들) - 바운스로 변경
	for (int i = 1; i <= 5; i++) {
		BOUNCE_BLOCK* block = new BOUNCE_BLOCK();
		block->init("obj/uv_cube.obj", shaderProgramID_, 0.5f, 0.5f, 0.5f);
		block->loadTexture("img/default.png");
		block->setTranslation(glm::vec3(0.0f, -1.0f - i * 0.3f, -2.0f - (i * 2.5f)));
		block->setSelfScale(glm::vec3(0.6f, 0.6f, 0.6f));
		blocks_.push_back(block);
	}

	// 2-2. 지그재그 구간 - 바운스로 변경
	for (int i = 0; i < 4; i++) {
		BOUNCE_BLOCK* block = new BOUNCE_BLOCK();
		block->init("obj/uv_cube.obj", shaderProgramID_, 0.5f, 0.5f, 0.5f);
		block->loadTexture("img/default.png");
		float xOffset = (i % 2 == 0) ? -2.0f : 2.0f;
		block->setTranslation(glm::vec3(xOffset, -2.5f, -17.0f - (i * 3.0f)));
		block->setSelfScale(glm::vec3(0.7f, 0.7f, 0.7f));
		blocks_.push_back(block);
	}

	// ==========================================
	//     SECTION 3: 첫 번째 화살표 + 착지 구역
	// ==========================================
	// 3-1. 화살표 발사대 발판 - 바운스로 변경
	BOUNCE_BLOCK* arrowPlatform1 = new BOUNCE_BLOCK();
	arrowPlatform1->init("obj/uv_cube.obj", shaderProgramID_, 0.4f, 0.4f, 0.4f);
	arrowPlatform1->loadTexture("img/default.png");
	arrowPlatform1->setTranslation(glm::vec3(2.0f, -2.5f, -30.0f));
	arrowPlatform1->setSelfScale(glm::vec3(1.0f, 1.0f, 1.0f));
	blocks_.push_back(arrowPlatform1);

	// 3-2. 화살표 착지 플랫폼 (높은 곳) - 바운스로 변경
	for (int x = -1; x <= 1; x++) {
		BOUNCE_BLOCK* block = new BOUNCE_BLOCK();
		block->init("obj/uv_cube.obj", shaderProgramID_, 0.6f, 0.6f, 0.6f);
		block->loadTexture("img/default.png");
		block->setTranslation(glm::vec3(x * 2.0f, 5.0f, -45.0f));
		block->setSelfScale(glm::vec3(1.0f, 1.0f, 1.0f));
		blocks_.push_back(block);
	}

	// ==========================================
	//     SECTION 4: 부서지는 다리 + 바운스 조합
	// ==========================================
	// 4-1. 좁은 안전 발판 - 바운스로 변경
	BOUNCE_BLOCK* safe1 = new BOUNCE_BLOCK();
	safe1->init("obj/uv_cube.obj", shaderProgramID_, 0.5f, 0.5f, 0.5f);
	safe1->loadTexture("img/default.png");
	safe1->setTranslation(glm::vec3(0.0f, 5.0f, -50.0f));
	safe1->setSelfScale(glm::vec3(0.8f, 0.8f, 0.8f));
	blocks_.push_back(safe1);

	// 4-2. 낮은 바운스 착지 구역 - 바운스로 변경
	for (int x = -1; x <= 1; x++) {
		BOUNCE_BLOCK* block = new BOUNCE_BLOCK();
		block->init("obj/uv_cube.obj", shaderProgramID_, 0.5f, 0.5f, 0.5f);
		block->loadTexture("img/default.png");
		block->setTranslation(glm::vec3(x * 2.0f, -1.0f, -60.0f));
		block->setSelfScale(glm::vec3(1.0f, 1.0f, 1.0f));
		blocks_.push_back(block);
	}

	// ==========================================
	//     SECTION 5: 연속 화살표 스테이지
	// ==========================================
	// 5-1. 두 번째 화살표 발사대 - 바운스로 변경
	BOUNCE_BLOCK* arrowPlatform2 = new BOUNCE_BLOCK();
	arrowPlatform2->init("obj/uv_cube.obj", shaderProgramID_, 0.4f, 0.4f, 0.4f);
	arrowPlatform2->loadTexture("img/default.png");
	arrowPlatform2->setTranslation(glm::vec3(-3.0f, -1.0f, -65.0f));
	arrowPlatform2->setSelfScale(glm::vec3(1.0f, 1.0f, 1.0f));
	blocks_.push_back(arrowPlatform2);

	// 5-2. 중간 전환 플랫폼 - 바운스로 변경
	BOUNCE_BLOCK* midPlatform = new BOUNCE_BLOCK();
	midPlatform->init("obj/uv_cube.obj", shaderProgramID_, 0.5f, 0.5f, 0.5f);
	midPlatform->loadTexture("img/default.png");
	midPlatform->setTranslation(glm::vec3(-3.0f, 2.0f, -80.0f));
	midPlatform->setSelfScale(glm::vec3(1.2f, 1.2f, 1.2f));
	blocks_.push_back(midPlatform);

	// 5-3. 세 번째 화살표 발사대 - 바운스로 변경
	BOUNCE_BLOCK* arrowPlatform3 = new BOUNCE_BLOCK();
	arrowPlatform3->init("obj/uv_cube.obj", shaderProgramID_, 0.4f, 0.4f, 0.4f);
	arrowPlatform3->loadTexture("img/default.png");
	arrowPlatform3->setTranslation(glm::vec3(0.0f, 2.0f, -85.0f));
	arrowPlatform3->setSelfScale(glm::vec3(1.0f, 1.0f, 1.0f));
	blocks_.push_back(arrowPlatform3);

	// ==========================================
	//     SECTION 6: 가시 미로 구간
	// ==========================================
	// 6-1. 가시 앞 발판 - 바운스로 변경
	BOUNCE_BLOCK* spikeEntry = new BOUNCE_BLOCK();
	spikeEntry->init("obj/uv_cube.obj", shaderProgramID_, 0.5f, 0.5f, 0.5f);
	spikeEntry->loadTexture("img/default.png");
	spikeEntry->setTranslation(glm::vec3(0.0f, 7.0f, -100.0f));
	spikeEntry->setSelfScale(glm::vec3(1.0f, 1.0f, 1.0f));
	blocks_.push_back(spikeEntry);

	// 6-2. 가시 후 안전 발판 - 바운스로 변경
	BOUNCE_BLOCK* spikeExit = new BOUNCE_BLOCK();
	spikeExit->init("obj/uv_cube.obj", shaderProgramID_, 0.5f, 0.5f, 0.5f);
	spikeExit->loadTexture("img/default.png");
	spikeExit->setTranslation(glm::vec3(0.0f, 7.0f, -115.0f));
	spikeExit->setSelfScale(glm::vec3(1.0f, 1.0f, 1.0f));
	blocks_.push_back(spikeExit);

	// ==========================================
	//     SECTION 7: 최종 점프 챌린지
	// ==========================================
	// 7-1. 높이가 다른 계단식 발판 - 바운스로 변경
	for (int i = 0; i < 5; i++) {
		BOUNCE_BLOCK* block = new BOUNCE_BLOCK();
		block->init("obj/uv_cube.obj", shaderProgramID_, 0.6f, 0.6f, 0.6f);
		block->loadTexture("img/default.png");
		block->setTranslation(glm::vec3(0.0f, 7.0f + i * 1.5f, -120.0f - (i * 4.0f)));
		block->setSelfScale(glm::vec3(0.8f, 0.8f, 0.8f));
		blocks_.push_back(block);
	}

	// ==========================================
	//     SECTION 8: 도착 지점 (Goal)
	// ==========================================
	for (int x = -2; x <= 2; x++) {
		for (int z = 0; z < 3; z++) {
			BOUNCE_BLOCK* block = new BOUNCE_BLOCK();
			block->init("obj/uv_cube.obj", shaderProgramID_, 1.0f, 0.84f, 0.0f); // 금색
			block->loadTexture("img/default.png");
			block->setTranslation(glm::vec3(x * 2.0f, 14.0f, -142.0f - (z * 2.0f)));
			block->setSelfScale(glm::vec3(1.0f, 1.0f, 1.0f));
			blocks_.push_back(block);
		}
	}
}

void GameWorld::createBounceBlocks()
{
	// 바운스 1: 첫 번째 높이 극복용
	BOUNCE_BLOCK* bounce1 = new BOUNCE_BLOCK();
	bounce1->init("obj/uv_cube.obj", shaderProgramID_, 0.0f, 0.5f, 1.0f);
	bounce1->loadTexture("img/default.png");
	bounce1->setTranslation(glm::vec3(0.0f, -4.0f, -18.0f));
	bounce1->setSelfScale(glm::vec3(1.0f, 1.0f, 1.0f));
	bounceBlocks_.push_back(bounce1);

	// 바운스 2: 고공 점프용 (부서지는 다리 전)
	BOUNCE_BLOCK* bounce2 = new BOUNCE_BLOCK();
	bounce2->init("obj/uv_cube.obj", shaderProgramID_, 0.0f, 0.5f, 1.0f);
	bounce2->loadTexture("img/default.png");
	bounce2->setTranslation(glm::vec3(0.0f, 3.5f, -52.0f));
	bounce2->setSelfScale(glm::vec3(1.0f, 1.0f, 1.0f));
	bounceBlocks_.push_back(bounce2);

	// 바운스 3: 최종 구간 진입용
	BOUNCE_BLOCK* bounce3 = new BOUNCE_BLOCK();
	bounce3->init("obj/uv_cube.obj", shaderProgramID_, 0.0f, 0.5f, 1.0f);
	bounce3->loadTexture("img/default.png");
	bounce3->setTranslation(glm::vec3(0.0f, 5.0f, -117.0f));
	bounce3->setSelfScale(glm::vec3(1.0f, 1.0f, 1.0f));
	bounceBlocks_.push_back(bounce3);
}

void GameWorld::createBreakableBlocks()
{
	// 부서지는 다리 1: 바운스 후 빠르게 통과해야 함
	for (int i = 0; i < 4; i++) {
		BREAKABLE_BLOCK* block = new BREAKABLE_BLOCK();
		block->init("obj/uv_cube.obj", shaderProgramID_, 1.0f, 0.0f, 1.0f);
		block->loadTexture("img/breakable.png");
		block->setTranslation(glm::vec3(0.0f, 5.0f, -54.0f - (i * 1.8f)));
		block->setSelfScale(glm::vec3(0.7f, 0.7f, 0.7f));
		breakableBlocks_.push_back(block);
	}

	// 부서지는 다리 2: 연속 점프 필요
	for (int i = 0; i < 3; i++) {
		BREAKABLE_BLOCK* block = new BREAKABLE_BLOCK();
		block->init("obj/uv_cube.obj", shaderProgramID_, 1.0f, 0.0f, 1.0f);
		block->loadTexture("img/breakable.png");
		float xOffset = (i % 2 == 0) ? -1.5f : 1.5f;
		block->setTranslation(glm::vec3(xOffset, 7.0f, -105.0f - (i * 3.0f)));
		block->setSelfScale(glm::vec3(0.8f, 0.8f, 0.8f));
		breakableBlocks_.push_back(block);
	}
}

void GameWorld::createSpikeBlocks()
{
	// 가시 1: 낮은 가시 (점프로 통과)
	SPIKE_BLOCK* spike1 = new SPIKE_BLOCK();
	spike1->init("obj/uv_cube.obj", shaderProgramID_, 1.0f, 0.0f, 0.0f);
	spike1->loadTexture("img/spike.png");
	spike1->setTranslation(glm::vec3(0.0f, -2.5f, -28.0f));
	spike1->setSelfScale(glm::vec3(1.0f, 0.4f, 1.5f));
	spikeBlocks_.push_back(spike1);

	// 가시 2: 지그재그 구간 중간
	SPIKE_BLOCK* spike2 = new SPIKE_BLOCK();
	spike2->init("obj/uv_cube.obj", shaderProgramID_, 1.0f, 0.0f, 0.0f);
	spike2->loadTexture("img/spike.png");
	spike2->setTranslation(glm::vec3(0.0f, -2.5f, -23.0f));
	spike2->setSelfScale(glm::vec3(0.8f, 0.4f, 0.8f));
	spikeBlocks_.push_back(spike2);

	// 가시 3: 가시 미로 구간 (3개 배치)
	for (int i = 0; i < 3; i++) {
		SPIKE_BLOCK* spike = new SPIKE_BLOCK();
		spike->init("obj/uv_cube.obj", shaderProgramID_, 1.0f, 0.0f, 0.0f);
		spike->loadTexture("img/spike.png");
		spike->setTranslation(glm::vec3(0.0f, 6.5f, -107.0f - (i * 2.5f)));
		spike->setSelfScale(glm::vec3(0.6f, 0.5f, 1.2f));
		spikeBlocks_.push_back(spike);
	}
}

void GameWorld::createArrowBlocks()
{
	// 화살표 1: 수평 + 상승 (첫 번째 화살표 구간)
	ARROW_BLOCK* arrow1 = new ARROW_BLOCK();
	arrow1->init("obj/uv_cube.obj", shaderProgramID_, 0.0f, 1.0f, 0.0f);
	arrow1->loadTexture("img/arrow.png");
	arrow1->setTranslation(glm::vec3(2.0f, -1.5f, -32.0f));
	arrow1->setSelfScale(glm::vec3(1.0f, 1.0f, 1.0f));
	arrow1->setArrowDirection(glm::vec3(0.0f, 0.5f, -1.0f));
	arrow1->launchSpeed_ = 15.0f;
	arrowBlocks_.push_back(arrow1);

	// 화살표 2: 옆으로 + 하강 (두 번째 화살표 구간)
	ARROW_BLOCK* arrow2 = new ARROW_BLOCK();
	arrow2->init("obj/uv_cube.obj", shaderProgramID_, 0.0f, 1.0f, 0.0f);
	arrow2->loadTexture("img/arrow.png");
	arrow2->setTranslation(glm::vec3(-3.0f, 0.5f, -67.0f));
	arrow2->setSelfScale(glm::vec3(1.0f, 1.0f, 1.0f));
	arrow2->setArrowDirection(glm::vec3(0.0f, 0.3f, -1.0f));
	arrow2->launchSpeed_ = 18.0f;
	arrowBlocks_.push_back(arrow2);

	// 화살표 3: 급상승 (세 번째 화살표 구간)
	ARROW_BLOCK* arrow3 = new ARROW_BLOCK();
	arrow3->init("obj/uv_cube.obj", shaderProgramID_, 0.0f, 1.0f, 0.0f);
	arrow3->loadTexture("img/arrow.png");
	arrow3->setTranslation(glm::vec3(0.0f, 3.0f, -87.0f));
	arrow3->setSelfScale(glm::vec3(1.0f, 1.0f, 1.0f));
	arrow3->setArrowDirection(glm::vec3(0.0f, 0.8f, -0.5f));
	arrow3->launchSpeed_ = 20.0f;
	arrowBlocks_.push_back(arrow3);
}

void GameWorld::createStars()
{
	//// 별 1: 시작 지점 (튜토리얼)
	//STAR* star1 = new STAR();
	//star1->init("obj/star.obj", shaderProgramID_, 1.0f, 1.0f, 0.0f);
	//star1->setRotation(glm::vec3(90.0f, 0.0f, 0.0f));
	//star1->setTranslation(glm::vec3(0.0f, 2.0f, 0.0f));
	//star1->setSelfScale(glm::vec3(0.15f, 0.3f, 0.15f));
	//stars_.push_back(star1);

	// 별 2: 징검다리 중간 (어려움)
	STAR* star2 = new STAR();
	star2->init("obj/star.obj", shaderProgramID_, 1.0f, 1.0f, 0.0f);
	star2->setRotation(glm::vec3(90.0f, 0.0f, 0.0f));
	star2->setTranslation(glm::vec3(0.0f, 0.5f, -10.0f));
	star2->setSelfScale(glm::vec3(0.15f, 0.3f, 0.15f));
	stars_.push_back(star2);

	// 별 3: 첫 화살표 착지 지점
	STAR* star3 = new STAR();
	star3->init("obj/star.obj", shaderProgramID_, 1.0f, 1.0f, 0.0f);
	star3->setRotation(glm::vec3(90.0f, 0.0f, 0.0f));
	star3->setTranslation(glm::vec3(0.0f, 8.0f, -45.0f));
	star3->setSelfScale(glm::vec3(0.15f, 0.3f, 0.15f));
	stars_.push_back(star3);

	// 별 4: 부서지는 다리 끝
	STAR* star4 = new STAR();
	star4->init("obj/star.obj", shaderProgramID_, 1.0f, 1.0f, 0.0f);
	star4->setRotation(glm::vec3(90.0f, 0.0f, 0.0f));
	star4->setTranslation(glm::vec3(0.0f, 2.0f, -60.0f));
	star4->setSelfScale(glm::vec3(0.15f, 0.3f, 0.15f));
	stars_.push_back(star4);

	// 별 5: 중간 전환 플랫폼
	STAR* star5 = new STAR();
	star5->init("obj/star.obj", shaderProgramID_, 1.0f, 1.0f, 0.0f);
	star5->setRotation(glm::vec3(90.0f, 0.0f, 0.0f));
	star5->setTranslation(glm::vec3(-3.0f, 5.0f, -80.0f));
	star5->setSelfScale(glm::vec3(0.15f, 0.3f, 0.15f));
	stars_.push_back(star5);

	// 별 6: 가시 미로 위 (매우 어려움)
	STAR* star6 = new STAR();
	star6->init("obj/star.obj", shaderProgramID_, 1.0f, 1.0f, 0.0f);
	star6->setRotation(glm::vec3(90.0f, 0.0f, 0.0f));
	star6->setTranslation(glm::vec3(0.0f, 10.0f, -107.0f));
	star6->setSelfScale(glm::vec3(0.15f, 0.3f, 0.15f));
	stars_.push_back(star6);

	// 별 7: 최종 계단 중간
	STAR* star7 = new STAR();
	star7->init("obj/star.obj", shaderProgramID_, 1.0f, 1.0f, 0.0f);
	star7->setRotation(glm::vec3(90.0f, 0.0f, 0.0f));
	star7->setTranslation(glm::vec3(0.0f, 12.0f, -130.0f));
	star7->setSelfScale(glm::vec3(0.15f, 0.3f, 0.15f));
	stars_.push_back(star7);

	// 별 8: 골인 지점 (보너스)
	STAR* star8 = new STAR();
	star8->init("obj/star.obj", shaderProgramID_, 1.0f, 1.0f, 0.0f);
	star8->setRotation(glm::vec3(90.0f, 0.0f, 0.0f));
	star8->setTranslation(glm::vec3(0.0f, 17.0f, -143.0f));
	star8->setSelfScale(glm::vec3(0.2f, 0.4f, 0.2f)); // 큰 별
	stars_.push_back(star8);
}

void GameWorld::resetBreakableBlocks()
{
	for (auto block : breakableBlocks_) {
		block->isBroken_ = false;
	}
	std::cout << "Breakable blocks restored!" << std::endl;
}