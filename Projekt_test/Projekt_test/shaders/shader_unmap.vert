#version 430 core

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec3 vertexNormal;
layout(location = 2) in vec2 vertexTexCoord;
layout(location = 3) in vec3 vertexTangent;
layout(location = 4) in vec3 vertexBitangent;

uniform mat4 transformation;
uniform mat4 modelMatrix;

out vec3 FragPos;
out vec2 TexCoords;
out mat3 TBN;

void main()
{
    FragPos = vec3(modelMatrix * vec4(vertexPosition, 1.0));
    TexCoords = vertexTexCoord;

    vec3 T = normalize(mat3(modelMatrix) * vertexTangent);
    vec3 N = normalize(mat3(modelMatrix) * vertexNormal);
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T);

    TBN = mat3(T, B, N);

    gl_Position = transformation * vec4(vertexPosition, 1.0);
}
