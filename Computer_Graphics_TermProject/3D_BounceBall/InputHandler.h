#pragma once
#include "Headers.h"
#include "GameWorld.h"
#include "Camera.h"

class InputHandler
{
public:
	InputHandler();
	~InputHandler();

	void setGameWorld(GameWorld* world) { gameWorld_ = world; }
	void setCamera(Camera* cam) { camera_ = cam; }

	void setRenderingSettings(bool* depthTest, bool* cullFace, bool* projPerspective,
		bool* drawSolid, bool* drawWireframe);
	void setLightingSettings(bool* turnLight, float* intensity);
	void setPlayerMoveSpeed(float speed) { playerMoveSpeed_ = speed; }

	void handleKeyboard(unsigned char key, int x, int y);
	void handleKeyboardUp(unsigned char key, int x, int y);
	void handleSpecialKey(int key, int x, int y);
	void handleSpecialKeyUp(int key, int x, int y);
	void updateKeyStates();

private:
	GameWorld* gameWorld_;
	Camera* camera_;

	bool* depthTest_;
	bool* cullFace_;
	bool* projectionPerspective_;
	bool* drawSolid_;
	bool* drawWireframe_;

	bool* turnOnLight_;
	float* lightIntensity_;

	float playerMoveSpeed_;

	bool keysPressed_[256] = { false };
	bool specialKeysPressed_[256] = { false };

	void handleRenderingKeys(unsigned char key);
	void handleLightingKeys(unsigned char key);
	void handleGameKeys(unsigned char key);
	void handleTrajectoryKeys(unsigned char key);
	void handleCameraSpecialKeys(int key);
	void handleGameSpecialKeys(int key);
};