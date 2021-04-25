#include "openglwindow.hpp"
#include <iostream>
#include <fmt/core.h>
#include <imgui.h>
#include <tiny_obj_loader.h>

#include <cppitertools/itertools.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtx/fast_trigonometry.hpp>
#include <glm/gtx/hash.hpp>
#include <unordered_map>

#include "imfilebrowser.h"

void OpenGLWindow::handleEvent(SDL_Event& ev) {
  if (ev.type == SDL_KEYDOWN) {
    if (ev.key.keysym.sym == SDLK_UP || ev.key.keysym.sym == SDLK_w)
      m_dollySpeed = 1.0f;
    if (ev.key.keysym.sym == SDLK_DOWN || ev.key.keysym.sym == SDLK_s)
      m_dollySpeed = -1.0f;
    if (ev.key.keysym.sym == SDLK_LEFT || ev.key.keysym.sym == SDLK_a)
      m_panSpeed = -1.0f;
    if (ev.key.keysym.sym == SDLK_RIGHT || ev.key.keysym.sym == SDLK_d)
      m_panSpeed = 1.0f;
    if (ev.key.keysym.sym == SDLK_q) m_truckSpeed = -1.0f;
    if (ev.key.keysym.sym == SDLK_e) m_truckSpeed = 1.0f;
  }
  if (ev.type == SDL_KEYUP) {
    if ((ev.key.keysym.sym == SDLK_UP || ev.key.keysym.sym == SDLK_w) &&
        m_dollySpeed > 0)
      m_dollySpeed = 0.0f;
    if ((ev.key.keysym.sym == SDLK_DOWN || ev.key.keysym.sym == SDLK_s) &&
        m_dollySpeed < 0)
      m_dollySpeed = 0.0f;
    if ((ev.key.keysym.sym == SDLK_LEFT || ev.key.keysym.sym == SDLK_a) &&
        m_panSpeed < 0)
      m_panSpeed = 0.0f;
    if ((ev.key.keysym.sym == SDLK_RIGHT || ev.key.keysym.sym == SDLK_d) &&
        m_panSpeed > 0)
      m_panSpeed = 0.0f;
    if (ev.key.keysym.sym == SDLK_q && m_truckSpeed < 0) m_truckSpeed = 0.0f;
    if (ev.key.keysym.sym == SDLK_e && m_truckSpeed > 0) m_truckSpeed = 0.0f;
  }
}

void OpenGLWindow::initializeGL() {
  glClearColor(0, 0, 0, 1);
  glEnable(GL_DEPTH_TEST);

  // Create programs
  for (const auto& name : m_shaderNames) {
    auto path{getAssetsPath() + "shaders/" + name};
    auto program{createProgramFromFile(path + ".vert", path + ".frag")};
    m_programs.push_back(program);
  }

  // Load default model
  loadModel(getAssetsPath() + "uploads_files_2671085_Weed.obj"); 
  m_mappingMode = 3;  // "From mesh" option
  
  //Second object
  m_model2.loadFromFile(getAssetsPath() + "12248_Bird_v1_L2.obj");
  m_model2.setupVAO(m_programs.at(m_currentProgramIndex));

  //Third object
  m_model3.loadFromFile(getAssetsPath() + "10602_Rubber_Duck_v1_L3.obj");
  m_model3.setupVAO(m_programs.at(m_currentProgramIndex));

  resizeGL(getWindowSettings().width, getWindowSettings().height);
}

void OpenGLWindow::loadModel(std::string_view path) {
  m_model.loadDiffuseTexture(getAssetsPath() + "maps/grass-3d-model.jpg");
  m_model.loadNormalTexture(getAssetsPath() + "maps/grass-3d-model.jpg");
  m_model.loadFromFile(path);
  m_model.setupVAO(m_programs.at(m_currentProgramIndex));
  m_trianglesToDraw = m_model.getNumTriangles();

  // Use material properties from the loaded model
  m_Ka = m_model.getKa();
  m_Kd = m_model.getKd();
  m_Ks = m_model.getKs();
  m_shininess = m_model.getShininess();
}

void OpenGLWindow::paintGL() {
  update();

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glViewport(0, 0, m_viewportWidth, m_viewportHeight);

  // Use currently selected program
  const auto program{m_programs.at(m_currentProgramIndex)};
  glUseProgram(program);

  // Get location of uniform variables
  GLint viewMatrixLoc{glGetUniformLocation(program, "viewMatrix")};
  GLint projMatrixLoc{glGetUniformLocation(program, "projMatrix")};
  GLint modelMatrixLoc{glGetUniformLocation(program, "modelMatrix")};
  GLint normalMatrixLoc{glGetUniformLocation(program, "normalMatrix")};
  GLint lightDirLoc{glGetUniformLocation(program, "lightDirWorldSpace")};
  GLint shininessLoc{glGetUniformLocation(program, "shininess")};
  GLint IaLoc{glGetUniformLocation(program, "Ia")};
  GLint IdLoc{glGetUniformLocation(program, "Id")};
  GLint IsLoc{glGetUniformLocation(program, "Is")};
  GLint KaLoc{glGetUniformLocation(program, "Ka")};
  GLint KdLoc{glGetUniformLocation(program, "Kd")};
  GLint KsLoc{glGetUniformLocation(program, "Ks")};
  GLint diffuseTexLoc{glGetUniformLocation(program, "diffuseTex")};
  GLint normalTexLoc{glGetUniformLocation(program, "normalTex")};
  GLint mappingModeLoc{glGetUniformLocation(program, "mappingMode")};

  // Set uniform variables used by every scene object
  // Set uniform variables for viewMatrix and projMatrix
  // These matrices are used for every scene object
  glUniformMatrix4fv(viewMatrixLoc, 1, GL_FALSE, &m_camera.m_viewMatrix[0][0]);
  glUniformMatrix4fv(projMatrixLoc, 1, GL_FALSE, &m_camera.m_projMatrix[0][0]);
  glUniform1i(diffuseTexLoc, 0);
  glUniform1i(normalTexLoc, 1);
  glUniform1i(mappingModeLoc, m_mappingMode);

  auto lightDirRotated{m_lightDir};
  glUniform4fv(lightDirLoc, 1, &lightDirRotated.x);
  glUniform4fv(IaLoc, 1, &m_Ia.x);
  glUniform4fv(IdLoc, 1, &m_Id.x);
  glUniform4fv(IsLoc, 1, &m_Is.x);

  m_modelMatrix = glm::mat4(1.0);
    m_modelMatrix = glm::translate(m_modelMatrix, glm::vec3(0.0f, 0.0f, 1.0f));
    m_modelMatrix = glm::rotate(m_modelMatrix, glm::radians(90.0f), glm::vec3(0, 1, 0));
    m_modelMatrix = glm::scale(m_modelMatrix, glm::vec3(1.0f));

    // Set uniform variables of the current object
    glUniformMatrix4fv(modelMatrixLoc, 1, GL_FALSE, &m_modelMatrix[0][0]);

    auto modelViewMatrix{glm::mat3(m_viewMatrix * m_modelMatrix)};
    glm::mat3 normalMatrix{glm::inverseTranspose(modelViewMatrix)};
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, &normalMatrix[0][0]);

    glUniform1f(shininessLoc, m_shininess);
    glUniform4fv(KaLoc, 1, &m_Ka.x);
    glUniform4fv(KdLoc, 1, &m_Kd.x);
    glUniform4fv(KsLoc, 1, &m_Ks.x);

    m_model.render(m_trianglesToDraw);

  for(float i=0.0f;i<=10.0f;i+=0.5){
      for(float j=0.0f;j<=10.0f;j+=0.5){
      m_modelMatrix = glm::translate(m_modelMatrix, glm::vec3(i, 0.0f, j));
      m_modelMatrix = glm::rotate(m_modelMatrix, glm::radians(90.0f), glm::vec3(0, 1, 0));
      m_modelMatrix = glm::scale(m_modelMatrix, glm::vec3(1.0f));

      // Set uniform variables of the current object
      glUniformMatrix4fv(modelMatrixLoc, 1, GL_FALSE, &m_modelMatrix[0][0]);
      glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, &normalMatrix[0][0]);

      glUniform1f(shininessLoc, m_shininess);
      glUniform4fv(KaLoc, 1, &m_Ka.x);
      glUniform4fv(KdLoc, 1, &m_Kd.x);
      glUniform4fv(KsLoc, 1, &m_Ks.x);

      m_model.render(m_trianglesToDraw);
    }
  };

  m_modelMatrix = glm::translate(m_modelMatrix, glm::vec3(0.0f, 0.0f, 1.5f));
  m_modelMatrix = glm::rotate(m_modelMatrix, glm::radians(90.0f), glm::vec3(0, 1, 0));
  m_modelMatrix = glm::scale(m_modelMatrix, glm::vec3(1.0f));

  // Set uniform variables of the current object
  glUniformMatrix4fv(modelMatrixLoc, 1, GL_FALSE, &m_modelMatrix[0][0]);
  glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, &normalMatrix[0][0]);

  glUniform1f(shininessLoc, m_shininess);
  glUniform4fv(KaLoc, 1, &m_Ka.x);
  glUniform4fv(KdLoc, 1, &m_Kd.x);
  glUniform4fv(KsLoc, 1, &m_Ks.x);

  m_model.render(m_trianglesToDraw);

  m_trianglesToDraw = m_model2.getNumTriangles() + m_model.getNumTriangles() ;

  float X = m_camera.m_at.x;

  if(X < -0.5f){
    X=-0.5f;
  }else if(X>0.4f){
    X=0.4f;
  }else {
    X = m_camera.m_at.x;
  }

  m_modelMatrix2 = glm::mat4(1.0);
  m_modelMatrix2 = glm::translate(m_modelMatrix2, glm::vec3(X,0.5f,m_camera.m_eye.z - 0.5f));
  m_modelMatrix2 = glm::rotate(m_modelMatrix2, glm::radians(270.0f), glm::vec3(1, 0, 0));
  m_modelMatrix2 = glm::rotate(m_modelMatrix2, glm::radians(180.0f), glm::vec3(0, 0, 1));
  m_modelMatrix2 = glm::scale(m_modelMatrix2, glm::vec3(0.2f));

  // Set uniform variables of the current object
  glUniformMatrix4fv(modelMatrixLoc, 1, GL_FALSE, &m_modelMatrix2[0][0]);

  glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, &normalMatrix[0][0]);

  m_model2.render(m_trianglesToDraw);

  //Patinho1
  m_modelMatrix3 = glm::mat4(1.0);
  //colisão do patinho1
    if((m_camera.m_eye.z -0.5f> -3.0f && m_camera.m_eye.z -0.5f < -2.5f) || colisao1>0){
      colisao1++;
      if(colisao1>0){
        m_modelMatrix3 = glm::translate(m_modelMatrix3, glm::vec3(X + 0.3f,0.1f, m_camera.m_eye.z -0.7f));
        m_modelMatrix3 = glm::rotate(m_modelMatrix3, glm::radians(270.0f), glm::vec3(1, 0, 0));
        m_modelMatrix3 = glm::rotate(m_modelMatrix3, glm::radians(180.0f), glm::vec3(0, 0, 1));
      }
    }else {
      m_modelMatrix3 = glm::translate(m_modelMatrix3, glm::vec3(0.0f,0.1f,-3.0f));
      m_modelMatrix3 = glm::rotate(m_modelMatrix3, glm::radians(270.0f), glm::vec3(1, 0, 0));
      } 
  m_modelMatrix3 = glm::scale(m_modelMatrix3, glm::vec3(0.1f));
  glUniformMatrix4fv(modelMatrixLoc, 1, GL_FALSE, &m_modelMatrix3[0][0]);
  glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, &normalMatrix[0][0]);

  m_model3.render(m_trianglesToDraw);

  //Patinho2
  m_modelMatrix3 = glm::mat4(1.0);
  //colisão do patinho2
    if((m_camera.m_eye.z -0.5f> -4.0f && m_camera.m_eye.z -0.5f < -3.5f) || colisao2>0){
      colisao2++;
      if(colisao2>0){
        m_modelMatrix3 = glm::translate(m_modelMatrix3, glm::vec3(X+0.2f,0.1f, m_camera.m_eye.z -0.7f));
        m_modelMatrix3 = glm::rotate(m_modelMatrix3, glm::radians(270.0f), glm::vec3(1, 0, 0));
        m_modelMatrix3 = glm::rotate(m_modelMatrix3, glm::radians(180.0f), glm::vec3(0, 0, 1));
      }
    }else {
      m_modelMatrix3 = glm::translate(m_modelMatrix3, glm::vec3(0.0f,0.1f,-4.0f));
      m_modelMatrix3 = glm::rotate(m_modelMatrix3, glm::radians(270.0f), glm::vec3(1, 0, 0));
      } 
  m_modelMatrix3 = glm::scale(m_modelMatrix3, glm::vec3(0.1f));
  glUniformMatrix4fv(modelMatrixLoc, 1, GL_FALSE, &m_modelMatrix3[0][0]);
  glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, &normalMatrix[0][0]);

  m_model3.render(m_trianglesToDraw);

    //Patinho3
  m_modelMatrix3 = glm::mat4(1.0);
  //colisão do patinho3
    if((m_camera.m_eye.z -0.5f> -5.0f && m_camera.m_eye.z -0.5f < -4.5f) || colisao3>0){
      colisao3++;
      if(colisao3>0){
        m_modelMatrix3 = glm::translate(m_modelMatrix3, glm::vec3(X+0.1f,0.1f, m_camera.m_eye.z -0.7f));
        m_modelMatrix3 = glm::rotate(m_modelMatrix3, glm::radians(270.0f), glm::vec3(1, 0, 0));
        m_modelMatrix3 = glm::rotate(m_modelMatrix3, glm::radians(180.0f), glm::vec3(0, 0, 1));
      }
    }else {
      m_modelMatrix3 = glm::translate(m_modelMatrix3, glm::vec3(0.0f,0.1f,-5.0f));
      m_modelMatrix3 = glm::rotate(m_modelMatrix3, glm::radians(270.0f), glm::vec3(1, 0, 0));
      } 
  m_modelMatrix3 = glm::scale(m_modelMatrix3, glm::vec3(0.1f));


        // Set uniform variables of the current object
  glUniformMatrix4fv(modelMatrixLoc, 1, GL_FALSE, &m_modelMatrix3[0][0]);

  glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, &normalMatrix[0][0]);

  m_model3.render(m_trianglesToDraw);
  
  if(colisao1 > 0 && colisao2 > 0 && colisao3 > 0){
    win = true;
  }

  glUseProgram(0);
}

void OpenGLWindow::paintUI() {
  abcg::OpenGLWindow::paintUI();

  {
    auto size{ImVec2(100, 600)};
    auto position{ImVec2((m_viewportWidth - size.x) / 2.0f,
                         (m_viewportHeight - size.y) / 2.0f)};
    ImGui::SetNextWindowPos(position);
    ImGui::SetNextWindowSize(size);
    ImGuiWindowFlags flags{ImGuiWindowFlags_NoBackground |
                           ImGuiWindowFlags_NoTitleBar |
                           ImGuiWindowFlags_NoInputs};
    ImGui::Begin(" ", nullptr, flags);
    
    if(win)
      ImGui::Text("Você ganhou!");
    
    ImGui::End();
  }
}

void OpenGLWindow::resizeGL(int width, int height) {
  m_viewportWidth = width;
  m_viewportHeight = height;

  m_camera.computeProjectionMatrix(width, height);
}

void OpenGLWindow::terminateGL() {
  for (const auto& program : m_programs) {
    glDeleteProgram(program);
  }
}

void OpenGLWindow::update() {
  float deltaTime{static_cast<float>(getDeltaTime())};

  // Update LookAt camera
  m_camera.dolly(m_dollySpeed * deltaTime);
  m_camera.truck(m_truckSpeed * deltaTime);
  m_camera.pan(m_panSpeed * deltaTime);
}
