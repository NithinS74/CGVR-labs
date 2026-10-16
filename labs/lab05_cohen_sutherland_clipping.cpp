// 1. Epoxy MUST be included first
#include <epoxy/gl.h>

// 2. Prevent GLFW from including standard GL headers
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

// 3. C++ Standard Library headers
#include <cmath>
#include <iostream>
#include <vector>

// Window dimensions
const int WIDTH = 800;
const int HEIGHT = 600;

// Clipping window (in pixel coordinates)
const int X_MIN = 150;
const int X_MAX = 650;
const int Y_MIN = 150;
const int Y_MAX = 450;

// Outcode bits: Top=1, Bottom=2, Right=4, Left=8
int computeOutcode(int x, int y) {
  int code = 0;
  if (y < Y_MIN)       code |= 1;  // Top
  else if (y > Y_MAX)  code |= 2;  // Bottom
  if (x > X_MAX)       code |= 4;  // Right
  else if (x < X_MIN)  code |= 8;  // Left
  return code;
}

// Cohen-Sutherland line clipping algorithm
bool cohenSutherlandClip(float x0, float y0, float x1, float y1,
                          float &cx0, float &cy0, float &cx1, float &cy1) {
  int outcode0 = computeOutcode((int)std::round(x0), (int)std::round(y0));
  int outcode1 = computeOutcode((int)std::round(x1), (int)std::round(y1));

  while (true) {
    if (!(outcode0 | outcode1)) {
      // Both endpoints inside clipping window - fully visible
      cx0 = x0; cy0 = y0; cx1 = x1; cy1 = y1;
      return true;
    } else if (outcode0 & outcode1) {
      // Both endpoints share an outside region - line is fully outside, reject
      return false;
    } else {
      // At least one endpoint is outside - clip the line
      int outcodeOut = (outcode0 != 0) ? outcode0 : outcode1;
      float x = 0.0f, y = 0.0f;

      if (outcodeOut & 1) { // Top edge
        y = (float)Y_MIN;
        x = x0 + (x1 - x0) * ((float)Y_MIN - y0) / (y1 - y0);
      } else if (outcodeOut & 2) { // Bottom edge
        y = (float)Y_MAX;
        x = x0 + (x1 - x0) * ((float)Y_MAX - y0) / (y1 - y0);
      } else if (outcodeOut & 4) { // Right edge
        x = (float)X_MAX;
        y = y0 + (y1 - y0) * ((float)X_MAX - x0) / (x1 - x0);
      } else if (outcodeOut & 8) { // Left edge
        x = (float)X_MIN;
        y = y0 + (y1 - y0) * ((float)X_MIN - x0) / (x1 - x0);
      }

      if (outcodeOut == outcode0) {
        x0 = x; y0 = y;
        outcode0 = computeOutcode((int)std::round(x0), (int)std::round(y0));
      } else {
        x1 = x; y1 = y;
        outcode1 = computeOutcode((int)std::round(x1), (int)std::round(y1));
      }
    }
  }
}

// Convert pixel coordinate to NDC [-1, 1]
float toNDCX(int px) { return (2.0f * px) / WIDTH - 1.0f; }
float toNDCY(int py) { return 1.0f - (2.0f * py) / HEIGHT; }

// Generate line points using Bresenham's algorithm for visualization
std::vector<float> generateLinePoints(float x0, float y0, float x1, float y1) {
  std::vector<float> pts;
  int ix0 = (int)std::round(x0);
  int iy0 = (int)std::round(y0);
  int ix1 = (int)std::round(x1);
  int iy1 = (int)std::round(y1);

  int dx = std::abs(ix1 - ix0);
  int dy = std::abs(iy1 - iy0);
  int sx = (ix0 < ix1) ? 1 : -1;
  int sy = (iy0 < iy1) ? 1 : -1;
  int err = dx - dy;

  int x = ix0, y = iy0;
  while (true) {
    pts.push_back(toNDCX(x));
    pts.push_back(toNDCY(y));
    if (x == ix1 && y == iy1) break;
    int e2 = 2 * err;
    if (e2 > -dy) { err -= dy; x += sx; }
    if (e2 < dx) { err += dx; y += sy; }
  }
  return pts;
}

// Simple vertex shader
const char *vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
void main() {
    gl_Position = vec4(aPos, 0.0, 1.0);
}
)";

// Red fragment shader (original line)
const char *fragmentShaderRed = R"(
#version 330 core
out vec4 FragColor;
void main() {
    FragColor = vec4(1.0, 0.25, 0.25, 1.0); // Red
}
)";

// Green fragment shader (clipped result)
const char *fragmentShaderGreen = R"(
#version 330 core
out vec4 FragColor;
void main() {
    FragColor = vec4(0.2, 0.85, 0.3, 1.0); // Green
}
)";

// White fragment shader (clipping window border)
const char *fragmentShaderWhite = R"(
#version 330 core
out vec4 FragColor;
void main() {
    FragColor = vec4(1.0, 1.0, 1.0, 1.0); // White
}
)";

GLuint compileShader(GLenum type, const char *source) {
  GLuint shader = glCreateShader(type);
  glShaderSource(shader, 1, &source, NULL);
  glCompileShader(shader);
  GLint success;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
  if (!success) {
    char infoLog[512];
    glGetShaderInfoLog(shader, 512, NULL, infoLog);
    std::cerr << "Shader Compilation Error:\n" << infoLog << std::endl;
  }
  return shader;
}

GLuint createProgram(const char *vertSrc, const char *fragSrc) {
  GLuint vert = compileShader(GL_VERTEX_SHADER, vertSrc);
  GLuint frag = compileShader(GL_FRAGMENT_SHADER, fragSrc);
  GLuint program = glCreateProgram();
  glAttachShader(program, vert);
  glAttachShader(program, frag);
  glLinkProgram(program);
  glDeleteShader(vert);
  glDeleteShader(frag);
  return program;
}

// Helper to draw points
void drawPoints(GLuint program, GLuint VAO, GLuint VBO,
                const std::vector<float>& verts, float pointSize) {
  glPointSize(pointSize);
  glBindVertexArray(VAO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float),
               verts.data(), GL_STATIC_DRAW);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);
  glUseProgram(program);
  glDrawArrays(GL_POINTS, 0, verts.size() / 2);
}

int main() {
  if (!glfwInit()) {
    std::cerr << "Failed to initialize GLFW" << std::endl;
    return -1;
  }
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT,
      "Cohen-Sutherland Line Clipping - OpenGL", NULL, NULL);
  if (!window) {
    std::cerr << "Failed to create GLFW window" << std::endl;
    glfwTerminate();
    return -1;
  }
  glfwMakeContextCurrent(window);

  // Define an original line that crosses the clipping window
  float ox0 = 50.0f, oy0 = 300.0f;   // Outside left
  float ox1 = 700.0f, oy1 = 550.0f;  // Outside bottom-right

  // Apply Cohen-Sutherland clipping
  float cx0, cy0, cx1, cy1;
  bool clipped = cohenSutherlandClip(ox0, oy0, ox1, oy1, cx0, cy0, cx1, cy1);

  std::cout << "Cohen-Sutherland Clipping Result: " << (clipped ? "CLIPPED" : "REJECTED") << std::endl;
  if (clipped) {
    std::cout << "  Original: (" << ox0 << "," << oy0 << ") -> (" << ox1 << "," << oy1 << ")" << std::endl;
    std::cout << "  Clipped: (" << cx0 << "," << cy0 << ") -> (" << cx1 << "," << cy1 << ")" << std::endl;
  }

  GLuint programRed   = createProgram(vertexShaderSource, fragmentShaderRed);
  GLuint programGreen = createProgram(vertexShaderSource, fragmentShaderGreen);
  GLuint programWhite = createProgram(vertexShaderSource, fragmentShaderWhite);

  GLuint VAO, VBO;
  glGenVertexArrays(1, &VAO);
  glGenBuffers(1, &VBO);

  while (!glfwWindowShouldClose(window)) {
    glClearColor(0.05f, 0.05f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // Draw clipping window border (white rectangle outline)
    // We'll draw four line segments for the border using points
    std::vector<float> borderPts;
    // Top edge
    for (int x = X_MIN; x <= X_MAX; ++x) {
      borderPts.push_back(toNDCX(x));
      borderPts.push_back(toNDCY(Y_MIN));
    }
    // Bottom edge
    for (int x = X_MIN; x <= X_MAX; ++x) {
      borderPts.push_back(toNDCX(x));
      borderPts.push_back(toNDCY(Y_MAX));
    }
    // Left edge
    for (int y = Y_MIN; y <= Y_MAX; ++y) {
      borderPts.push_back(toNDCX(X_MIN));
      borderPts.push_back(toNDCY(y));
    }
    // Right edge
    for (int y = Y_MIN; y <= Y_MAX; ++y) {
      borderPts.push_back(toNDCX(X_MAX));
      borderPts.push_back(toNDCY(y));
    }
    drawPoints(programWhite, VAO, VBO, borderPts, 2.0f);

    // Draw original line (red) - always visible even if partially outside
    std::vector<float> origLine = generateLinePoints(ox0, oy0, ox1, oy1);
    drawPoints(programRed, VAO, VBO, origLine, 2.5f);

    // Draw clipped result (green) - only visible portion inside window
    if (clipped) {
      std::vector<float> clippedLine = generateLinePoints(cx0, cy0, cx1, cy1);
      drawPoints(programGreen, VAO, VBO, clippedLine, 3.5f);
    }

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glDeleteVertexArrays(1, &VAO);
  glDeleteBuffers(1, &VBO);
  glDeleteProgram(programRed);
  glDeleteProgram(programGreen);
  glDeleteProgram(programWhite);

  glfwDestroyWindow(window);
  glfwTerminate();
  return 0;
}
