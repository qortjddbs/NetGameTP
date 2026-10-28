#include "Camera.h"

Camera::Camera()
	: position_(0.0f, 5.0f, 5.0f)
	, target_(0.0f, 0.0f, 0.0f)
	, yaw_(glm::radians(-90.0f))
	, pitch_(glm::radians(0.0f))
	, roll_(glm::radians(45.0f))
	, maxPitch_(glm::radians(30.0f))
	, maxPitchFirstPerson_(glm::radians(89.0f))
	, distance_(5.0f)
	, playerScale_(0.0f)
{
}

Camera::~Camera()
{
}

glm::vec3 Camera::getForward() const
{
	glm::vec3 forward;
	forward.x = cos(yaw_);
	forward.y = 0.0f;
	forward.z = sin(yaw_);
	return glm::normalize(forward);
}

glm::vec3 Camera::getRight() const
{
	glm::vec3 forward = getForward();
	return glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));
}

glm::vec3 Camera::getUp() const
{
	return glm::vec3(0.0f, 1.0f, 0.0f);
}

glm::mat4 Camera::getViewMatrix(bool thirdPersonView) const
{
	glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

	if (thirdPersonView) {
		// 3인칭 - 구형 궤도
		glm::vec3 eye;
		glm::vec3 target;

		// 구면 좌표계를 이용한 카메라 위치 계산
		glm::vec3 offset;
		offset.x = distance_ * cos(pitch_) * cos(yaw_);
		offset.y = distance_ * sin(pitch_);
		offset.z = distance_ * cos(pitch_) * sin(yaw_);

		target = target_ + glm::vec3(0.0f, playerScale_, 0.0f);
		eye = target - offset;

		const_cast<Camera*>(this)->position_ = eye;

		return glm::lookAt(eye, target, up);
	}
	else {
		// 1인칭
		glm::vec3 eye = position_;

		glm::vec3 forward;
		forward.x = cos(pitch_) * cos(yaw_);
		forward.y = sin(pitch_);
		forward.z = cos(pitch_) * sin(yaw_);
		forward = glm::normalize(forward);

		glm::vec3 target = eye + forward;

		return glm::lookAt(eye, target, up);
	}
}

void Camera::orbitAroundTarget(float distance, float yawAngle, float pitchAngle, bool thirdPersonView)
{
	yaw_ = yawAngle;
	if (thirdPersonView) pitch_ = glm::clamp(pitchAngle, -maxPitch_, maxPitch_);
	else pitch_ = glm::clamp(pitchAngle, -maxPitchFirstPerson_, maxPitchFirstPerson_);
	distance_ = distance;

	// 3인칭 모드에서 카메라 위치 업데이트
	if (thirdPersonView) {
		// 구면 좌표계를 이용한 카메라 위치 계산
		glm::vec3 offset;
		offset.x = distance_ * cos(pitch_) * cos(yaw_);
		offset.y = distance_ * sin(pitch_);
		offset.z = distance_ * cos(pitch_) * sin(yaw_);

		glm::vec3 target = target_ + glm::vec3(0.0f, playerScale_, 0.0f);
		position_ = target - offset;
	}
}