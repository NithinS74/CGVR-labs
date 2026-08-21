#include <epoxy/gl.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>

// Vertex Shader: Accepts position, color, and MVP Matrix
const char *vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;

out vec3 ourColor;

uniform mat4 uMVP;

void main() {
    gl_Position = uMVP * vec4(aPos, 1.0);
    ourColor = aColor;
}
)";

// Fragment Shader
const char *fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;
in vec3 ourColor;

void main() {
    FragColor = vec4(ourColor, 1.0f);
}
)";

void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
  glViewport(0, 0, width, height);
}

int main() {
  // 1. Initialize GLFW & Create Window
  if (!glfwInit())
    return -1;

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  GLFWwindow *window =
      glfwCreateWindow(800, 600, "Rotating 3D Cube", nullptr, nullptr);
  if (!window) {
    glfwTerminate();
    return -1;
  }
  glfwMakeContextCurrent(window);
  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

  // 2. Enable Depth Testing for 3D Rendering
  glEnable(GL_DEPTH_TEST);

  // 3. Compile & Link Shaders
  GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
  glCompileShader(vertexShader);

  GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
  glCompileShader(fragmentShader);

  GLuint shaderProgram = glCreateProgram();
  glAttachShader(shaderProgram, vertexShader);
  glAttachShader(shaderProgram, fragmentShader);
  glLinkProgram(shaderProgram);

  glDeleteShader(vertexShader);
  glDeleteShader(fragmentShader);

  // Get location of the MVP uniform matrix
  GLint mvpLoc = glGetUniformLocation(shaderProgram, "uMVP");

  // 4. Cube Vertex Data (36 vertices: 6 faces * 2 triangles) with distinct face
  // colors
  float vertices[] = {// Positions           // Colors
                      // Back Face (Red)
                      -0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 0.5f, -0.5f, -0.5f,
                      1.0f, 0.0f, 0.0f, 0.5f, 0.5f, -0.5f, 1.0f, 0.0f, 0.0f,
                      0.5f, 0.5f, -0.5f, 1.0f, 0.0f, 0.0f, -0.5f, 0.5f, -0.5f,
                      1.0f, 0.0f, 0.0f, -0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f,

                      // Front Face (Green)
                      -0.5f, -0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 0.5f, -0.5f, 0.5f,
                      0.0f, 1.0f, 0.0f, 0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f,
                      0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f, -0.5f, 0.5f, 0.5f,
                      0.0f, 1.0f, 0.0f, -0.5f, -0.5f, 0.5f, 0.0f, 1.0f, 0.0f,

                      // Left Face (Blue)
                      -0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f, -0.5f, 0.5f, -0.5f,
                      0.0f, 0.0f, 1.0f, -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 1.0f,
                      -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 1.0f, -0.5f, -0.5f, 0.5f,
                      0.0f, 0.0f, 1.0f, -0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f,

                      // Right Face (Yellow)
                      0.5f, 0.5f, 0.5f, 1.0f, 1.0f, 0.0f, 0.5f, 0.5f, -0.5f,
                      1.0f, 1.0f, 0.0f, 0.5f, -0.5f, -0.5f, 1.0f, 1.0f, 0.0f,
                      0.5f, -0.5f, -0.5f, 1.0f, 1.0f, 0.0f, 0.5f, -0.5f, 0.5f,
                      1.0f, 1.0f, 0.0f, 0.5f, 0.5f, 0.5f, 1.0f, 1.0f, 0.0f,

                      // Bottom Face (Magenta)
                      -0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 1.0f, 0.5f, -0.5f, -0.5f,
                      1.0f, 0.0f, 1.0f, 0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 1.0f,
                      0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 1.0f, -0.5f, -0.5f, 0.5f,
                      1.0f, 0.0f, 1.0f, -0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 1.0f,

                      // Top Face (Cyan)
                      -0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 0.5f, 0.5f, -0.5f,
                      0.0f, 1.0f, 1.0f, 0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 1.0f,
                      0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 1.0f, -0.5f, 0.5f, 0.5f,
                      0.0f, 1.0f, 1.0f, -0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 1.0f};

  GLuint VAO, VBO;
  glGenVertexArrays(1, &VAO);
  glGenBuffers(1, &VBO);

  glBindVertexArray(VAO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  // Position attribute
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);

  // Color attribute
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                        (void *)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);

  // 5. Main Render Loop
  while (!glfwWindowShouldClose(window)) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
      glfwSetWindowShouldClose(window, true);

    // Clear both Color Buffer and Depth Buffer
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Use shader program
    glUseProgram(shaderProgram);

    // Calculate View and Projection Matrices
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    float aspect = (height > 0) ? (float)width / (float)height : 1.0f;

    // Projection: 45 degree FOV, Perspective
    glm::mat4 projection =
        glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);

    // View: Camera positioned at (0, 1.5, 3), looking at center (0, 0, 0)
    glm::mat4 view =
        glm::lookAt(glm::vec3(0.0f, 1.5f, 3.0f), glm::vec3(0.0f, 0.0f, 0.0f),
                    glm::vec3(0.0f, 1.0f, 0.0f));

    // Model: Rotate continuously based on time
    float time = (float)glfwGetTime();
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::rotate(model, time * glm::radians(50.0f),
                        glm::vec3(0.5f, 1.0f, 0.0f));

    // MVP = Projection * View * Model (Order matters!)
    glm::mat4 mvp = projection * view * model;

    // Send MVP matrix to Vertex Shader
    glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(mvp));

    // Draw the 36 vertices (12 triangles) of the cube
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  // Cleanup
  glDeleteVertexArrays(1, &VAO);
  glDeleteBuffers(1, &VBO);
  glDeleteProgram(shaderProgram);

  glfwDestroyWindow(window);
  glfwTerminate();
  return 0;
}
