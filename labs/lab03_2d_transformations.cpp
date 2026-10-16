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
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec3 aColor;

out vec3 ourColor;

uniform mat4 model;
uniform mat4 projection;

void main() {
    gl_Position = projection * model * vec4(aPos, 0.0, 1.0);
    ourColor = aColor;
}
)";

const char *fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;
in vec3 ourColor;
void main() {
    FragColor = vec4(ourColor, 1.0);
}
)";

// --- HELPER FUNCTIONS ---
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

// --- SHAPE DATA ---
// Rectangle centered near origin, in pixel-space for ortho projection
float rectVertices[] = {
    // Position      // Color (RGB)
    -50.0f, -30.0f,  1.0f, 0.0f, 0.0f,   // bottom-left (red)
     50.0f, -30.0f,  0.0f, 1.0f, 0.0f,   // bottom-right (green)
     50.0f,  30.0f,  0.0f, 0.0f, 1.0f,   // top-right (blue)
    -50.0f,  30.0f,  1.0f, 1.0f, 0.0f,   // top-left (yellow)
};

unsigned int rectIndices[] = {
    0, 1, 2,
    2, 3, 0
};

int main() {
  if (!glfwInit())
    return -1;
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  const int WINDOW_WIDTH = 800;
  const int WINDOW_HEIGHT = 600;
  GLFWwindow *window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT,
                                      "2D Transformations - Lab03", nullptr, nullptr);
  if (!window) {
    glfwTerminate();
    return -1;
  }
  glfwMakeContextCurrent(window);

  GLuint shaderProgram = createProgram(vertexShaderSource, fragmentShaderSource);

  GLuint VAO, VBO, EBO;
  glGenVertexArrays(1, &VAO);
  glGenBuffers(1, &VBO);
  glGenBuffers(1, &EBO);

  glBindVertexArray(VAO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(rectVertices), rectVertices, GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(rectIndices), rectIndices, GL_STATIC_DRAW);

  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);

  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                        (void *)(2 * sizeof(float)));
  glEnableVertexAttribArray(1);

  glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(WINDOW_WIDTH),
                                    0.0f, static_cast<float>(WINDOW_HEIGHT), -1.0f, 1.0f);

  while (!glfwWindowShouldClose(window)) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
      glfwSetWindowShouldClose(window, true);

    int w, h;
    glfwGetFramebufferSize(window, &w, &h);
    glViewport(0, 0, w, h);

    glClearColor(0.05f, 0.05f, 0.08f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(shaderProgram);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1,
                       GL_FALSE, glm::value_ptr(projection));

    glBindVertexArray(VAO);

    float time = glfwGetTime();

    // --- 1. ORIGINAL RECTANGLE (no transformation) ---
    {
      glm::mat4 model = glm::mat4(1.0f);
      // Place at left side
      model = glm::translate(model, glm::vec3(150.0f, 300.0f, 0.0f));
      glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1,
                         GL_FALSE, glm::value_ptr(model));
      glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    }

    // --- 2. TRANSLATION ---
    // Shift rectangle to the right by 200 units horizontally, same y
    {
      glm::mat4 model = glm::mat4(1.0f);
      model = glm::translate(model, glm::vec3(400.0f, 300.0f, 0.0f));
      glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1,
                         GL_FALSE, glm::value_ptr(model));
      glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    }

    // --- 3. ROTATION (homogeneous coordinates) ---
    // Rotate 45 degrees around the z-axis, placed at center-bottom area
    {
      glm::mat4 model = glm::mat4(1.0f);
      model = glm::translate(model, glm::vec3(550.0f, 150.0f, 0.0f));
      model = glm::rotate(model, glm::radians(45.0f), glm::vec3(0.0f, 0.0f, 1.0f));
      glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1,
                         GL_FALSE, glm::value_ptr(model));
      glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    }

    // --- 4. SCALING ---
    // Scale rectangle by 1.5x horizontally and 0.6x vertically
    {
      glm::mat4 model = glm::mat4(1.0f);
      model = glm::translate(model, glm::vec3(200.0f, 450.0f, 0.0f));
      model = glm::scale(model, glm::vec3(1.5f, 0.6f, 1.0f));
      glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1,
                         GL_FALSE, glm::value_ptr(model));
      glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    }

    // --- 5. REFLECTION (scaling by -1) ---
    // Reflect across y-axis (scale x by -1)
    {
      glm::mat4 model = glm::mat4(1.0f);
      model = glm::translate(model, glm::vec3(550.0f, 450.0f, 0.0f));
      model = glm::scale(model, glm::vec3(-1.0f, 1.0f, 1.0f));
      glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1,
                         GL_FALSE, glm::value_ptr(model));
      glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    }

    // --- 6. SHEARING ---
    // Shear along x-axis: y' = y + k*x, k = 0.5
    // Using homogeneous shear matrix:
    // [1, k, 0, 0]
    // [0, 1, 0, 0]
    // [0, 0, 1, 0]
    // [0, 0, 0, 1]
    {
      float k = 0.4f;
      glm::mat4 shear = glm::mat4(1.0f);
      shear[0][1] = k;  // shear x based on y? No, standard: x' = x + k*y
      // Actually for 2D shear along x: x' = x + k*y, y' = y
      // So column 1 gets the k in row 0
      // Let's build manually:
      shear = glm::mat4(
        1.0f, k,    0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
      );
      glm::mat4 model = glm::mat4(1.0f);
      model = glm::translate(model, glm::vec3(300.0f, 150.0f, 0.0f));
      model = model * shear;
      glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1,
                         GL_FALSE, glm::value_ptr(model));
      glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    }

    // --- 7. COMPOSITE: Rotation + Translation + Scale ---
    // Rotate 30 degrees, scale by 0.8, translate to bottom-right
    {
      glm::mat4 model = glm::mat4(1.0f);
      model = glm::translate(model, glm::vec3(500.0f, 100.0f, 0.0f));
      model = glm::rotate(model, glm::radians(30.0f), glm::vec3(0.0f, 0.0f, 1.0f));
      model = glm::scale(model, glm::vec3(0.8f, 0.8f, 1.0f));
      glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1,
                         GL_FALSE, glm::value_ptr(model));
      glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    }

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glfwDestroyWindow(window);
  glfwTerminate();
  return 0;
}
