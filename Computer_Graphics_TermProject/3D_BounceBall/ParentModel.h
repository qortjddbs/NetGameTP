#pragma once
#include "Headers.h"
#include "Vertex.h"
#include "ObjModel.h"

// 충돌 타입
enum class CollisionType {
	NONE,
	NORMAL_BLOCK,    // 일반 블럭
	BOUNCE_BLOCK,    // 튕기는 블럭
	BREAKABLE_BLOCK, // 부서지는 블럭
	SPIKE_BLOCK,      // 가시 블럭
	ARROW_BLOCK,   // 화살표 블럭
	STAR			// 별
};

enum class Shape {
	SPHERE,
	AABB
};

class ParentModel
{
public:
	glm::mat4 createModelMatrix();
	void init(const char* filename, GLuint shaderProgramID,  float r, float g, float b);
	void loadTexture(const char* filepath) { m_.loadTextureFromFile(filepath); }
	void draw();
	virtual void reset();

	// 바운딩 정보
	virtual glm::vec3 getPosition() const { return translation_; }
	virtual glm::vec3 getScale() const { return selfscale_ * scale_; }

	// 충돌 형태 반환 (자식 클래스에서 오버라이드)
	virtual Shape getShape() const { return Shape::AABB; }

	// 구체 반지름 (Sphere 타입일 경우)
	virtual float getRadius() const { return 0.5f; }

	// 충돌 검사
	bool checkCollision(const ParentModel* other) const;

	// 충돌 시 호출되는 가상 함수 (자식 클래스에서 오버라이드)
	virtual void onCollision(ParentModel* other) {}

	// 충돌 타입 반환 (자식 클래스에서 오버라이드)
	virtual CollisionType getCollisionType() const { return CollisionType::NONE; }

	// Getter
	glm::vec3 getTranslation() const { return translation_; }
	glm::vec3 getRotation() const { return rotation_; }
	glm::vec3 getOrbitRotation() const { return orbitRotation_; }
	glm::vec3 getSelfScale() const { return selfscale_; }
	glm::vec3 getGlobalScale() const { return scale_; }

	// Setter
	void setTranslation(const glm::vec3& pos) { translation_ = pos; }
	void setRotation(const glm::vec3& rot) { rotation_ = rot; }
	void setOrbitRotation(const glm::vec3& orbit) { orbitRotation_ = orbit; }
	void setSelfScale(const glm::vec3& s) { selfscale_ = s; }
	void setGlobalScale(const glm::vec3& s) { scale_ = s; }

	// 편의
	void translate(const glm::vec3& offset) { translation_ += offset; }
	void rotate(const glm::vec3& rot) { rotation_ += rot; }
	void scaleBy(const glm::vec3& factor) { selfscale_ *= factor; }

protected:
	OBJ_MODEL m_;
	glm::vec3 translation_ = glm::vec3(0.0f, 0.0f, 0.0f);    // 이동
	glm::vec3 rotation_ = glm::vec3(0.0f, 0.0f, 0.0f);		// 자전
	glm::vec3 orbitRotation_ = glm::vec3(0.0f, 0.0f, 0.0f);  // 공전
	glm::vec3 selfscale_ = glm::vec3(1.0f, 1.0f, 1.0f);		// 제자리 스케일
	glm::vec3 scale_ = glm::vec3(1.0f, 1.0f, 1.0f);			// 원점 스케일

private:
	static bool checkSphereAABBCollision(const ParentModel* sphere, const ParentModel* aabb);
	static bool checkSphereSphereCollision(const ParentModel* sphere1, const ParentModel* sphere2);
	static bool checkAABBCollision(const ParentModel* aabb1, const ParentModel* aabb2);
};

