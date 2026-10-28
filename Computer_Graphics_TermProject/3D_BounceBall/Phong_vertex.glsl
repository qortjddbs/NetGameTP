#version 330 core

layout (location = 0) in vec3 vPos;
layout (location = 1) in vec3 vColor;
layout (location = 2) in vec3 vNormal;
layout (location = 3) in vec2 vTexCoord;  // 텍스처 좌표 추가

out vec3 FragPos;
out vec3 Normal;
out vec3 vertexColor;
out vec2 TexCoord;  // Fragment Shader로 전달

uniform mat4 modelMat;
uniform mat4 view;
uniform mat4 proj;

void main()
{
    gl_Position = proj * view * modelMat * vec4(vPos, 1.0);
    FragPos = vec3(modelMat * vec4(vPos, 1.0));
    Normal = mat3(transpose(inverse(modelMat))) * vNormal;
    vertexColor = vColor;
    TexCoord = vTexCoord;  // 텍스처 좌표 전달
}