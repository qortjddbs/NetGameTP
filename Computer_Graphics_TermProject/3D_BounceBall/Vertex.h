#pragma once
#include <gl/glew.h>
#include <gl/freeglut.h>
#include <gl/freeglut_ext.h>
#include <gl/glm/glm.hpp>
#include <gl/glm/ext.hpp>
#include <gl/glm/gtc/matrix_transform.hpp>
#include <vector>

struct VERTEX {
	GLfloat x, y, z;
	GLfloat r, g, b;
	GLfloat nx, ny, nz;
	GLfloat u, v;
};

struct FACE {
	std::vector<VERTEX> vertices;
};