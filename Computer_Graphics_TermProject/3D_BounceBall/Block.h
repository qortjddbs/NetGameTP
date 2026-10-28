#pragma once
#include "ParentModel.h"

// 기본 블럭 클래스
class BLOCK : public ParentModel
{
public:
	CollisionType getCollisionType() const override;
	void onCollision(ParentModel* other) override;
};

// 튕기는 블럭
class BOUNCE_BLOCK : public BLOCK
{
public:
	CollisionType getCollisionType() const override;
};

// 부서지는 블럭
class BREAKABLE_BLOCK : public BLOCK
{
public:
	bool isBroken_ = false;

	CollisionType getCollisionType() const override;
	void onCollision(ParentModel* other) override;
	void reset() override;  // 리셋 시 복구
};

// 가시 블럭
class SPIKE_BLOCK : public BLOCK
{
public:
	CollisionType getCollisionType() const override;
};

// 화살표 블록
class ARROW_BLOCK : public BLOCK
{
public:
	glm::vec3 arrowDirection_ = glm::vec3(0.0f, 0.0f, -1.0f); // 화살표 방향
	float launchSpeed_ = 10.0f; // 발사 속도

	CollisionType getCollisionType() const override;

	// 화살표 방향 설정 (단위 벡터로 자동 정규화)
	void setArrowDirection(const glm::vec3& dir) {
		if (glm::length(dir) > 0.001f) {
			arrowDirection_ = glm::normalize(dir);
		}
	}

	glm::vec3 getArrowDirection() const { return arrowDirection_; }
	float getLaunchSpeed() const { return launchSpeed_; }
};

// 별
class STAR : public ParentModel
{
public:
	bool isCollected_ = false;

	CollisionType getCollisionType() const override;
	void onCollision(ParentModel* other) override;
	void reset() override;  // 리셋 시 복구

	Shape getShape() const override {
		return Shape::SPHERE;  // 구체로 충돌 처리
	}

	float getRadius() const override {
		return 0.6f;  // 별 크기에 맞게 조정
	}
};