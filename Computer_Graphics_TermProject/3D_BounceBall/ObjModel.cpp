#include "ObjModel.h"
#include <iostream>

void OBJ_MODEL::init(GLuint shaderProgramID) {
    shaderProgramID_ = shaderProgramID;

    // 메모리 초기화
    objModel_.vertices = nullptr;
    objModel_.faces = nullptr;
    objModel_.vertex_count = 0;
    objModel_.face_count = 0;

    std::cout << "OBJ Model initialized" << std::endl;
}

void OBJ_MODEL::loadFromFile(const char* filename) {
    // 랜덤 색상 사용 (기본값)
    color_ = glm::vec3(-1.0f, -1.0f, -1.0f);
    
    // 1. OBJ 파일 읽기
    read_obj_file(filename, &objModel_);

    if (objModel_.vertex_count == 0) {
        std::cerr << "Failed to load OBJ file: " << filename << std::endl;
        return;
    }

    std::cout << "OBJ file loaded successfully: " << filename << std::endl;
    std::cout << "Vertices: " << objModel_.vertex_count << ", Faces: " << objModel_.face_count << std::endl;

    // 2. OpenGL 버텍스 배열로 변환
    convertToVertexArray();

    // 3. OpenGL 설정
    setupOpenGL();
}

void OBJ_MODEL::loadFromFile(const char* filename, float r, float g, float b) {
    // 지정된 색상 사용
    color_ = glm::vec3(r, g, b);
    
    // 1. OBJ 파일 읽기
    read_obj_file(filename, &objModel_);

    if (objModel_.vertex_count == 0) {
        std::cerr << "Failed to load OBJ file: " << filename << std::endl;
        return;
    }

    std::cout << "OBJ file loaded successfully: " << filename << " with color RGB(" 
              << r << ", " << g << ", " << b << ")" << std::endl;
    std::cout << "Vertices: " << objModel_.vertex_count << ", Faces: " << objModel_.face_count << std::endl;

    // 2. OpenGL 버텍스 배열로 변환
    convertToVertexArray();

    // 3. OpenGL 설정
    setupOpenGL();
}

void OBJ_MODEL::convertToVertexArray() {
    vertices_.clear();

    for (size_t i = 0; i < objModel_.face_count; ++i) {
        Face& face = objModel_.faces[i];

        // 삼각형의 세 정점 가져오기
        glm::vec3 v0(objModel_.vertices[face.v1].x, objModel_.vertices[face.v1].y, objModel_.vertices[face.v1].z);
        glm::vec3 v1(objModel_.vertices[face.v2].x, objModel_.vertices[face.v2].y, objModel_.vertices[face.v2].z);
        glm::vec3 v2(objModel_.vertices[face.v3].x, objModel_.vertices[face.v3].y, objModel_.vertices[face.v3].z);

        // 페이스 노말 계산 (외적)
        glm::vec3 edge1 = v1 - v0;
        glm::vec3 edge2 = v2 - v0;
        glm::vec3 normal = glm::normalize(glm::cross(edge1, edge2));

        for (int j = 0; j < 3; ++j) {
            unsigned int vertexIndex; // 위치 인덱스
            unsigned int texIndex;    // 텍스처 인덱스
            if (j == 0) {
                vertexIndex = face.v1;
                texIndex = face.t1;
            }
            else if (j == 1) {
                vertexIndex = face.v2;
                texIndex = face.t2;
            }
            else {
                vertexIndex = face.v3;
                texIndex = face.t3;
            }

            if (vertexIndex < objModel_.vertex_count) {
                VERTEX vertex;
                vertex.x = objModel_.vertices[vertexIndex].x;
                vertex.y = objModel_.vertices[vertexIndex].y;
                vertex.z = objModel_.vertices[vertexIndex].z;
                
                // 색상 설정: color_가 (-1, -1, -1)이면 랜덤, 아니면 지정된 색상 사용
                if (color_.x < 0.0f) {
                    // 랜덤 색상
                    vertex.r = static_cast <float>(rand()) / static_cast <float>(RAND_MAX);
                    vertex.g = static_cast <float>(rand()) / static_cast <float>(RAND_MAX);
                    vertex.b = static_cast <float>(rand()) / static_cast <float>(RAND_MAX);
                } else {
                    // 지정된 색상
                    vertex.r = color_.x;
                    vertex.g = color_.y;
                    vertex.b = color_.z;
                }

                // 노말 추가
                vertex.nx = normal.x;
                vertex.ny = normal.y;
                vertex.nz = normal.z;

                // 4. 텍스처 좌표 설정
                // 텍스처 데이터가 존재하고, 인덱스가 유효한 경우
                if (objModel_.texCoords != nullptr && texIndex < objModel_.texCoord_count) {
                    vertex.u = objModel_.texCoords[texIndex].u;
                    vertex.v = objModel_.texCoords[texIndex].v;
                }
                else {
                    // 텍스처 좌표가 없는 경우 기본값
                    vertex.u = 0.0f;
                    vertex.v = 0.0f;
                }

                vertices_.push_back(vertex);
            }
        }
    }

    std::cout << "Converted to vertex array. Total vertices: " << vertices_.size() << std::endl;
}

void OBJ_MODEL::setupOpenGL() {
    if (VAO_ != 0) {
        glDeleteVertexArrays(1, &VAO_);
    }
    if (VBO_ != 0) {
        glDeleteBuffers(1, &VBO_);
    }

    glGenVertexArrays(1, &VAO_);
    glGenBuffers(1, &VBO_);

    glBindVertexArray(VAO_);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_);

    glBufferData(GL_ARRAY_BUFFER, vertices_.size() * sizeof(VERTEX), vertices_.data(), GL_STATIC_DRAW);

    // 위치 (location = 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VERTEX), (GLvoid*)0);
    glEnableVertexAttribArray(0);

    // 색상 (location = 1)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(VERTEX), (GLvoid*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    // 노멀 (location = 2)
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(VERTEX), (GLvoid*)(6 * sizeof(GLfloat)));
    glEnableVertexAttribArray(2);

    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(VERTEX), (GLvoid*)(9 * sizeof(GLfloat)));
    glEnableVertexAttribArray(3);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    std::cout << "OpenGL setup completed. VAO: " << VAO_ << ", VBO: " << VBO_ << std::endl;
}

void OBJ_MODEL::draw(const glm::mat4& modelMatrix) {
    if (vertices_.empty()) {
        std::cerr << "No vertices to draw!" << std::endl;
        return;
    }

    // 외부에서 받은 변환 행렬을 셰이더에 전달
    GLint modelLoc = glGetUniformLocation(shaderProgramID_, "modelMat");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));

    // 렌더링
    glBindVertexArray(VAO_);

    // 텍스처 바인딩
    if (textureID_ > 0) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureID_);
        glUniform1i(glGetUniformLocation(shaderProgramID_, "texture1"), 0);
        glUniform1i(glGetUniformLocation(shaderProgramID_, "useTexture"), true);
    }
    else {
        glUniform1i(glGetUniformLocation(shaderProgramID_, "useTexture"), false);
    }
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices_.size()));
    glBindVertexArray(0);
}

void OBJ_MODEL::draw() {
    if (vertices_.empty()) {
        std::cerr << "No vertices to draw!" << std::endl;
        return;
    }

    // 렌더링
    glBindVertexArray(VAO_);

    // 텍스처 바인딩
    if (textureID_ > 0) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureID_);
        glUniform1i(glGetUniformLocation(shaderProgramID_, "texture1"), 0);
        glUniform1i(glGetUniformLocation(shaderProgramID_, "useTexture"), true);
    }
    else {
        glUniform1i(glGetUniformLocation(shaderProgramID_, "useTexture"), false);
    }
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices_.size()));
    glBindVertexArray(0);
}

void OBJ_MODEL::printModelInfo() const {
    std::cout << "=== OBJ Model Info ===" << std::endl;
    std::cout << "Vertex count: " << objModel_.vertex_count << std::endl;
    std::cout << "Face count: " << objModel_.face_count << std::endl;
    std::cout << "Rendered vertices: " << vertices_.size() << std::endl;
}

void OBJ_MODEL::cleanup() {
    if (objModel_.vertices) {
        free(objModel_.vertices);
        objModel_.vertices = nullptr;
    }
    if (objModel_.texCoords) {
        free(objModel_.texCoords);
        objModel_.texCoords = nullptr;
    }
    if (objModel_.faces) {
        free(objModel_.faces);
        objModel_.faces = nullptr;
    }

    if (VAO_ != 0) {
        glDeleteVertexArrays(1, &VAO_);
        VAO_ = 0;
    }
    if (VBO_ != 0) {
        glDeleteBuffers(1, &VBO_);
        VBO_ = 0;
    }
}

void OBJ_MODEL::setTextureID(GLuint id) {
    // 외부에서 이미 생성된 텍스처 ID를 할당
    textureID_ = id;
}

GLuint OBJ_MODEL::getTextureID() const {
    return textureID_;
}

bool OBJ_MODEL::loadTextureFromFile(const char* filepath) {
    int width = 0, height  = 0, channels = 0;
    // OpenGL 텍스처 좌표와 이미지 파일의 y축이 반대인 경우 flip
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(filepath, &width, &height, &channels, 0);
    if (!data) {
        std::cerr << "Failed to load texture image: " << filepath << std::endl;
        return false;
    }

    GLenum format = GL_RGB;
    if (channels == 1) format = GL_RED;
    else if (channels == 3) format = GL_RGB;
    else if (channels == 4) format = GL_RGBA;

    if (textureID_ == 0) {
        glGenTextures(1, &textureID_);
    }
    glBindTexture(GL_TEXTURE_2D, textureID_);
    
    // 텍스처 파라미터 설정 (필요에 따라 조절)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // 이미지 업로드
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    // 정리
    stbi_image_free(data);
    glBindTexture(GL_TEXTURE_2D, 0);

    std::cout << "Texture loaded: " << filepath << " (ID=" << textureID_ << ", " << width << "x" << height << ", ch=" << channels << ")" << std::endl;
    return true;
}