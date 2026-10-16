#include <epoxy/gl.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cmath>
#include <iostream>
#include <vector>
#include <algorithm>

// --- 1. SHADERS ---
const char *vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
uniform mat4 projection;
void main() {
    gl_Position = projection * vec4(aPos, 0.0, 1.0);
    gl_PointSize = 5.0;
}
)";

const char *fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;
void main() {
    FragColor = vec4(0.0, 1.0, 0.5, 1.0);
}
)";

// --- 2. POINT STRUCT ---
struct Point {
  int x, y;
};

// --- 3. BRESENHAM'S ALGORITHM ---
std::vector<Point> getBresenhamLine(int x0, int y0, int x1, int y1) {
  std::vector<Point> linePoints;
  int dx = std::abs(x1 - x0);
  int dy = std::abs(y1 - y0);
  int sx = (x0 < x1) ? 1 : -1;
  int sy = (y0 < y1) ? 1 : -1;

  int err = dx - dy;

  while (true) {
    linePoints.push_back({x0, y0});
    if (x0 == x1 && y0 == y1)
      break;

    int e2 = 2 * err;
    if (e2 > -dy) {
      err -= dy;
      x0 += sx;
    }
    if (e2 < dx) {
      err += dx;
      y0 += sy;
    }
  }
  return linePoints;
}

// --- 4. DDA ALGORITHM ---
std::vector<Point> getDDALine(int x0, int y0, int x1, int y1) {
  std::vector<Point> linePoints;

  int dx = x1 - x0;
  int dy = y1 - y0;
  int steps = std::max(std::abs(dx), std::abs(dy));

  float xInc = (steps == 0) ? 0.0f : static_cast<float>(dx) / static_cast<float>(steps);
  float yInc = (steps == 0) ? 0.0f : static_cast<float>(dy) / static_cast<float>(steps);

  float x = static_cast<float>(x0);
  float y = static_cast<float>(y0);

  for (int i = 0; i <= steps; ++i) {
    linePoints.push_back({static_cast<int>(std::round(x)), static_cast<int>(std::round(y))});
    x += xInc;
    y += yInc;
  }

  return linePoints;
}

// --- 5. HELPER FUNCTIONS ---
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

// --- 6. MAIN PROGRAM ---
int main() {
  if (!glfwInit())
    return -1;
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  const int WINDOW_WIDTH = 800;
  const int WINDOW_HEIGHT = 600;
  GLFWwindow *window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT,
                                        "Lab01: Bresenham + DDA Line", nullptr, nullptr);
  if (!window) {
    glfwTerminate();
    return -1;
  }
  glfwMakeContextCurrent(window);

  GLuint shaderProgram = createProgram(vertexShaderSource, fragmentShaderSource);

  // Demonstrate both algorithms
  std::vector<Point> bresenhamPoints = getBresenhamLine(100, 500, 700, 100);
  std::vector<Point> ddaPoints = getDDALine(100, 500, 700, 100);

  std::cout << "Bresenham points count: " << bresenhamPoints.size() << std::endl;
  std::cout << "DDA points count: " << ddaPoints.size() << std::endl;

  // Combine points for visual demonstration (offset DDA slightly to show difference)
  std::vector<Point> combinedPoints = bresenhamPoints;
  for (const auto &p : ddaPoints) {
    combinedPoints.push_back({p.x, p.y + 10});
  }

  std::vector<float> vertices;
  for (const auto &p : combinedPoints) {
    vertices.push_back(static_cast<float>(p.x));
    vertices.push_back(static_cast<float>(p.y));
  }

  GLuint VAO, VBO;
  glGenVertexArrays(1, &VAO);
  glGenBuffers(1, &VBO);

  glBindVertexArray(VAO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);

  glEnable(GL_PROGRAM_POINT_SIZE);

  glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(WINDOW_WIDTH), 0.0f,
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
    glDrawArrays(GL_POINTS, 0, vertices.size() / 2);

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glfwDestroyWindow(window);
  glfwTerminate();
  return 0;
}
