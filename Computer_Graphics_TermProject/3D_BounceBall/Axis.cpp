#include "Axis.h"

// 정적 축 데이터 정의
const VERTEX AXIS::lineVertices[6] = {
	// x축 (빨강)
	{ -0.8f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f },
	{  0.8f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f },

	// y축 (초록)
	{ 0.0f, -0.8f, 0.0f, 0.0f, 1.0f, 0.0f },
	{ 0.0f,  0.8f, 0.0f, 0.0f, 1.0f, 0.0f },

	// z축 (파랑)
	{ 0.0f, 0.0f, -0.8f, 0.0f, 0.0f, 1.0f },
	{ 0.0f, 0.0f,  0.8f, 0.0f, 0.0f, 1.0f }
};

void AXIS::init(GLuint shaderProgramID) {
	this->GetShaderProgramID(shaderProgramID);

	// 축
	glGenVertexArrays(1, &AxisVAO_);
	glGenBuffers(1, &AxisVBO_);
	glBindVertexArray(AxisVAO_);
	glBindBuffer(GL_ARRAY_BUFFER, AxisVBO_);

	glBufferData(GL_ARRAY_BUFFER, sizeof(lineVertices), lineVertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VERTEX), (GLvoid*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(VERTEX), (GLvoid*)(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void AXIS::DrawRotated(float angleDegrees) {
	// 30도 회전 행렬 생성
	glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f),
		glm::radians(angleDegrees),
		glm::vec3(0.0f, 0.0f, 1.0f)); // Z축 기준 회전

	Draw(rotationMatrix);
}

void AXIS::Draw(const glm::mat4& modelMatrix) {
	glUseProgram(shaderProgramID_);

	GLint modelLoc = glGetUniformLocation(shaderProgramID_, "modelMat");
	glm::mat4 axisModel = glm::mat4(1.0f); // 단위 행렬 (회전 없음)
	axisModel = glm::scale(axisModel, glm::vec3(4.0f, 4.0f, 4.0f)); // 크기 조절 (필요시)
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(axisModel));

	// 셰이더에 변환 행렬 전달
	GLuint modelMatrixLocation = glGetUniformLocation(shaderProgramID_, "rotateAxis");
	glUniformMatrix4fv(modelMatrixLocation, 1, GL_FALSE, glm::value_ptr(modelMatrix));

	glBindVertexArray(AxisVAO_);
	glDrawArrays(GL_LINES, 0, 6);
	glBindVertexArray(0);
}