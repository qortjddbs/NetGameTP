#include "Block.h"

CollisionType BLOCK::getCollisionType() const {
	return CollisionType::NORMAL_BLOCK;
}

void BLOCK::onCollision(ParentModel* other) {
	// 블럭은 충돌 시 특별한 행동 없음
}

CollisionType BOUNCE_BLOCK::getCollisionType() const {
	return CollisionType::BOUNCE_BLOCK;
}

CollisionType BREAKABLE_BLOCK::getCollisionType() const {
	return CollisionType::BREAKABLE_BLOCK;
}

void BREAKABLE_BLOCK::onCollision(ParentModel* other) {
	isBroken_ = true;
}

void BREAKABLE_BLOCK::reset() {
	ParentModel::reset();  // 부모 클래스의 reset 호출
	isBroken_ = false;     // 부서진 상태 초기화
}

CollisionType SPIKE_BLOCK::getCollisionType() const {
	return CollisionType::SPIKE_BLOCK;
}

CollisionType ARROW_BLOCK::getCollisionType() const {
	return CollisionType::ARROW_BLOCK;
}

CollisionType STAR::getCollisionType() const {
	return CollisionType::STAR;  // 새로운 타입 추가 필요
}

void STAR::onCollision(ParentModel* other) {
	isCollected_ = true;
}

void STAR::reset() {
	ParentModel::reset();  // 부모 클래스의 reset 호출
	isCollected_ = false;  // 수집 상태 초기화
}