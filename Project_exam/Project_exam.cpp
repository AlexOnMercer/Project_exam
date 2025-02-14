#include <GL/glew.h>
#include <gl/freeglut.h>
#include <vector>
#include <cmath>
#include <iostream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#ifndef PI
#define PI 3.14159265358979323846
#endif

const int WINDOW_WIDTH = 1500;
const int WINDOW_HEIGHT = 1100;

const int SPHERE_LATITUDE = 40;
const int SPHERE_LONGITUDE = 40;

struct CelestialBody {
    float radius;
    float orbitRadius;
    float rotationSpeed;
    float orbitSpeed;
    float currentAngle;
    float selfRotation;
    GLuint texture;
    std::vector<float> sphereVertices;
    std::vector<unsigned int> sphereIndices;
    GLuint VAO, VBO, EBO;
    bool isSun;
    glm::vec3 color;
};

std::vector<CelestialBody> celestialBodies;
GLuint shaderProgram;
GLuint shadowShaderProgram;
float zoomLevel = 30.0f;

GLuint loadTexture(const char* filename) {
    int width, height, channels;
    unsigned char* image = stbi_load(filename, &width, &height, &channels, STBI_rgb_alpha);

    if (!image) {
        std::cerr << "Cannot load texture: " << filename << std::endl;
        return 0;
    }

    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(image);
    return textureID;
}

void generateSphere(CelestialBody& body, float radius) {
    body.sphereVertices.clear();
    body.sphereIndices.clear();

    for (int lat = 0; lat <= SPHERE_LATITUDE; ++lat) {
        float theta = lat * PI / SPHERE_LATITUDE;
        float sinTheta = sin(theta);
        float cosTheta = cos(theta);

        for (int lon = 0; lon <= SPHERE_LONGITUDE; ++lon) {
            float phi = lon * 2.0f * PI / SPHERE_LONGITUDE;
            float sinPhi = sin(phi);
            float cosPhi = cos(phi);

            float x = cosPhi * sinTheta;
            float y = cosTheta;
            float z = sinPhi * sinTheta;

            float u = lon / (float)SPHERE_LONGITUDE;
            float v = lat / (float)SPHERE_LATITUDE;

            body.sphereVertices.push_back(radius * x);
            body.sphereVertices.push_back(radius * y);
            body.sphereVertices.push_back(radius * z);
            body.sphereVertices.push_back(x);
            body.sphereVertices.push_back(y);
            body.sphereVertices.push_back(z);
            body.sphereVertices.push_back(u);
            body.sphereVertices.push_back(v);
        }
    }

    for (int lat = 0; lat < SPHERE_LATITUDE; ++lat) {
        for (int lon = 0; lon < SPHERE_LONGITUDE; ++lon) {
            int first = (lat * (SPHERE_LONGITUDE + 1)) + lon;
            int second = first + SPHERE_LONGITUDE + 1;

            body.sphereIndices.push_back(first);
            body.sphereIndices.push_back(second);
            body.sphereIndices.push_back(first + 1);

            body.sphereIndices.push_back(second);
            body.sphereIndices.push_back(second + 1);
            body.sphereIndices.push_back(first + 1);
        }
    }

    glGenVertexArrays(1, &body.VAO);
    glGenBuffers(1, &body.VBO);
    glGenBuffers(1, &body.EBO);

    glBindVertexArray(body.VAO);

    glBindBuffer(GL_ARRAY_BUFFER, body.VBO);
    glBufferData(GL_ARRAY_BUFFER, body.sphereVertices.size() * sizeof(float), body.sphereVertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, body.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, body.sphereIndices.size() * sizeof(unsigned int), body.sphereIndices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
}

void initCelestialBodies() {
    std::vector<std::string> planetTextures = {
        "mercury.jpg", "venus.jpg", "earth.jpg", "mars.jpg",
        "jupiter.jpg", "saturn.jpg", "uranus.jpg", "neptune.jpg"
    };


    CelestialBody sun;
    sun.radius = 2.0f;
    sun.orbitRadius = 0.0f;
    sun.rotationSpeed = 0.0005f;
    sun.orbitSpeed = 0.0f;
    sun.currentAngle = 0.0f;
    sun.selfRotation = 0.0f;
    sun.texture = loadTexture("sun.jpg");
    sun.isSun = true;
    sun.color = glm::vec3(1.0f, 1.0f, 0.8f);
    generateSphere(sun, sun.radius);
    celestialBodies.push_back(sun);

    // Добавление планет
    for (int i = 0; i < 8; ++i) {
        CelestialBody planet;
        planet.radius = 0.3f + i * 0.1f;
        planet.orbitRadius = 5.0f + i * 2.0f;
        planet.rotationSpeed = 0.0002f * (i + 1);
        planet.orbitSpeed = 0.001f / (i + 1);
        planet.currentAngle = 0.0f;
        planet.selfRotation = 0.0f;
        planet.texture = loadTexture(planetTextures[i].c_str());
        planet.isSun = false;


        if (i == 2) {
            generateSphere(planet, planet.radius);
            celestialBodies.push_back(planet);

            // Луна
            CelestialBody moon;
            moon.radius = 0.1f;
            moon.orbitRadius = planet.radius + 0.5f;
            moon.rotationSpeed = 0.00003f;
            moon.orbitSpeed = 0.001f;
            moon.currentAngle = 0.0f;
            moon.selfRotation = 0.0f;
            moon.texture = loadTexture("moon.jpg");
            moon.isSun = false;
            generateSphere(moon, moon.radius);
            celestialBodies.push_back(moon);
        }
        else {
            generateSphere(planet, planet.radius);
            celestialBodies.push_back(planet);
        }
    }
}


const char* vertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec3 aNormal;
    layout (location = 2) in vec2 aTexCoord;
    
    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;
    uniform vec3 lightPos;
    uniform bool isSun;
    
    out vec2 TexCoord;
    out vec3 Normal;
    out vec3 FragPos;
    out vec3 LightPos;
    
    void main() {
        FragPos = vec3(model * vec4(aPos, 1.0));
        Normal = mat3(transpose(inverse(model))) * aNormal;
        gl_Position = projection * view * vec4(FragPos, 1.0);
        TexCoord = aTexCoord;
        LightPos = lightPos;
    }
)";

const char* fragmentShaderSource = R"(
    #version 330 core
    in vec2 TexCoord;
    in vec3 Normal;
    in vec3 FragPos;
    in vec3 LightPos;
    
    uniform sampler2D texture1;
    uniform bool isSun;
    uniform vec3 sunColor;
    
    out vec4 FragColor;
    
    void main() {
        if (isSun) {
            FragColor = texture(texture1, TexCoord);
        } else {
            vec3 lightDir = normalize(LightPos - FragPos);
            float diff = max(dot(Normal, lightDir), 0.1);
            
            vec3 ambient = 0.1 * sunColor;
            vec3 diffuse = diff * sunColor;
            
            vec4 texColor = texture(texture1, TexCoord);
            vec3 finalColor = (ambient + diffuse) * texColor.rgb;
            
            FragColor = vec4(finalColor, texColor.a);
        }
    }
)";

GLuint createShaderProgram() {
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return program;
}

void display() {
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(shaderProgram);

    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT, 0.1f, 100.0f);
    glm::mat4 view = glm::lookAt(
        glm::vec3(0.0f, 10.0f, zoomLevel),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );

    GLint projLoc = glGetUniformLocation(shaderProgram, "projection");
    GLint viewLoc = glGetUniformLocation(shaderProgram, "view");
    GLint lightPosLoc = glGetUniformLocation(shaderProgram, "lightPos");
    GLint isSunLoc = glGetUniformLocation(shaderProgram, "isSun");
    GLint sunColorLoc = glGetUniformLocation(shaderProgram, "sunColor");

    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

    for (auto& body : celestialBodies) {
        if (body.isSun) {
            body.selfRotation += body.rotationSpeed;
        }
        else {
            body.currentAngle += body.orbitSpeed;
            body.selfRotation += body.rotationSpeed;
        }

        glm::mat4 model = glm::mat4(1.0f);

        if (body.radius == 0.1f) {
            auto& earth = celestialBodies[3];
            float x = std::cos(earth.currentAngle) * earth.orbitRadius + std::cos(body.currentAngle) * body.orbitRadius;
            float z = std::sin(earth.currentAngle) * earth.orbitRadius + std::sin(body.currentAngle) * body.orbitRadius;

            model = glm::translate(model, glm::vec3(x, 0.0f, z));
            model = glm::rotate(model, body.selfRotation, glm::vec3(0.0f, 1.0f, 0.0f));
        }
        else {
            float x = std::cos(body.currentAngle) * body.orbitRadius;
            float z = std::sin(body.currentAngle) * body.orbitRadius;

            model = glm::translate(model, glm::vec3(x, 0.0f, z));
            model = glm::rotate(model, body.selfRotation, glm::vec3(0.0f, 1.0f, 0.0f));
        }

        GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

        glUniform1i(isSunLoc, body.isSun);
        glUniform3fv(sunColorLoc, 1, glm::value_ptr(body.isSun ? glm::vec3(1.0f, 1.0f, 0.8f) : celestialBodies[0].color));

        glm::vec3 lightPos(0.0f, 0.0f, 0.0f);
        glUniform3fv(lightPosLoc, 1, glm::value_ptr(lightPos));

        glBindTexture(GL_TEXTURE_2D, body.texture);
        glBindVertexArray(body.VAO);
        glDrawElements(GL_TRIANGLES, body.sphereIndices.size(), GL_UNSIGNED_INT, 0);
    }

    glutSwapBuffers();
}

void reshape(int width, int height) {
    glViewport(0, 0, width, height);
    glutPostRedisplay();
}

void idle() {
    glutPostRedisplay();
}

void keyboard(unsigned char key, int x, int y) {
    switch (key) {
    case '+':
    case '=':
        zoomLevel = std::max(5.0f, zoomLevel - 1.0f);
        break;
    case '-':
    case '_':
        zoomLevel = std::min(50.0f, zoomLevel + 1.0f);
        break;
    case 27:
        exit(0);
        break;
    }
    glutPostRedisplay();
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
    glutCreateWindow("Enhanced Solar System Simulation");

    glewInit();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    initCelestialBodies();
    shaderProgram = createShaderProgram();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutIdleFunc(idle);
    glutKeyboardFunc(keyboard);

    std::cout << "Controls:" << std::endl;
    std::cout << "+ : Zoom in" << std::endl;
    std::cout << "- : Zoom out" << std::endl;
    std::cout << "ESC : Exit" << std::endl;

    glutMainLoop();

    return 0;
}