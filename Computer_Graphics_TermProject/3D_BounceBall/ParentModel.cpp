#include "ParentModel.h"

glm::mat4 ParentModel::createModelMatrix() {
	glm::mat4 modelMatrix = glm::mat4(1.0f);

	// 원점 기준 스케일링
	modelMatrix = glm::scale(modelMatrix, scale_);

	// 공전 관련
	modelMatrix = glm::rotate(modelMatrix, glm::radians(orbitRotation_.x), glm::vec3(1.0f, 0.0f, 0.0f));
	modelMatrix = glm::rotate(modelMatrix, glm::radians(orbitRotation_.y), glm::vec3(0.0f, 1.0f, 0.0f));
	modelMatrix = glm::rotate(modelMatrix, glm::radians(orbitRotation_.z), glm::vec3(0.0f, 0.0f, 1.0f));

	// 이동
	modelMatrix = glm::translate(modelMatrix, translation_);

	// 자전 관련
	modelMatrix = glm::rotate(modelMatrix, glm::radians(rotation_.x), glm::vec3(1.0f, 0.0f, 0.0f));
	modelMatrix = glm::rotate(modelMatrix, glm::radians(rotation_.y), glm::vec3(0.0f, 1.0f, 0.0f));
	modelMatrix = glm::rotate(modelMatrix, glm::radians(rotation_.z), glm::vec3(0.0f, 0.0f, 1.0f));

	// 제자리 스케일링
	modelMatrix = glm::scale(modelMatrix, selfscale_);

	return modelMatrix;
}

void ParentModel::init(const char* filename, GLuint shaderProgramID, float r, float g, float b) {
	m_.init(shaderProgramID);
	m_.loadFromFile(filename, r, g, b);
}

void ParentModel::draw() {
	glm::mat4 modelMatrix = createModelMatrix();
	m_.draw(modelMatrix);
}

void ParentModel::reset() {
	translation_ = glm::vec3(0.0f, 0.0f, 0.0f);
	rotation_ = glm::vec3(0.0f, 0.0f, 0.0f);
	orbitRotation_ = glm::vec3(0.0f, 0.0f, 0.0f);
	selfscale_ = glm::vec3(1.0f, 1.0f, 1.0f);
	scale_ = glm::vec3(1.0f, 1.0f, 1.0f);
}

bool ParentModel::checkCollision(const ParentModel* other) const {
	Shape shape1 = this->getShape();
	Shape shape2 = other->getShape();

	// Sphere vs AABB
	if (shape1 == Shape::SPHERE && shape2 == Shape::AABB) {
		return checkSphereAABBCollision(this, other);
	}
	// AABB vs Sphere
	else if (shape1 == Shape::AABB && shape2 == Shape::SPHERE) {
		return checkSphereAABBCollision(other, this);
	}
	// Sphere vs Sphere
	else if (shape1 == Shape::SPHERE && shape2 == Shape::SPHERE) {
		return checkSphereSphereCollision(this, other);
	}
	// AABB vs AABB
	else {
		return checkAABBCollision(this, other);
	}
}

bool ParentModel::checkSphereAABBCollision(const ParentModel* sphere, const ParentModel* aabb) {
	glm::vec3 spherePos = sphere->getPosition();
	float radius = sphere->getRadius();

	glm::vec3 aabbPos = aabb->getPosition();
	glm::vec3 aabbScale = aabb->getScale();

	glm::vec3 aabbMin = aabbPos - aabbScale * 0.5f;
	glm::vec3 aabbMax = aabbPos + aabbScale * 0.5f;

	glm::vec3 closestPoint = glm::clamp(spherePos, aabbMin, aabbMax);

	float distance = glm::length(closestPoint - spherePos);
	return distance < radius;
}

bool ParentModel::checkSphereSphereCollision(const ParentModel* sphere1, const ParentModel* sphere2) {
	glm::vec3 pos1 = sphere1->getPosition();
	glm::vec3 pos2 = sphere2->getPosition();
	float radius1 = sphere1->getRadius();
	float radius2 = sphere2->getRadius();

	float distance = glm::length(pos1 - pos2);
	return distance < (radius1 + radius2);
}

bool ParentModel::checkAABBCollision(const ParentModel* aabb1, const ParentModel* aabb2) {
	glm::vec3 pos1 = aabb1->getPosition();
	glm::vec3 scale1 = aabb1->getScale();
	glm::vec3 pos2 = aabb2->getPosition();
	glm::vec3 scale2 = aabb2->getScale();

	return (abs(pos1.x - pos2.x) < (scale1.x + scale2.x) / 2.0f) &&
		(abs(pos1.y - pos2.y) < (scale1.y + scale2.y) / 2.0f) &&
		(abs(pos1.z - pos2.z) < (scale1.z + scale2.z) / 2.0f);
}