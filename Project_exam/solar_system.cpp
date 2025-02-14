// solar_system.cpp
#include "solar_system.h"
#include <iostream>
#include <cmath>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

const char* vertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;
    out vec3 FragPos;
    out vec3 Normal;

    void main() {
        FragPos = vec3(model * vec4(aPos, 1.0));
        Normal = mat3(transpose(inverse(model))) * aPos;
        gl_Position = projection * view * vec4(FragPos, 1.0);
    }
)";

const char* fragmentShaderSource = R"(
    #version 330 core
    in vec3 FragPos;
    in vec3 Normal;
    uniform vec3 objectColor;
    out vec4 FragColor;

    void main() {
        vec3 lightColor = vec3(1.0, 1.0, 1.0);
        vec3 lightPos = vec3(0.0, 0.0, 10.0);
        
        float ambientStrength = 0.1;
        vec3 ambient = ambientStrength * lightColor;
        
        vec3 norm = normalize(Normal);
        vec3 lightDir = normalize(lightPos - FragPos);
        float diff = max(dot(norm, lightDir), 0.0);
        vec3 diffuse = diff * lightColor;
        
        vec3 result = (ambient + diffuse) * objectColor;
        FragColor = vec4(result, 1.0);
    }
)";

// Остальной код остается без изменений

void SolarSystem::createSphereBuffers() {
    const int segments = 36;
    std::vector<float> vertices;

    for (int lat = 0; lat <= segments; ++lat) {
        float theta = lat * static_cast<float>(M_PI) / segments;
        for (int lon = 0; lon <= segments; ++lon) {
            float phi = lon * 2 * static_cast<float>(M_PI) / segments;
            float x = std::sin(theta) * std::cos(phi);
            float y = std::sin(theta) * std::sin(phi);
            float z = std::cos(theta);
            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);
        }
    }

    // Остальной код без изменений
}

void SolarSystem::drawSphere(const CelestialBody& body, const glm::mat4& projection, const glm::mat4& view) {
    glUseProgram(shaderProgram);

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(std::cos(body.currentAngle) * body.distanceFromSun, 0.0f, std::sin(body.currentAngle) * body.distanceFromSun));
    model = glm::scale(model, glm::vec3(body.radius));

    GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
    GLint viewLoc = glGetUniformLocation(shaderProgram, "view");
    GLint projectionLoc = glGetUniformLocation(shaderProgram, "projection");
    GLint colorLoc = glGetUniformLocation(shaderProgram, "objectColor");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
    glUniform3fv(colorLoc, 1, glm::value_ptr(body.color));

    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 36 * 36);
}