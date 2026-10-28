#pragma once
#include <gl/glew.h>
#include <gl/freeglut.h>
#include <gl/freeglut_ext.h>
#include <gl/glm/glm.hpp>
#include <gl/glm/ext.hpp>
#include <gl/glm/gtc/matrix_transform.hpp>
#include "Vertex.h"

class AXIS
{
public:
	void init(GLuint shaderProgramID);
	void GetShaderProgramID (GLuint shaderProgramID) { shaderProgramID_ = shaderProgramID; }

	void DrawRotated(float angleDegrees);
	void Draw(const glm::mat4& modelMatrix = glm::mat4(1.0f));

private:
	GLuint AxisVBO_, AxisVAO_;
	GLuint shaderProgramID_;
	static const VERTEX lineVertices[6];  // 축 정점 데이터
	glm::mat4 rotationMatrix;             // 회전 변환 행렬
};
