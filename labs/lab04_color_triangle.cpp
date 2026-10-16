#include <epoxy/gl.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>

// --- 1. SHADERS ---
const char *vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec2 aPos;   // Position attribute
layout (location = 1) in vec3 aColor; // Color attribute

out vec3 ourColor; // Pass color to fragment shader

uniform mat4 projection;

void main() {
    gl_Position = projection * vec4(aPos, 0.0, 1.0);
    ourColor = aColor; // Pass vertex color down the pipeline
}
)";

const char *fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;
in vec3 ourColor; // Interpolated color from vertex shader

void main() {
    FragColor = vec4(ourColor, 1.0);
}
)";

// --- 2. HELPER FUNCTIONS ---
GLuint compileShader(GLenum type, const char *source) {
  GLuint shader = glCreateShader(type);
  glShaderSource(shader, 1, &source, nullptr);
  glCompileShader(shader);
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
  return prog;
}

// --- 3. MAIN PROGRAM ---
int main() {
  if (!glfwInit())
    return -1;
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  const int WINDOW_WIDTH = 800;
  const int WINDOW_HEIGHT = 600;
  GLFWwindow *window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT,
                                        "Color Triangle", nullptr, nullptr);
  if (!window) {
    glfwTerminate();
    return -1;
  }
  glfwMakeContextCurrent(window);

  GLuint shaderProgram =
      createProgram(vertexShaderSource, fragmentShaderSource);

  // Triangle data: X, Y, Red, Green, Blue
  float vertices[] = {
      // Position          // Color (RGB)
      400.0f, 480.0f, 1.0f, 0.0f, 0.0f, // Top vertex (Red)
      150.0f, 120.0f, 0.0f, 1.0f, 0.0f, // Bottom-Left vertex (Green)
      650.0f, 120.0f, 0.0f, 0.0f, 1.0f  // Bottom-Right vertex (Blue)
  };

  // Setup VAO and VBO
  GLuint VAO, VBO;
  glGenVertexArrays(1, &VAO);
  glGenBuffers(1, &VBO);

  glBindVertexArray(VAO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  // Attribute 0: Position (2 floats)
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);

  // Attribute 1: Color (3 floats)
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                        (void *)(2 * sizeof(float)));
  glEnableVertexAttribArray(1);

  glm::mat4 projection =
      glm::ortho(0.0f, static_cast<float>(WINDOW_WIDTH), 0.0f,
                 static_cast<float>(WINDOW_HEIGHT), -1.0f, 1.0f);

  while (!glfwWindowShouldClose(window)) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
      glfwSetWindowShouldClose(window, true);

    int display_w, display_h;
    glfwGetFramebufferSize(window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);

    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(shaderProgram);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1,
                       GL_FALSE, glm::value_ptr(projection));

    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glfwDestroyWindow(window);
  glfwTerminate();
  return 0;
}
