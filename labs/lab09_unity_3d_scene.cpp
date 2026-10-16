#include <epoxy/gl.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>

// --- SHADERS ---
const char *vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

out vec3 FragPos;
out vec3 Normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
)";

const char *fragmentShaderSource = R"(
#version 330 core
in vec3 FragPos;
in vec3 Normal;
out vec4 FragColor;

uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 objectColor;
uniform vec3 viewPos;

void main() {
    float ambientStrength = 0.15;
    vec3 ambient = ambientStrength * lightColor;

    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    float specularStrength = 0.5;
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * lightColor;

    vec3 result = (ambient + diffuse + specular) * objectColor;
    FragColor = vec4(result, 1.0);
}
)";

// --- HELPER FUNCTIONS ---
GLuint compileShader(GLenum type, const char *source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    int success;
    char infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "Shader compilation failed: " << infoLog << std::endl;
    }
    return shader;
}

GLuint createProgram(const char *vSource, const char *fSource) {
    GLuint prog = glCreateProgram();
    GLuint vShader = compileShader(GL_VERTEX_SHADER, vSource);
    GLuint fShader = compileShader(GL_FRAGMENT_SHADER, fSource);
    glAttachShader(prog, vShader);
    glAttachShader(prog, fShader);
    glLinkProgram(prog);
    glDeleteShader(vShader);
    glDeleteShader(fShader);
    int success;
    char infoLog[512];
    glGetProgramiv(prog, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(prog, 512, nullptr, infoLog);
        std::cerr << "Program linking failed: " << infoLog << std::endl;
    }
    return prog;
}

// --- CUBE DATA (position + normal, 36 vertices) ---
float cubeVertices[] = {
    // front face (+Z) - blue tint
    -0.5f, -0.5f,  0.5f,   0.0f,  0.0f,  1.0f,
     0.5f, -0.5f,  0.5f,   0.0f,  0.0f,  1.0f,
     0.5f,  0.5f,  0.5f,   0.0f,  0.0f,  1.0f,
     0.5f,  0.5f,  0.5f,   0.0f,  0.0f,  1.0f,
    -0.5f,  0.5f,  0.5f,   0.0f,  0.0f,  1.0f,
    -0.5f, -0.5f,  0.5f,   0.0f,  0.0f,  1.0f,
    // back face (-Z)
    -0.5f, -0.5f, -0.5f,   0.0f,  0.0f, -1.0f,
    -0.5f,  0.5f, -0.5f,   0.0f,  0.0f, -1.0f,
     0.5f,  0.5f, -0.5f,   0.0f,  0.0f, -1.0f,
     0.5f,  0.5f, -0.5f,   0.0f,  0.0f, -1.0f,
     0.5f, -0.5f, -0.5f,   0.0f,  0.0f, -1.0f,
    -0.5f, -0.5f, -0.5f,   0.0f,  0.0f, -1.0f,
    // right face (+X)
     0.5f, -0.5f,  0.5f,   1.0f,  0.0f,  0.0f,
     0.5f,  0.5f,  0.5f,   1.0f,  0.0f,  0.0f,
     0.5f,  0.5f, -0.5f,   1.0f,  0.0f,  0.0f,
     0.5f,  0.5f, -0.5f,   1.0f,  0.0f,  0.0f,
     0.5f, -0.5f, -0.5f,   1.0f,  0.0f,  0.0f,
     0.5f, -0.5f,  0.5f,   1.0f,  0.0f,  0.0f,
    // left face (-X)
    -0.5f, -0.5f,  0.5f,  -1.0f,  0.0f,  0.0f,
    -0.5f, -0.5f, -0.5f,  -1.0f,  0.0f,  0.0f,
    -0.5f,  0.5f, -0.5f,  -1.0f,  0.0f,  0.0f,
    -0.5f,  0.5f, -0.5f,  -1.0f,  0.0f,  0.0f,
    -0.5f,  0.5f,  0.5f,  -1.0f,  0.0f,  0.0f,
    -0.5f, -0.5f,  0.5f,  -1.0f,  0.0f,  0.0f,
    // top face (+Y)
    -0.5f,  0.5f,  0.5f,   0.0f,  1.0f,  0.0f,
     0.5f,  0.5f,  0.5f,   0.0f,  1.0f,  0.0f,
     0.5f,  0.5f, -0.5f,   0.0f,  1.0f,  0.0f,
     0.5f,  0.5f, -0.5f,   0.0f,  1.0f,  0.0f,
    -0.5f,  0.5f, -0.5f,   0.0f,  1.0f,  0.0f,
    -0.5f,  0.5f,  0.5f,   0.0f,  1.0f,  0.0f,
    // bottom face (-Y)
    -0.5f, -0.5f,  0.5f,   0.0f, -1.0f,  0.0f,
     0.5f, -0.5f,  0.5f,   0.0f, -1.0f,  0.0f,
     0.5f, -0.5f, -0.5f,   0.0f, -1.0f,  0.0f,
     0.5f, -0.5f, -0.5f,   0.0f, -1.0f,  0.0f,
    -0.5f, -0.5f, -0.5f,   0.0f, -1.0f,  0.0f,
    -0.5f, -0.5f,  0.5f,   0.0f, -1.0f,  0.0f,
};

// --- MAIN PROGRAM ---
int main() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    const int WINDOW_WIDTH = 1024;
    const int WINDOW_HEIGHT = 768;
    GLFWwindow *window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT,
                                          "Lab09: Unity-Style 3D Interactive Scene", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    GLuint shaderProgram = createProgram(vertexShaderSource, fragmentShaderSource);

    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);

    // Position attribute (3 floats)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    // Normal attribute (3 floats), offset 3 floats
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Enable depth testing for 3D
    glEnable(GL_DEPTH_TEST);

    // Interaction / rotation state
    float rotationAngle = 0.0f;
    float rotationSpeed = 30.0f; // degrees per second
    float cubeScale = 1.0f;
    bool autoRotate = true;
    bool wireframe = false;

    // Mouse tracking for manual rotation
    double lastMouseX = WINDOW_WIDTH / 2.0;
    double lastMouseY = WINDOW_HEIGHT / 2.0;
    double yaw = -90.0f;
    double pitch = 0.0f;
    bool mousePressed = false;

    glfwSetCursorPosCallback(window, [](GLFWwindow* w, double xpos, double ypos) {
        // Manual rotation via mouse handled in loop
    });

    glfwSetMouseButtonCallback(window, [](GLFWwindow* w, int button, int action, int mods) {
        if (button == GLFW_MOUSE_BUTTON_LEFT) {
            // Toggle auto-rotate on left click
        }
    });

    // Camera
    glm::vec3 cameraPos   = glm::vec3(2.0f, 2.0f, 4.0f);
    glm::vec3 cameraFront = glm::normalize(glm::vec3(-0.5f, -0.3f, -0.8f));
    glm::vec3 cameraUp    = glm::vec3(0.0f, 1.0f, 0.0f);

    float deltaTime = 0.0f;
    float lastFrame = 0.0f;

    std::cout << "=== Lab09: Unity-Style 3D Interactive Scene ===" << std::endl;
    std::cout << "Controls:" << std::endl;
    std::cout << "  W / A / S / D  - Move camera" << std::endl;
    std::cout << "  Arrow Keys     - Rotate cube manually" << std::endl;
    std::cout << "  R              - Reset rotation" << std::endl;
    std::cout << "  SPACE          - Toggle auto-rotation" << std::endl;
    std::cout << "  Q / E          - Scale cube up / down" << std::endl;
    std::cout << "  ESC            - Exit" << std::endl;
    std::cout << "===========================================" << std::endl;

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Keyboard interaction
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);

        // Toggle auto-rotate
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
            // Simple debounce: don't toggle continuously
            static bool spaceWasDown = false;
            if (!spaceWasDown) {
                autoRotate = !autoRotate;
                std::cout << (autoRotate ? "Auto-rotation ON" : "Auto-rotation OFF") << std::endl;
            }
            spaceWasDown = true;
        } else {
            static bool spaceWasDown = false;
            spaceWasDown = false;
        }

        // Scale with Q (increase) / E (decrease)
        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
            cubeScale += 0.5f * deltaTime;
            if (cubeScale > 3.0f) cubeScale = 3.0f;
        }
        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
            cubeScale -= 0.5f * deltaTime;
            if (cubeScale < 0.2f) cubeScale = 0.2f;
        }

        // Manual rotation with arrow keys (Unity-style control)
        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
            rotationAngle += 45.0f * deltaTime;
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
            rotationAngle -= 45.0f * deltaTime;
        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
            rotationAngle += 30.0f * deltaTime;
        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
            rotationAngle -= 30.0f * deltaTime;

        if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
            rotationAngle = 0.0f;
            cubeScale = 1.0f;
            cameraPos = glm::vec3(2.0f, 2.0f, 4.0f);
            std::cout << "Reset applied" << std::endl;
        }

        // Auto-rotate
        if (autoRotate) {
            rotationAngle += rotationSpeed * deltaTime;
        }

        // Camera movement with WASD
        float cameraSpeed = 2.5f * deltaTime;
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            cameraPos += cameraSpeed * cameraFront;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            cameraPos -= cameraSpeed * cameraFront;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;

        // Mouse-based manual rotation tracking (optional)
        double mouseX, mouseY;
        glfwGetCursorPos(window, &mouseX, &mouseY);
        double deltaX = mouseX - lastMouseX;
        double deltaY = mouseY - lastMouseY;
        lastMouseX = mouseX;
        lastMouseY = mouseY;

        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);

        glClearColor(0.08f, 0.08f, 0.14f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Build matrices
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
        model = glm::rotate(model, glm::radians(rotationAngle), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::rotate(model, glm::radians(20.0f + rotationAngle * 0.25f), glm::vec3(1.0f, 0.2f, 0.5f));
        model = glm::scale(model, glm::vec3(cubeScale, cubeScale, cubeScale));

        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        glm::mat4 projection = glm::perspective(glm::radians(45.0f),
                                               (float)display_w / (float)display_h,
                                               0.1f, 100.0f);

        glUseProgram(shaderProgram);

        // Set uniforms
        GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
        GLint viewLoc  = glGetUniformLocation(shaderProgram, "view");
        GLint projLoc  = glGetUniformLocation(shaderProgram, "projection");
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(viewLoc,  1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc,  1, GL_FALSE, glm::value_ptr(projection));

        glUniform3f(glGetUniformLocation(shaderProgram, "lightPos"), 2.0f, 4.0f, 2.0f);
        glUniform3f(glGetUniformLocation(shaderProgram, "lightColor"), 1.0f, 1.0f, 0.95f);
        glUniform3f(glGetUniformLocation(shaderProgram, "objectColor"), 0.35f, 0.55f, 0.9f);
        glUniform3f(glGetUniformLocation(shaderProgram, "viewPos"), cameraPos.x, cameraPos.y, cameraPos.z);

        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
