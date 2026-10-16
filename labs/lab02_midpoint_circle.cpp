#include <epoxy/gl.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <vector>

// --- 1. SHADERS ---
const char *vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
uniform mat4 projection;
void main() {
    gl_Position = projection * vec4(aPos, 0.0, 1.0);
    gl_PointSize = 4.0; // Point size defined inside vertex shader
}
)";

const char *fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;
void main() {
    FragColor = vec4(1.0, 0.5, 0.0, 1.0); // Bright Orange
}
)";

// --- 2. MIDPOINT CIRCLE ALGORITHM ---
struct Point {
  int x, y;
};

// Helper to add points in all 8 octants
void addSymmetricPoints(int cx, int cy, int x, int y,
                        std::vector<Point> &points) {
  points.push_back({cx + x, cy + y});
  points.push_back({cx - x, cy + y});
  points.push_back({cx + x, cy - y});
  points.push_back({cx - x, cy - y});
  points.push_back({cx + y, cy + x});
  points.push_back({cx - y, cy + x});
  points.push_back({cx + y, cy - x});
  points.push_back({cx - y, cy - x});
}

std::vector<Point> getMidpointCircle(int centerX, int centerY, int radius) {
  std::vector<Point> circlePoints;
  int x = 0;
  int y = radius;
  int p = 1 - radius; // Initial decision parameter

  // Plot initial points at cardinal positions (0, r), (r, 0), etc.
  addSymmetricPoints(centerX, centerY, x, y, circlePoints);

  while (x < y) {
    x++;
    if (p < 0) {
      p += 2 * x + 1; // Midpoint is inside the circle
    } else {
      y--;
      p += 2 * (x - y) + 1; // Midpoint is outside or on the circle
    }
    addSymmetricPoints(centerX, centerY, x, y, circlePoints);
  }

  return circlePoints;
}

// --- 3. HELPER FUNCTIONS ---
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

// --- 4. MAIN PROGRAM ---
int main() {
  if (!glfwInit())
    return -1;
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  const int WINDOW_WIDTH = 800;
  const int WINDOW_HEIGHT = 600;
  GLFWwindow *window =
      glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Midpoint Circle Algorithm",
                       nullptr, nullptr);
  if (!window) {
    glfwTerminate();
    return -1;
  }
  glfwMakeContextCurrent(window);

  GLuint shaderProgram =
      createProgram(vertexShaderSource, fragmentShaderSource);

  // Generate circle points (Center: 400, 300 | Radius: 180)
  std::vector<Point> points = getMidpointCircle(400, 300, 180);

  // Convert points to float vector for VBO
  std::vector<float> vertices;
  for (const auto &p : points) {
    vertices.push_back((float)p.x);
    vertices.push_back((float)p.y);
  }

  // Setup VAO / VBO
  GLuint VAO, VBO;
  glGenVertexArrays(1, &VAO);
  glGenBuffers(1, &VBO);

  glBindVertexArray(VAO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float),
               vertices.data(), GL_STATIC_DRAW);

  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);

  glEnable(GL_PROGRAM_POINT_SIZE);

  glm::mat4 projection = glm::ortho(0.0f, (float)WINDOW_WIDTH, 0.0f,
                                    (float)WINDOW_HEIGHT, -1.0f, 1.0f);

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
    glDrawArrays(GL_POINTS, 0, vertices.size() / 2);

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glfwDestroyWindow(window);
  glfwTerminate();
  return 0;
}
