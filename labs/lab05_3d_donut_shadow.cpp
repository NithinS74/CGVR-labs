#include <epoxy/gl.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cmath>
#include <iostream>
#include <vector>

// --- 1. SHADER SOURCES ---
const char *shadowVertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
uniform mat4 model;
uniform mat4 lightSpaceMatrix;
void main() {
    gl_Position = lightSpaceMatrix * model * vec4(aPos, 1.0);
}
)";

const char *shadowFragmentShaderSource = R"(
#version 330 core
void main() {}
)";

const char *sceneVertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

out vec3 FragPos;
out vec3 Normal;
out vec4 FragPosLightSpace;
out vec3 LocalPos; 

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 lightSpaceMatrix;

void main() {
    LocalPos = aPos; 
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    FragPosLightSpace = lightSpaceMatrix * vec4(FragPos, 1.0);
        
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
)";

const char *sceneFragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec4 FragPosLightSpace;
in vec3 LocalPos;

uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 objectColor;
uniform bool isDonut;

// Changed to sampler2DShadow for Hardware PCF filtering
uniform sampler2DShadow shadowMap;

float ShadowCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir) {
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    
    if(projCoords.z > 1.0) return 0.0;
    
    float bias = max(0.005 * (1.0 - dot(normal, lightDir)), 0.001);
    float currentDepth = projCoords.z - bias;
    
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    float visibility = 0.0;

    // 16-Tap Poisson Disk to eliminate banding/blocky artifacts
    vec2 poissonDisk[16] = vec2[]( 
        vec2( -0.94201624, -0.39906216 ), vec2( 0.94558609, -0.76890725 ), 
        vec2( -0.094184101, -0.92938870 ), vec2( 0.34495938, 0.29387760 ), 
        vec2( -0.91588401, 0.45771436 ), vec2( -0.81544232, -0.87912464 ), 
        vec2( -0.38277543, 0.27676845 ), vec2( 0.97484398, 0.75648379 ), 
        vec2( 0.44323325, -0.97511554 ), vec2( 0.53742981, -0.47373420 ), 
        vec2( -0.26496911, -0.41893023 ), vec2( 0.79197514, 0.19090188 ), 
        vec2( -0.24188840, 0.99706507 ), vec2( -0.81409955, 0.91437590 ), 
        vec2( 0.19984126, 0.78641367 ), vec2( 0.14383161, -0.14100790 ) 
    );
    
    float spread = 2.0; // Increase this value to make the shadows softer

    for(int i = 0; i < 16; i++) {
        // sampler2DShadow returns an interpolated float between 0.0 (shadowed) and 1.0 (lit)
        visibility += texture(shadowMap, vec3(projCoords.xy + poissonDisk[i] * texelSize * spread, currentDepth));
    }
    
    // Invert visibility to match your original formula (where 1.0 meant fully shadowed)
    return 1.0 - (visibility / 16.0);
}

void main() {
    vec3 color = objectColor;
        
    // PROCEDURAL ICING GENERATION
    if (isDonut) {
        vec3 doughColor = vec3(0.9, 0.65, 0.35); 
        vec3 icingColor = vec3(0.95, 0.4, 0.6);  
                
        float dripWave = sin(LocalPos.x * 12.0) * cos(LocalPos.z * 12.0) * 0.08;
        float icingThreshold = 0.05 + dripWave;
        float icingMix = smoothstep(icingThreshold - 0.02, icingThreshold + 0.02, LocalPos.y);
                
        color = mix(doughColor, icingColor, icingMix);
    }

    // Standard Blinn-Phong Lighting
    vec3 normal = normalize(Normal);
    vec3 lightColor = vec3(1.0, 0.95, 0.9);
    vec3 lightDir = normalize(lightPos - FragPos);
    
    vec3 ambient = 0.3 * color;
    
    float diff = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = diff * lightColor;
    
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);  
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);
        
    float specStrength = (isDonut && LocalPos.y > 0.0) ? 0.6 : 0.1;
    vec3 specular = specStrength * spec * lightColor;
        
    float shadow = ShadowCalculation(FragPosLightSpace, normal, lightDir);       
    
    vec3 lighting = (ambient + (1.0 - shadow) * (diffuse + specular)) * color;
            
    FragColor = vec4(lighting, 1.0);
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

void generateTorus(float mainRadius, float tubeRadius, int mainSegments,
                   int tubeSegments, std::vector<float> &vertices,
                   std::vector<unsigned int> &indices) {
  const float PI = 3.14159265359f;
  for (int i = 0; i <= mainSegments; ++i) {
    float u = (float)i / mainSegments * 2.0f * PI;
    for (int j = 0; j <= tubeSegments; ++j) {
      float v = (float)j / tubeSegments * 2.0f * PI;
      float x = (mainRadius + tubeRadius * std::cos(v)) * std::cos(u);
      float y = tubeRadius * std::sin(v);
      float z = (mainRadius + tubeRadius * std::cos(v)) * std::sin(u);
      float nx = std::cos(v) * std::cos(u);
      float ny = std::sin(v);
      float nz = std::cos(v) * std::sin(u);
      vertices.insert(vertices.end(), {x, y, z, nx, ny, nz});
    }
  }
  for (int i = 0; i < mainSegments; ++i) {
    for (int j = 0; j < tubeSegments; ++j) {
      unsigned int current = i * (tubeSegments + 1) + j;
      unsigned int next = (i + 1) * (tubeSegments + 1) + j;
      indices.insert(indices.end(),
                     {current, next, current + 1, next, next + 1, current + 1});
    }
  }
}

void renderScene(GLuint shader, GLuint donutVAO, int donutIndices,
                 GLuint floorVAO, glm::mat4 donutModel) {
  // 1. Draw Floor
  glm::mat4 floorModel = glm::mat4(1.0f);
  floorModel = glm::translate(floorModel, glm::vec3(0.0f, -1.2f, 0.0f));
  glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE,
                     glm::value_ptr(floorModel));
  glUniform3fv(glGetUniformLocation(shader, "objectColor"), 1,
               glm::value_ptr(glm::vec3(0.7f, 0.7f, 0.7f)));
  glUniform1i(glGetUniformLocation(shader, "isDonut"), 0);
  glBindVertexArray(floorVAO);
  glDrawArrays(GL_TRIANGLES, 0, 6);

  // 2. Draw Donut
  glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE,
                     glm::value_ptr(donutModel));
  glUniform1i(glGetUniformLocation(shader, "isDonut"), 1);
  glBindVertexArray(donutVAO);
  glDrawElements(GL_TRIANGLES, donutIndices, GL_UNSIGNED_INT, 0);
}

// --- 3. MAIN PROGRAM ---
int main() {
  if (!glfwInit())
    return -1;
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  // --- ENABLE ANTI-ALIASING ---
  glfwWindowHint(GLFW_SAMPLES, 4);
  GLFWwindow *window =
      glfwCreateWindow(800, 600, "Anti-Aliased Pink Donut", nullptr, nullptr);
  if (!window) {
    glfwTerminate();
    return -1;
  }
  glfwMakeContextCurrent(window);

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_MULTISAMPLE);

  // Setup Shaders and Geometry
  GLuint shadowProgram =
      createProgram(shadowVertexShaderSource, shadowFragmentShaderSource);
  GLuint sceneProgram =
      createProgram(sceneVertexShaderSource, sceneFragmentShaderSource);

  std::vector<float> dVerts;
  std::vector<unsigned int> dInds;
  generateTorus(1.0f, 0.45f, 64, 32, dVerts, dInds);

  GLuint donutVAO, dVBO, dEBO;
  glGenVertexArrays(1, &donutVAO);
  glGenBuffers(1, &dVBO);
  glGenBuffers(1, &dEBO);
  glBindVertexArray(donutVAO);
  glBindBuffer(GL_ARRAY_BUFFER, dVBO);
  glBufferData(GL_ARRAY_BUFFER, dVerts.size() * sizeof(float), dVerts.data(),
               GL_STATIC_DRAW);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, dEBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, dInds.size() * sizeof(unsigned int),
               dInds.data(), GL_STATIC_DRAW);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                        (void *)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);

  float floorVertices[] = {
      25.0f, 0.0f, 25.0f, 0.0f,   1.0f, 0.0f,   -25.0f, 0.0f, 25.0f,
      0.0f,  1.0f, 0.0f,  -25.0f, 0.0f, -25.0f, 0.0f,   1.0f, 0.0f,
      25.0f, 0.0f, 25.0f, 0.0f,   1.0f, 0.0f,   -25.0f, 0.0f, -25.0f,
      0.0f,  1.0f, 0.0f,  25.0f,  0.0f, -25.0f, 0.0f,   1.0f, 0.0f};

  GLuint floorVAO, fVBO;
  glGenVertexArrays(1, &floorVAO);
  glGenBuffers(1, &fVBO);
  glBindVertexArray(floorVAO);
  glBindBuffer(GL_ARRAY_BUFFER, fVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(floorVertices), floorVertices,
               GL_STATIC_DRAW);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                        (void *)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);

  const unsigned int SHADOW_WIDTH = 2048, SHADOW_HEIGHT = 2048;
  GLuint depthMapFBO, depthMapTexture;
  glGenFramebuffers(1, &depthMapFBO);
  glGenTextures(1, &depthMapTexture);
  glBindTexture(GL_TEXTURE_2D, depthMapTexture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH,
               SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

  // --- ENABLE HARDWARE PCF (Shadow Anti-Aliasing) ---
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                  GL_LINEAR); // Required for hardware PCF
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE,
                  GL_COMPARE_REF_TO_TEXTURE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
  float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
  glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

  glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
                         depthMapTexture, 0);
  glDrawBuffer(GL_NONE);
  glReadBuffer(GL_NONE);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  glm::vec3 lightPos(-2.0f, 5.0f, -1.0f);
  glm::vec3 cameraPos(0.0f, 2.5f, 4.5f);

  while (!glfwWindowShouldClose(window)) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
      glfwSetWindowShouldClose(window, true);

    float time = (float)glfwGetTime();

    glm::mat4 donutModel = glm::mat4(1.0f);
    donutModel = glm::translate(donutModel,
                                glm::vec3(0.0f, std::sin(time) * 0.3f, 0.0f));
    donutModel = glm::rotate(donutModel, time * glm::radians(35.0f),
                             glm::vec3(0.4f, 1.0f, 0.1f));

    glm::mat4 lightProjection =
        glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, 1.0f, 15.0f);
    glm::mat4 lightView =
        glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0.0, 1.0, 0.0));
    glm::mat4 lightSpaceMatrix = lightProjection * lightView;

    // Pass 1: Depth Map
    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT);
    glUseProgram(shadowProgram);
    glUniformMatrix4fv(glGetUniformLocation(shadowProgram, "lightSpaceMatrix"),
                       1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));
    renderScene(shadowProgram, donutVAO, dInds.size(), floorVAO, donutModel);

    // Pass 2: Main Render
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    int w, h;
    glfwGetFramebufferSize(window, &w, &h);
    glViewport(0, 0, w, h);
    glClearColor(0.2f, 0.25f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(sceneProgram);

    glm::mat4 projection = glm::perspective(glm::radians(45.0f),
                                            (float)w / (float)h, 0.1f, 100.0f);
    glm::mat4 view =
        glm::lookAt(cameraPos, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    glUniformMatrix4fv(glGetUniformLocation(sceneProgram, "projection"), 1,
                       GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(glGetUniformLocation(sceneProgram, "view"), 1, GL_FALSE,
                       glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(sceneProgram, "lightSpaceMatrix"),
                       1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));
    glUniform3fv(glGetUniformLocation(sceneProgram, "viewPos"), 1,
                 glm::value_ptr(cameraPos));
    glUniform3fv(glGetUniformLocation(sceneProgram, "lightPos"), 1,
                 glm::value_ptr(lightPos));

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, depthMapTexture);
    glUniform1i(glGetUniformLocation(sceneProgram, "shadowMap"), 0);

    renderScene(sceneProgram, donutVAO, dInds.size(), floorVAO, donutModel);

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glfwDestroyWindow(window);
  glfwTerminate();
  return 0;
}
