//
// Created by acaramizaru on 6/30/23.
//

#include "ShaderProgram.hpp"

#include "Utils.hpp"

rendering::ShaderProgram::ShaderProgram(const char *vertexProgram,
                                        const char *fragmentProgram)
    :m_vertexShaderText(vertexProgram), m_fragmentShaderText(fragmentProgram) {

  QOpenGLFunctions::initializeOpenGLFunctions();
  Init();
}

rendering::ShaderProgram::ShaderProgram(std::string &vertexShaderFileName,
                                        std::string &fragmentShaderFileName)
    : m_vertexShaderText(vertexShaderFileName),
      m_fragmentShaderText(fragmentShaderFileName) {

  QOpenGLFunctions::initializeOpenGLFunctions();
  Init();
}

void rendering::ShaderProgram::Init() {
  m_vertexShaderId = glCreateShader(GL_VERTEX_SHADER);
  CheckOpenGLError("glCreateShader");
  char *vertexShaderText = m_vertexShaderText.data();
  glShaderSource(m_vertexShaderId, 1, (const char **)&vertexShaderText,
                 nullptr);
  CheckOpenGLError("glShaderSource");
  glCompileShader(m_vertexShaderId);
  CheckOpenGLError("glCompileShader");

#ifdef QT_DEBUG
  GLint success_VS;
  glGetShaderiv(m_vertexShaderId, GL_COMPILE_STATUS, &success_VS);
  if (!success_VS) {
    GLchar InfoLog[1024];
    glGetShaderInfoLog(m_vertexShaderId, sizeof(InfoLog), nullptr, InfoLog);
    SPDLOG_ERROR("Error compiling shader type GL_VERTEX_SHADER: " +
                 std::string(InfoLog));
  }
#endif

  m_fragmentShaderId = glCreateShader(GL_FRAGMENT_SHADER);
  CheckOpenGLError("glCreateShader");
  char *fragmentShaderText = m_fragmentShaderText.data();
  glShaderSource(m_fragmentShaderId, 1, (const char **)&fragmentShaderText,
                 nullptr);
  CheckOpenGLError("Utils::TraceGLError");
  glCompileShader(m_fragmentShaderId);
  CheckOpenGLError("glCompileShader");

#ifdef QT_DEBUG
  GLint success_FS;
  glGetShaderiv(m_fragmentShaderId, GL_COMPILE_STATUS, &success_FS);
  if (!success_FS) {
    GLchar InfoLog[1024];
    glGetShaderInfoLog(m_fragmentShaderId, sizeof(InfoLog), nullptr, InfoLog);
    SPDLOG_ERROR("Error compiling shader type GL_FRAGMENT_SHADER: " +
                 std::string(InfoLog));
  }
#endif

  m_programId = glCreateProgram();
  CheckOpenGLError("glCreateProgram");
  glAttachShader(m_programId, m_vertexShaderId);
  CheckOpenGLError("glAttachShader");
  glAttachShader(m_programId, m_fragmentShaderId);
  CheckOpenGLError("glAttachShader");

  glLinkProgram(m_programId);
  CheckOpenGLError("glLinkProgram");

#ifdef QT_DEBUG
  GLint success_SP;
  glGetProgramiv(m_programId, GL_LINK_STATUS, &success_SP);
  if (success_SP == 0) {
    GLchar ErrorLog[1024];
    glGetProgramInfoLog(m_programId, sizeof(ErrorLog), nullptr, ErrorLog);
    SPDLOG_ERROR("Error linking shader program: " + std::string(ErrorLog));
  }

  GLint succes_Program;
  glValidateProgram(m_programId);
  glGetProgramiv(m_programId, GL_VALIDATE_STATUS, &succes_Program);

  if (succes_Program == 1)
    SPDLOG_INFO("The status of the shader program: OK!");
  else
    SPDLOG_ERROR("The status of the shader program: FAIL!");
#endif

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

void rendering::ShaderProgram::Use() {
  glUseProgram(m_programId);
  CheckOpenGLError("glUseProgram");
}
