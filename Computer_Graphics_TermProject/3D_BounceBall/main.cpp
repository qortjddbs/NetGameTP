#define _CRT_SECURE_NO_WARNINGS
#define STB_IMAGE_IMPLEMENTATION
#include "Headers.h"
#include "Vertex.h"
#include "Axis.h"
#include "ObjModel.h"
#include "GameWorld.h"
#include "InputHandler.h"
#include "Camera.h"

std::mt19937 rd(std::random_device{}());
std::uniform_real_distribution<float> speed(1.0f, 2.5f);
std::uniform_real_distribution<float> Fspeed(0.01f, 0.07f);

#define WINDOW_WIDTH 1200
#define WINDOW_HEIGHT 800

// === 게임 객체 ===
GameWorld* gameWorld = nullptr;
InputHandler* inputHandler = nullptr;
Camera* camera = nullptr;

// === 게임 시간 관리 ===
float deltaTime = 0.0f;
int lastTime = 0;

GLuint VBO, VAO;
AXIS Axis;

// 렌더링 설정
bool DepthTest = true;
bool CullFace = false;
bool ProjectionPerspective = true;
bool DrawSolid = true;
bool DrawWireframe = false;
bool useTexture = false;

float bgR = 0.6f, bgG = 0.6f, bgB = 0.6f;

GLfloat transformx(int x) { return ((float)x / (WINDOW_WIDTH / 2)) - 1.0f; }
GLfloat transformy(int y) { return ((WINDOW_HEIGHT - (float)y) / (WINDOW_HEIGHT / 2)) - 1.0f; }

void make_vertexShaders();
void make_fragmentShaders();
GLuint make_shaderProgram();
GLvoid drawScene();
GLvoid Reshape(int w, int h);
GLvoid KeyBoardDown(unsigned char key, int x, int y);
GLvoid KeyBoardUp(unsigned char key, int x, int y);
GLvoid SpecialKey(int key, int x, int y);
GLvoid SpecialKeyUp(int key, int x, int y);
GLvoid Mouse(int button, int state, int x, int y);
GLvoid Move(int x, int y);
GLvoid Idle();
void init();

GLuint shaderProgramID;
GLuint vertexShader;
GLuint fragmentShader;

// 조명 설정
bool turnOnLight = true;
glm::vec3 lightPos = glm::vec3(-50.0f, 100.0f, -70.0f);
glm::vec3 lightColor = glm::vec3(1.0f, 1.0f, 1.0f);
float lightIntensity = 0.5f;

float lightOrbitAngle = 0.0f;
float lightOrbitRadius = 5.0f;
bool lightOrbitCW = false;
bool lightOrbitCCW = false;
float lightOrbitSpeed = 1.0f;

float cameraYaw = 0.0f;
float cameraPitch = 0.0f;

int lastMouseX = WINDOW_WIDTH / 2;
int lastMouseY = WINDOW_HEIGHT / 2;
bool firstMouse = true;
bool leftMousePressed = false;
float mouseSensitivity = 0.003f;

char* filetobuf(const char* file)
{
	FILE* fptr;
	long length;
	char* buf;
	fptr = fopen(file, "rb");
	if (!fptr)
		return NULL;
	fseek(fptr, 0, SEEK_END);
	length = ftell(fptr);
	buf = (char*)malloc(length + 1);
	fseek(fptr, 0, SEEK_SET);
	fread(buf, length, 1, fptr);
	fclose(fptr);
	buf[length] = 0;
	return buf;
}

void main(int argc, char** argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
	glutInitWindowPosition(100, 100);
	glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
	glutCreateWindow("3D Bounce Ball Game");

	glewExperimental = GL_TRUE;
	glewInit();

	make_vertexShaders();
	make_fragmentShaders();
	shaderProgramID = make_shaderProgram();

	init();

	// Camera 생성
	camera = new Camera();
	camera->setRoll(glm::radians(45.0f));
	camera->setTarget(glm::vec3(0.0f, 0.0f, 0.0f));
	cameraYaw = camera->getYaw();
	cameraPitch = camera->getPitch();
	float dis = glm::length(camera->getPosition() - camera->getTarget());
	camera->orbitAroundTarget(dis, cameraYaw, cameraPitch, true);

	// GameWorld 생성
	gameWorld = new GameWorld(shaderProgramID);
	gameWorld->initialize();

	// InputHandler 생성 및 설정
	inputHandler = new InputHandler();
	inputHandler->setGameWorld(gameWorld);
	inputHandler->setCamera(camera);  // 카메라 설정
	inputHandler->setRenderingSettings(&DepthTest, &CullFace, &ProjectionPerspective, &DrawSolid, &DrawWireframe);
	inputHandler->setLightingSettings(&turnOnLight, &lightIntensity);
	inputHandler->setPlayerMoveSpeed(gameWorld->getPlayer()->getSpeed());

	std::cout << "\n=== Controls ===" << std::endl;
	std::cout << "W/A/S/D: Move player" << std::endl;
	std::cout << "C: Start/Pause game" << std::endl;
	std::cout << "X: Reset game" << std::endl;
	std::cout << "V: Toggle 1st/3rd Person View" << std::endl;
	std::cout << "H: Toggle Depth Test" << std::endl;
	std::cout << "R: Toggle Lighting" << std::endl;
	std::cout << "+/-: Adjust Light Intensity" << std::endl;
	std::cout << "T: Trace Path" << std::endl;
	std::cout << "Q: Quit" << std::endl;
	std::cout << "================\n" << std::endl;

	glutDisplayFunc(drawScene);
	glutReshapeFunc(Reshape);
	glutKeyboardFunc(KeyBoardDown);
	glutKeyboardUpFunc(KeyBoardUp);
	glutSpecialFunc(SpecialKey);        // 스페셜 키 추가
	glutSpecialUpFunc(SpecialKeyUp);    // 스페셜 키 Up 추가
	glutMouseFunc(Mouse);
	glutMotionFunc(Move);
	glutIdleFunc(Idle);

	// 종료 시 정리
	atexit([]() {
		std::cout << "Cleaning up..." << std::endl;
		if (gameWorld) {
			delete gameWorld;
			gameWorld = nullptr;
		}
		if (inputHandler) {
			delete inputHandler;
			inputHandler = nullptr;
		}
		if (camera) {
			delete camera;
			camera = nullptr;
		}
		});

	glutMainLoop();
}

void make_vertexShaders()
{
	GLchar* vertexSource;
	vertexSource = filetobuf("Phong_vertex.glsl");
	vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexSource, NULL);
	glCompileShader(vertexShader);
	GLint result;
	GLchar errorLog[512];
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &result);
	if (!result)
	{
		glGetShaderInfoLog(vertexShader, 512, NULL, errorLog);
		std::cerr << "ERROR: vertex shader 컴파일 실패\n" << errorLog << std::endl;
		return;
	}
}

void make_fragmentShaders()
{
	GLchar* fragmentSource;
	fragmentSource = filetobuf("Phong_fragment.glsl");
	fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
	glCompileShader(fragmentShader);
	GLint result;
	GLchar errorLog[512];
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &result);
	if (!result)
	{
		glGetShaderInfoLog(fragmentShader, 512, NULL, errorLog);
		std::cerr << "ERROR: frag_shader 컴파일 실패\n" << errorLog << std::endl;
		return;
	}
}

GLuint make_shaderProgram()
{
	GLint result;
	GLchar* errorLog = NULL;
	GLuint shaderID;
	shaderID = glCreateProgram();
	glAttachShader(shaderID, vertexShader);
	glAttachShader(shaderID, fragmentShader);
	glLinkProgram(shaderID);
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);
	glGetProgramiv(shaderID, GL_LINK_STATUS, &result);
	if (!result) {
		glGetProgramInfoLog(shaderID, 512, NULL, errorLog);
		std::cerr << "ERROR: shader program 연결 실패\n" << errorLog << std::endl;
		return false;
	}
	glUseProgram(shaderID);
	return shaderID;
}

void init()
{
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);

	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(VERTEX), (GLvoid*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(VERTEX), (GLvoid*)(2 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	glUseProgram(shaderProgramID);
	unsigned int lightPosLocation = glGetUniformLocation(shaderProgramID, "lightPos");
	glUniform3f(lightPosLocation, lightPos.x, lightPos.y, lightPos.z);
	unsigned int lightColorLocation = glGetUniformLocation(shaderProgramID, "lightColor");
	glUniform3f(lightColorLocation, lightColor.r, lightColor.g, lightColor.b);
	unsigned int objColorLocation = glGetUniformLocation(shaderProgramID, "objectColor");
	glUniform3f(objColorLocation, 1.0, 0.5, 0.3);

	lightOrbitRadius = sqrt(lightPos.x * lightPos.x + lightPos.z * lightPos.z);
	lightOrbitAngle = glm::degrees(atan2(lightPos.z, lightPos.x));
	if (lightOrbitAngle < 0.0f) lightOrbitAngle += 360.0f;

	Axis.init(shaderProgramID);
	lastTime = glutGet(GLUT_ELAPSED_TIME);
}

GLvoid drawScene()
{
	glUseProgram(shaderProgramID);

	if (DepthTest) glEnable(GL_DEPTH_TEST);
	else glDisable(GL_DEPTH_TEST);

	if (CullFace) glEnable(GL_CULL_FACE);
	else glDisable(GL_CULL_FACE);

	glClearColor(bgR, bgG, bgB, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// 텍스처 유니폼 전달
	GLint useTextureLoc = glGetUniformLocation(shaderProgramID, "useTexture");
	glUniform1i(useTextureLoc, useTexture);

	// 조명 유니폼 전달
	GLint turnOnLightLoc = glGetUniformLocation(shaderProgramID, "turnOnLight");
	glUniform1i(turnOnLightLoc, turnOnLight);

	GLfloat lightIntensityLoc = glGetUniformLocation(shaderProgramID, "lightIntensity");
	glUniform1f(lightIntensityLoc, lightIntensity);

	GLint lightPosLoc = glGetUniformLocation(shaderProgramID, "lightPos");
	GLint lightColorLoc = glGetUniformLocation(shaderProgramID, "lightColor");
	GLint viewPosLoc = glGetUniformLocation(shaderProgramID, "viewPos");

	glUniform3fv(lightPosLoc, 1, glm::value_ptr(lightPos));
	glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));
	glUniform3fv(viewPosLoc, 1, glm::value_ptr(camera->getPosition()));

	// View 및 Projection 매트릭스
	GLint viewLoc = glGetUniformLocation(shaderProgramID, "view");
	GLint projLoc = glGetUniformLocation(shaderProgramID, "proj");


	bool ThirdPersonView = gameWorld->getThirdPersonView();
	if (gameWorld && camera) {
		glm::vec3 playerPos = gameWorld->getPlayer()->getTranslation();
		camera->setTarget(playerPos);
		camera->setTargetScale(gameWorld->getPlayer()->getScaleFactor());
		camera->orbitAroundTarget(5.0f, cameraYaw, cameraPitch, ThirdPersonView);
	}

	// View 매트릭스 가져오기
	if (!ThirdPersonView) {
		camera->setPosition(gameWorld->getPlayer()->getEyePosition());
	}
	glm::mat4 view = camera->getViewMatrix(ThirdPersonView);
	glm::mat4 proj = glm::perspective(camera->getRoll(), (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT, 0.1f, 100.0f);

	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(proj));


	// 축 그리기
	GLint modelLoc = glGetUniformLocation(shaderProgramID, "modelMat");
	glm::mat4 axisModel = glm::mat4(1.0f);
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(axisModel));
	// Axis.Draw();

	// GameWorld 그리기
	if (gameWorld) {
		gameWorld->draw();
	}

	glutSwapBuffers();
}

GLvoid Reshape(int w, int h)
{
	glViewport(0, 0, w, h);
}

GLvoid KeyBoardDown(unsigned char key, int x, int y)
{
	if (inputHandler) {
		inputHandler->handleKeyboard(key, x, y);
	}
	glutPostRedisplay();
}

GLvoid KeyBoardUp(unsigned char key, int x, int y)
{
	if (inputHandler) {
		inputHandler->handleKeyboardUp(key, x, y);
	}
	glutPostRedisplay();
}

// 스페셜 키 콜백 함수 구현 (main.cpp 하단에 추가)
GLvoid SpecialKey(int key, int x, int y) {
	if (inputHandler) {
		inputHandler->handleSpecialKey(key, x, y);
	}
	glutPostRedisplay();
}

GLvoid SpecialKeyUp(int key, int x, int y) {
	if (inputHandler) {
		inputHandler->handleSpecialKeyUp(key, x, y);
	}
	glutPostRedisplay();
}

GLvoid Mouse(int button, int state, int x, int y) {
	if (button == GLUT_LEFT_BUTTON) {
		if (state == GLUT_DOWN) {
			leftMousePressed = true;
			lastMouseX = x;
			lastMouseY = y;
		}
		else if (state == GLUT_UP) {
			leftMousePressed = false;
		}
	}

	glutPostRedisplay();
}

GLvoid Move(int x, int y) {
	if (leftMousePressed) {
		int deltaX = x - lastMouseX;
		int deltaY = y - lastMouseY;

		lastMouseX = x;
		lastMouseY = y;

		cameraYaw += deltaX * mouseSensitivity;
		cameraPitch -= deltaY * mouseSensitivity;

		float maxPitch = 0.0f;
		if (gameWorld->getThirdPersonView()) {
			maxPitch = camera->getMaxPitch_3rd();
		}
		else {
			maxPitch = camera->getMaxPitch_1st();
		}
		if (cameraPitch > maxPitch) cameraPitch = maxPitch;
		if (cameraPitch < -maxPitch) cameraPitch = -maxPitch;
	}

	glutPostRedisplay();
}

GLvoid Idle()
{
	static int lastFrameTime = 0;
	const int targetFPS = 60;
	const int frameDelay = 1000 / targetFPS;

	int currentTime = glutGet(GLUT_ELAPSED_TIME);

	if (currentTime - lastFrameTime >= frameDelay) {
		deltaTime = (currentTime - lastTime) / 1000.0f;
		lastTime = currentTime;
		lastFrameTime = currentTime;

		if (gameWorld) {
			inputHandler->updateKeyStates();
			gameWorld->update(deltaTime);
		}

		glutPostRedisplay();
	}
}