//
// Created by acaramizaru on 6/30/23.
//

#include "ShaderProgram.hpp"

#include "Utils.hpp"

rendering::ShaderProgram::ShaderProgram(const char *vertexProgram,
                                        const char *fragmentProgram) {
  initializeOpenGLFunctions();

  m_vertexShaderText = std::string(vertexProgram);
  m_fragmentShaderText = std::string(fragmentProgram);

  Init();
}

rendering::ShaderProgram::ShaderProgram(std::string vertexShaderName,
                                        std::string fragmentShaderName) {
  QOpenGLFunctions::initializeOpenGLFunctions();

  m_vertexShaderText = Utils::LoadFile(vertexShaderName);
  m_fragmentShaderText = Utils::LoadFile(fragmentShaderName);

  Init();
}

void rendering::ShaderProgram::Init() {
  m_vertexShaderId = glCreateShader(GL_VERTEX_SHADER);
  CheckOpenGLError("glCreateShader");
  char *vertexShaderText = new char[m_vertexShaderText.length() + 1];
  strncpy(vertexShaderText, m_vertexShaderText.c_str(),
          m_vertexShaderText.length() + 1);
  glShaderSource(m_vertexShaderId, 1, (const char **)&vertexShaderText,
                 nullptr);
  CheckOpenGLError("glShaderSource");
  glCompileShader(m_vertexShaderId);
  CheckOpenGLError("glCompileShader");
  delete[] vertexShaderText;

  m_fragmentShaderId = glCreateShader(GL_FRAGMENT_SHADER);
  CheckOpenGLError("glCreateShader");
  char *fragmentShaderText = new char[m_fragmentShaderText.length() + 1];
  strncpy(fragmentShaderText, m_fragmentShaderText.c_str(),
          m_fragmentShaderText.length() + 1);
  glShaderSource(m_fragmentShaderId, 1, (const char **)&fragmentShaderText,
                 nullptr);
  CheckOpenGLError("Utils::TraceGLError");
  glCompileShader(m_fragmentShaderId);
  CheckOpenGLError("glCompileShader");
  delete[] fragmentShaderText;

  m_programId = glCreateProgram();
  SPDLOG_ERROR("{}", m_programId);
  CheckOpenGLError("glCreateProgram");
  glAttachShader(m_programId, m_vertexShaderId);
  CheckOpenGLError("glAttachShader");
  glAttachShader(m_programId, m_fragmentShaderId);
  CheckOpenGLError("glAttachShader");

  glLinkProgram(m_programId);
  CheckOpenGLError("glLinkProgram");

  m_rotationLocation = glGetUniformLocation(m_programId, "rotation");
  CheckOpenGLError("glGetUniformLocation");
  m_positionLocation = glGetUniformLocation(m_programId, "position");
  CheckOpenGLError("glGetUniformLocation");
  m_scaleLocation = glGetUniformLocation(m_programId, "scale");
  CheckOpenGLError("glGetUniformLocation");
  m_projectionLocation = glGetUniformLocation(m_programId, "projection");
  CheckOpenGLError("glGetUniformLocation");
  m_viewPositionLocation = glGetUniformLocation(m_programId, "viewPosition");
  CheckOpenGLError("glGetUniformLocation");
  m_viewRotationLocation = glGetUniformLocation(m_programId, "viewRotation");
  CheckOpenGLError("glGetUniformLocation");
}

rendering::ShaderProgram::~ShaderProgram() {
  SPDLOG_ERROR("");
  glUseProgram(0);
  CheckOpenGLError("glUseProgram");

  glDetachShader(m_programId, m_vertexShaderId);
  CheckOpenGLError("glDetachShader");
  glDetachShader(m_programId, m_fragmentShaderId);
  CheckOpenGLError("glDetachShader");

  glDeleteShader(m_vertexShaderId);
  CheckOpenGLError("glDeleteShader");
  glDeleteShader(m_fragmentShaderId);
  CheckOpenGLError("glDeleteShader");

  glDeleteProgram(m_programId);
  CheckOpenGLError("glDeleteProgram");
}

void rendering::ShaderProgram::SetRotation(GLfloat rotationX, GLfloat rotationY,
                                           GLfloat rotationZ) {
  Use();
  glUniform3f(m_rotationLocation, rotationX, rotationY, rotationZ);
  CheckOpenGLError("glUniform3f");
}

void rendering::ShaderProgram::SetPosition(GLfloat x, GLfloat y, GLfloat z) {
  Use();
  glUniform3f(m_positionLocation, x, y, z);
  CheckOpenGLError("glUniform3f");
}

void rendering::ShaderProgram::SetScale(GLfloat x, GLfloat y, GLfloat z) {
  Use();
  glUniform3f(m_scaleLocation, x, y, z);
  CheckOpenGLError("glUniform3f");
}

void rendering::ShaderProgram::SetProjection(GLfloat *matrix4) {
  Use();
  glUniformMatrix4fv(m_projectionLocation, 1, GL_FALSE, matrix4);
  CheckOpenGLError("glUniformMatrix4fv");
}

void rendering::ShaderProgram::SetViewPosition(GLfloat viewPosX,
                                               GLfloat viewPosY,
                                               GLfloat viewPosZ) {
  Use();
  glUniform3f(m_viewPositionLocation, viewPosX, viewPosY, viewPosZ);
  CheckOpenGLError("glUniform3f");
}
void rendering::ShaderProgram::SetViewRotation(GLfloat viewRotationX,
                                               GLfloat viewRotationY,
                                               GLfloat viewRotationZ) {
  Use();
  glUniform3f(m_viewRotationLocation, viewRotationX, viewRotationY,
              viewRotationZ);
  CheckOpenGLError("glUniform3f");
}

void rendering::ShaderProgram::Use(void) {
  SPDLOG_TRACE("");
  glUseProgram(m_programId);
  CheckOpenGLError("glUseProgram");
}
