#include "Network.h"
#include "InputHandler.h"
#include "GameWorld.h"
#include <iostream>

InputHandler::InputHandler()
	: gameWorld_(nullptr)
	, camera_(nullptr)
	, depthTest_(nullptr)
	, cullFace_(nullptr)
	, projectionPerspective_(nullptr)
	, drawSolid_(nullptr)
	, drawWireframe_(nullptr)
	, turnOnLight_(nullptr)
	, lightIntensity_(nullptr)
	, playerMoveSpeed_(0.2f)
{
}

InputHandler::~InputHandler()
{
}

void InputHandler::setRenderingSettings(bool* depthTest, bool* cullFace, bool* projPerspective, bool* drawSolid, bool* drawWireframe)
{
	this->depthTest_ = depthTest;
	this->cullFace_ = cullFace;
	this->projectionPerspective_ = projPerspective;
	this->drawSolid_ = drawSolid;
	this->drawWireframe_ = drawWireframe;
}

void InputHandler::setLightingSettings(bool* turnLight, float* intensity)
{
	this->turnOnLight_ = turnLight;
	this->lightIntensity_ = intensity;
}

void InputHandler::handleKeyboard(unsigned char key, int x, int y)
{
	// GameWorld가 없으면 기본 키만 처리
	if (!gameWorld_) {
		handleRenderingKeys(key);
		handleLightingKeys(key);
		if (key == 'q' || key == 'Q') {
			exit(0);
		}
		return;
	}

	GameState state = gameWorld_->getGameState();

	// 타이틀 / 로비: 아직 맵에 들어가기 전
	if (state == GameState::TITLE || state == GameState::LOBBY) {
		if (key == ' ') { // 혼자 테스트 용: 스페이스바 누르면 반드시 그냥 시작, 나중에 삭제
			gameWorld_->startGame();
			return;
		}

		if (key == '\r') {			// Enter
			if (state == GameState::TITLE) {
				gameWorld_->enterLobby();
			}
			else if (state == GameState::LOBBY) {
				bool ready = gameWorld_->toggleReady();
				SendReady(ready);
				std::cout << "Ready: " << (ready ? "ON" : "OFF") << std::endl;
			}
		}

		if (key == 'q' || key == 'Q') {
			exit(0);
		}
		return;
	}

	// 카운트다운: 입력 무시
	if (state == GameState::COUNTDOWN) {
		if (key == 'q' || key == 'Q') {
			exit(0);
		}
		return;
	}

	keysPressed_[key] = true;  // 키 누름 상태

	handleRenderingKeys(key);
	handleLightingKeys(key);
	handleGameKeys(key);
	handleTrajectoryKeys(key);

	if (key == 'q' || key == 'Q') {
		exit(0);
	}
}

void InputHandler::handleKeyboardUp(unsigned char key, int x, int y)
{
	// 키 해제(up) 이벤트 처리
	keysPressed_[key] = false;
}

void InputHandler::handleSpecialKey(int key, int x, int y)
{
	if (!gameWorld_)
		return;

	GameState state = gameWorld_->getGameState();

	// 타이틀 화면에서는 스페셜 키 무시
	if (state == GameState::TITLE)
		return;

	// 스페셜 키 누름 상태 저장
	if (key < 256) {
		specialKeysPressed_[key] = true;
	}

	handleCameraSpecialKeys(key);
	handleGameSpecialKeys(key);
}

void InputHandler::handleSpecialKeyUp(int key, int x, int y)
{
	// 스페셜 키 해제
	if (key < 256) {
		specialKeysPressed_[key] = false;
	}
}

void InputHandler::updateKeyStates()
{
	// 매 프레임 호출 - 플레이어 이동 입력 처리
	if (!gameWorld_)
		return;

	// 타이틀 화면에서는 플레이어 이동 안 함
	if (gameWorld_->getGameState() != GameState::PLAYING)
		return;

	PLAYER* player = gameWorld_->getPlayer();
	if (!player || !camera_)
		return;

	bool isMoving = false;
	bool isGravityactivated = gameWorld_->getGravityStatus();

	// W/A/S/D 키 상태 확인
	if (isGravityactivated) {
		if (keysPressed_['w'] || keysPressed_['W']) {
			player->move(camera_->getForward(), camera_->getRight(), 0, playerMoveSpeed_);
			isMoving = true;
		}
		if (keysPressed_['a'] || keysPressed_['A']) {
			player->move(camera_->getForward(), camera_->getRight(), 1, playerMoveSpeed_);
			isMoving = true;
		}
		if (keysPressed_['s'] || keysPressed_['S']) {
			player->move(camera_->getForward(), camera_->getRight(), 2, playerMoveSpeed_);
			isMoving = true;
		}
		if (keysPressed_['d'] || keysPressed_['D']) {
			player->move(camera_->getForward(), camera_->getRight(), 3, playerMoveSpeed_);
			isMoving = true;
		}
	}
	else {
		if (specialKeysPressed_[GLUT_KEY_UP]) {
			player->moveDebug(0);
			isMoving = true;
		}
		if (specialKeysPressed_[GLUT_KEY_LEFT]) {
			player->moveDebug(1);
			isMoving = true;
		}
		if (specialKeysPressed_[GLUT_KEY_DOWN]) {
			player->moveDebug(2);
			isMoving = true;
		}
		if (specialKeysPressed_[GLUT_KEY_RIGHT]) {
			player->moveDebug(3);
			isMoving = true;
		}
		if (specialKeysPressed_[GLUT_KEY_CTRL_L]) {
			player->moveUp();
			isMoving = true;
		}
		if (specialKeysPressed_[GLUT_KEY_SHIFT_L]) {
			player->moveDown();
			isMoving = true;
		}
	}

	if (!isMoving) {
		// 입력 없음 - 감속 처리
		player->Deceleration(0.016f);
	}
}


void InputHandler::handleRenderingKeys(unsigned char key)
{
	if (!depthTest_ || !cullFace_ || !projectionPerspective_ || !drawSolid_ || !drawWireframe_)
		return;

	switch (key)
	{
	case 'h':
	case 'H':
		*depthTest_ = !(*depthTest_);
		std::cout << "Depth Test: " << (*depthTest_ ? "Enabled" : "Disabled") << std::endl;
		break;

	/*case 'u':
	case 'U':
		*cullFace_ = !(*cullFace_);
		std::cout << "Cull Face: " << (*cullFace_ ? "Enabled" : "Disabled") << std::endl;
		break;*/

	/*case 'p':
		*projectionPerspective_ = false;
		std::cout << "Orthographic Projection" << std::endl;`
		break;

	case 'P':
		*projectionPerspective_ = true;
		std::cout << "Perspective Projection" << std::endl;
		break;*/

	/*case 'm':
		*drawSolid_ = true;
		*drawWireframe_ = false;
		std::cout << "Draw Solid Mode" << std::endl;
		break;

	case 'M':
		*drawSolid_ = false;
		*drawWireframe_ = true;
		std::cout << "Draw Wireframe Mode" << std::endl;
		break;*/
	}
}

void InputHandler::handleLightingKeys(unsigned char key)
{
	if (!turnOnLight_ || !lightIntensity_)
		return;

	switch (key)
	{
	case 'r':
	case 'R':
		*turnOnLight_ = !(*turnOnLight_);
		std::cout << "Lighting: " << (*turnOnLight_ ? "ON" : "OFF") << std::endl;
		break;

	case '+':
		*lightIntensity_ += 0.1f;
		if (*lightIntensity_ > 1.0f) *lightIntensity_ = 1.0f;
		std::cout << "Light Intensity: " << *lightIntensity_ << std::endl;
		break;

	case '-':
		*lightIntensity_ -= 0.1f;
		if (*lightIntensity_ < 0.0f) *lightIntensity_ = 0.0f;
		std::cout << "Light Intensity: " << *lightIntensity_ << std::endl;
		break;
	}
}

void InputHandler::handleGameKeys(unsigned char key)
{
	if (!gameWorld_)
		return;

	switch (key)
	{
	case 'v':
	case 'V': {
		bool wasThirdPerson = gameWorld_->getThirdPersonView();
		gameWorld_->toggleThirdPersonView();
		bool isThirdPerson = gameWorld_->getThirdPersonView();

		// 3인칭으로 돌아갈 때 pitch를 제한 범위 내로 조정
		if (isThirdPerson && camera_) {
			float currentPitch = camera_->getPitch();
			float maxPitch = camera_->getMaxPitch_3rd();

			if (currentPitch > maxPitch) {
				camera_->setPitch(maxPitch);
			}
			else if (currentPitch < -maxPitch) {
				camera_->setPitch(-maxPitch);
			}
		}
		std::cout << "Camera View: " << (gameWorld_->getThirdPersonView() ? "Third Person" : "First Person") << std::endl;
		break;
	}
	case 'c':
	case 'C':
		gameWorld_->setGameStarted(!gameWorld_->isGameStarted());
		std::cout << "Game " << (gameWorld_->isGameStarted() ? "Started" : "Paused") << std::endl;
		break;

	case 'x':
	case 'X':
		gameWorld_->reset(true);
		std::cout << "Game Reset! Score: " << gameWorld_->getScore() << std::endl;
		break;
	}
}

void InputHandler::handleTrajectoryKeys(unsigned char key)
{
	if (!gameWorld_)
		return;

	switch (key)
	{
	case 't':
	case 'T':
		gameWorld_->toggleTrajectory();
		std::cout << "Trajectory: " << (gameWorld_->isTrajectoryVisible() ? "ON" : "OFF") << std::endl;
		break;
	}
}

void InputHandler::handleCameraSpecialKeys(int key)
{
	if (!camera_)
		return;

	switch (key)
	{
	case GLUT_KEY_F1:
		// 중력 비활성화 토글
	{
		gameWorld_->toggleGravity();
		if (gameWorld_->getGravityStatus()) {
			std::cout << "Gravity Activated" << std::endl;
		}
		else {
			std::cout << "\n=== Debug Mode Controls ===" << std::endl;
			std::cout << "←/↑/→/↓: Left / Forward / Right / Backward" << std::endl;
			std::cout << "Left Ctrl: Move Up" << std::endl;
			std::cout << "Left Shift: Move Down" << std::endl;
			std::cout << "================\n" << std::endl;
		}
	}
	break;

	case GLUT_KEY_PAGE_DOWN:
	{
		// FOV 감소
		/*bool isThirdPerson = gameWorld_->getThirdPersonView();
		if (isThirdPerson) {
			float currentRoll = camera_->getRoll();
			currentRoll += glm::radians(5.0f);
			if (currentRoll > glm::radians(120.0f)) currentRoll = glm::radians(120.0f);
			camera_->setRoll(currentRoll);
			std::cout << "FOV: " << glm::degrees(currentRoll) << " degrees" << std::endl;
		}
		else break;*/
	}
	break;

	case GLUT_KEY_PAGE_UP:
	{
		// FOV 증가
		/*bool isThirdPerson = gameWorld_->getThirdPersonView();
		if (isThirdPerson) {
			float currentRoll = camera_->getRoll();
			currentRoll -= glm::radians(5.0f);
			if (currentRoll < glm::radians(20.0f)) currentRoll = glm::radians(20.0f);
			camera_->setRoll(currentRoll);
			std::cout << "FOV: " << glm::degrees(currentRoll) << " degrees" << std::endl;
		}
		else break;*/
	}
	break;
	}
}

void InputHandler::handleGameSpecialKeys(int key)
{
	if (!gameWorld_)
		return;
	switch (key)
	{
	default: break;
	}
}