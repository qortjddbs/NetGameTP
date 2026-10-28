#pragma once
#include "Headers.h"
#include "Vertex.h"
#include <vector>

// 전방 선언
class ParentModel;

class TrajectoryPredictor
{
public:
	void init(GLuint shaderProgramID);
	void draw(const glm::vec3& startPos, const glm::vec3& velocity,
		const std::vector<ParentModel*>& blocks, int numPoints = 50, float timeStep = 0.05f);
	void cleanup();

private:
	GLuint trajectoryVAO_, trajectoryVBO_;
	GLuint circleVAO_, circleVBO_;
	GLuint shaderProgramID_;
	std::vector<VERTEX> trajectoryPoints_;
	std::vector<VERTEX> circleVertices_;

	// 궤적 계산 (중력 적용)
	void calculateTrajectory(const glm::vec3& startPos, const glm::vec3& velocity, int numPoints, float timeStep);

	// 블록 위 착지 지점 계산 (블록이 있을 경우에만)
	bool calculateLandingPointOnBlock(const glm::vec3& startPos, const glm::vec3& velocity,
		const std::vector<ParentModel*>& blocks, glm::vec3& outLandingPoint);

	// 점이 블록 위에 있는지 체크
	bool isPointOnBlock(const glm::vec3& point, ParentModel* block, float tolerance = 0.2f);

	// 원 그리기 (착지 지점 표시)
	void drawCircle(const glm::vec3& center, float radius, const glm::vec3& color);

	// 원 버텍스 생성
	void createCircleVertices(float radius, int segments = 36);
};