#include "gl.h"
#include <GLFW/glfw3.h>

#include <cmath>
#include <vector>
#include <array>
#include <cassert>
#include <iostream>
#include <sstream>

#include <vecmath.h>
#include "starter0_util.h"
#include "recorder.h"
#include "teapot.h"

using namespace std;

// Globals
uint32_t program;
int color_idx{0};
int color_n{4};
GLfloat lightPos[] = {2.0f, 3.0f, 5.0f, 1.0f};

// This is the list of points (3D vectors)
vector<Vector3f> vecv;

// This is the list of normals (also 3D vectors)
vector<Vector3f> vecn;

// This is the list of faces (indices into vecv and vecn)
vector<vector<unsigned>> vecf;

// You will need more global variables to implement color and position changes


void keyCallback(GLFWwindow *window, int key,
                 int scancode, int action, int mods) {
  if (action == GLFW_RELEASE) { // only handle PRESS and REPEAT
    return;
  }

  // Special keys (arrows, CTRL, ...) are documented
  // here: http://www.glfw.org/docs/latest/group__keys.html
  if (key == GLFW_KEY_ESCAPE) {
    glfwSetWindowShouldClose(window, GLFW_TRUE);
  } else if (key == 'A') {
    printf("Key A pressed\n");
  } else if (key == 'C') {
    color_idx = ((color_idx) + 1) % color_n;
  } else if (key == GLFW_KEY_LEFT) {
    lightPos[0] -= 0.5;
  } else if (key == GLFW_KEY_RIGHT) {
    lightPos[0] += 0.5;
  } else if (key == GLFW_KEY_UP) {
    lightPos[1] += 0.5;
  } else if (key == GLFW_KEY_DOWN) {
    lightPos[1] -= 0.5;
  } else {
    printf("Unhandled key press %d\n", key);
  }
}

void drawTriangle() {
  // set a reasonable upper limit for the buffer size
  GeometryRecorder rec(1024);
  rec.record(Vector3f(0.0, 0.0, 0.0), // Position
             Vector3f(0.0, 0.0, 1.0));// Normal

  rec.record(Vector3f(3.0, 0.0, 0.0),
             Vector3f(0.0, 0.0, 1.0));

  rec.record(Vector3f(3.0, 3.0, 0.0),
             Vector3f(0.0, 0.0, 1.0));
  rec.draw();
}

void drawTeapot() {
  // set the required buffer size exactly.
  GeometryRecorder rec(teapot_num_faces * 3);
  for (int idx : teapot_indices) {
    Vector3f position(teapot_positions[idx * 3 + 0],
                      teapot_positions[idx * 3 + 1],
                      teapot_positions[idx * 3 + 2]);

    Vector3f normal(teapot_normals[idx * 3 + 0],
                    teapot_normals[idx * 3 + 1],
                    teapot_normals[idx * 3 + 2]);

    rec.record(position, normal);
  }
  rec.draw();
}

void drawObjMesh() {
  // draw obj mesh here
  // read vertices and face indices from vecv, vecn, vecf
  auto n = vecf.size();
  GeometryRecorder rec{int(n) * 3};
  for (auto &f : vecf) {
    for (auto i = 0; i < 3; i++) {
      rec.record(vecv[f[i * 2]], vecn[f[i * 2 + 1]]);
    }
  }
  rec.draw();
}

// This function is responsible for displaying the object.
void drawScene() {
  drawObjMesh();
//  drawTeapot();
}

void setViewport(GLFWwindow *window) {
  int width, height;
  glfwGetFramebufferSize(window, &width, &height);

  // make sure the viewport is square-shaped.
  if (width > height) {
    int offsetx = (width - height) / 2;
    glViewport(offsetx, 0, height, height);
  } else {
    int offsety = (height - width) / 2;
    glViewport(0, offsety, width, width);
  }
}

void updateCameraUniforms() {
  // Set up a perspective view, with square aspect ratio
  float fovy_radians = deg2rad(50.0f);
  float nearz = 1.0f;
  float farz = 100.0f;
  float aspect = 1.0f;
  Matrix4f P = Matrix4f::perspectiveProjection(
      fovy_radians, aspect, nearz, farz);

  // See https://www.opengl.org/sdk/docs/man/html/glUniform.xhtml
  // for the many version of glUniformXYZ()
  // Returns -1 if uniform not found.
  int loc = glGetUniformLocation(program, "P");
  glUniformMatrix4fv(loc, 1, false, P);

  Vector3f eye(0.0, 0.0, 7.0f);
  Vector3f center(0.0, 0.0, 0.0);
  Vector3f up(0.0, 1.0f, -0.2f);
  Matrix4f V = Matrix4f::lookAt(eye, center, up);
  loc = glGetUniformLocation(program, "V");
  glUniformMatrix4fv(loc, 1, false, V);
  loc = glGetUniformLocation(program, "camPos");
  glUniform3fv(loc, 1, eye);

  // Make sure the model is centered in the viewport
  // We translate the model using the "Model" matrix
  Matrix4f M = Matrix4f::translation(0, -2.0, 0);
  loc = glGetUniformLocation(program, "M");
  glUniformMatrix4fv(loc, 1, false, M);

  // Transformation matrices act differently
  // on vectors than on points.
  // The inverse-transpose is what we want.
  Matrix4f N = M.inverse().transposed();
  loc = glGetUniformLocation(program, "N");
  glUniformMatrix4fv(loc, 1, false, N);
}

void updateMaterialUniforms() {
  // Here are some colors you might use - feel free to add more
  GLfloat diffColors[4][4] = {
      {0.5f, 0.5f, 0.9f, 1.0f},
      {0.9f, 0.5f, 0.5f, 1.0f},
      {0.5f, 0.9f, 0.3f, 1.0f},
      {0.3f, 0.8f, 0.9f, 1.0f}};

  // Here we use the first color entry as the diffuse color
  int loc = glGetUniformLocation(program, "diffColor");
  glUniform4fv(loc, 1, diffColors[color_idx]);

  // Define specular color and shininess
  GLfloat specColor[] = {0.2f, 0.2f, 0.2f, 1.0f};
  GLfloat shininess[] = {10.0f};

  // Note that the specular color and shininess can stay constant
  loc = glGetUniformLocation(program, "specColor");
  glUniform4fv(loc, 1, specColor);
  loc = glGetUniformLocation(program, "shininess");
  glUniform1f(loc, shininess[0]);
}

void updateLightUniforms() {
  // Light Position
  int loc = glGetUniformLocation(program, "lightPos");
  glUniform4fv(loc, 1, lightPos);

  // Light Color
  GLfloat lightDiff[] = {120.0, 120.0, 120.0, 1.0};
  loc = glGetUniformLocation(program, "lightDiff");
  glUniform4fv(loc, 1, lightDiff);
}

void loadInput() {
  // load the OBJ file here
  string s;
  Vector3f v;
  unsigned idx;
  while (getline(cin, s)) {
    stringstream ss{s};
    ss >> s;
    if (s == "v") {
      ss >> v[0] >> v[1] >> v[2];
      vecv.emplace_back(v);
    } else if (s == "vn") {
      ss >> v[0] >> v[1] >> v[2];
      vecn.emplace_back(v);
    } else if (s == "f") {
      int cnt{0};
      vecf.emplace_back();
      while (ss >> idx) {
        if (cnt % 3 != 1) {
          vecf.back().push_back(idx - 1);
        }
        ss.get();
        ++cnt;
      }
    }
  }
}

// Main routine.
// Set up OpenGL, define the callbacks and start the main loop
int main(int argc, char **argv) {
  loadInput();

  GLFWwindow *window = createOpenGLWindow(640, 480, "a0");

  // setup the keyboard event handler
  glfwSetKeyCallback(window, keyCallback);

  // glEnable() and glDisable() control parts of OpenGL's
  // fixed-function pipeline, such as rasterization, or
  // depth-buffering. What happens if you remove the next line?
  glEnable(GL_DEPTH_TEST);

  // The program object controls the programmable parts
  // of OpenGL. All OpenGL programs define a vertex shader
  // and a fragment shader.
  program = compileProgram(c_vertexshader, c_fragmentshader);
  if (!program) {
    printf("Cannot compile program\n");
    return -1;
  }

  glUseProgram(program);

  // Main Loop
  while (!glfwWindowShouldClose(window)) {
    // Clear the rendering window
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    setViewport(window);

    updateCameraUniforms();
    updateLightUniforms();
    updateMaterialUniforms();

    // Draw to back buffer
    drawScene();

    // Make back buffer visible
    glfwSwapBuffers(window);

    // Check if any input happened during the last frame
    glfwPollEvents();
  }

  // All OpenGL resource that are created with
  // glGen* or glCreate* must be freed.
  glDeleteProgram(program);

  glfwTerminate(); // destroy the window
  return 0;
}
