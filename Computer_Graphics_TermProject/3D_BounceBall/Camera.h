#pragma once
#include "Headers.h"
#include "Player.h"

class Camera
{
public:
	Camera();
	~Camera();

	// 카메라 위치 및 타겟 설정
	void setPosition(const glm::vec3& pos) { position_ = pos; }
	void setTarget(const glm::vec3& target) { target_ = target; }
	void setTargetScale(float scale) { playerScale_ = scale; }
	void setYaw(float yaw) { yaw_ = yaw; }
	void setPitch(float pitch) { pitch_ = pitch; }
	void setRoll(float roll) { roll_ = roll; }

	void setEye(const glm::vec3& eye) { position_ = eye; }

	// 카메라 방향 벡터 계산
	glm::vec3 getForward() const;
	glm::vec3 getRight() const;
	glm::vec3 getUp() const;
	
	glm::mat4 getViewMatrix(bool thirdPersonView = true) const;

	void orbitAroundTarget(float distance, float yaw, float pitch, bool thirdPersonView);

	// Getter
	glm::vec3 getPosition() const { return position_; }
	glm::vec3 getTarget() const { return target_; }
	float getYaw() const { return yaw_; }
	float getPitch() const { return pitch_; }
	float getRoll() const { return roll_; }
	float getMaxPitch_3rd() const { return maxPitch_; }
	float getMaxPitch_1st() const { return maxPitchFirstPerson_; }

private:
	glm::vec3 position_;
	glm::vec3 target_;
	float yaw_;    // 좌우 회전 (라디안)
	float pitch_;  // 상하 회전 (라디안)
	float roll_;   // 기울기 (라디안)
	float distance_; // 타겟과의 거리
	float playerScale_; // 플레이어 스케일 팩터
	float maxPitch_ = glm::radians(30.0f);
	float maxPitchFirstPerson_ = glm::radians(89.0f);  // 1인칭 최대 피치
};