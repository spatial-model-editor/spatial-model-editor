//
// Created by acaramizaru on 6/30/23.
//

#include "ShaderProgram.hpp"

#include "Utils.hpp"

rendering::ShaderProgram::ShaderProgram(const std::string &vertexProgram,
                                        const std::string &geometryProgram,
                                        const std::string &fragmentProgram)
    : m_vertexShaderText(vertexProgram), m_geometryShaderText(geometryProgram),
      m_fragmentShaderText(fragmentProgram) {

  QOpenGLFunctions::initializeOpenGLFunctions();
  Init();
}

GLuint rendering::ShaderProgram::createShader(GLenum type,
                                              const std::string &src) {

  GLuint shaderID = glCreateShader(type);
  CheckOpenGLError("glCreateShader");
  const char *vertexShaderText = src.data();
  glShaderSource(shaderID, 1, &vertexShaderText, nullptr);
  CheckOpenGLError("glShaderSource");
  glCompileShader(shaderID);
  CheckOpenGLError("glCompileShader");

#ifdef QT_DEBUG
  GLint success;
  glGetShaderiv(shaderID, GL_COMPILE_STATUS, &success);
  if (!success) {
    std::string InfoLog(1024, ' ');
    glGetShaderInfoLog(shaderID, static_cast<int>(InfoLog.size()), nullptr,
                       InfoLog.data());
    SPDLOG_ERROR("Error compiling shader type " + std::to_string(type) + ":" +
                 InfoLog);
  }
#endif

  return shaderID;
}

void rendering::ShaderProgram::Init() {

  m_vertexShaderId = createShader(GL_VERTEX_SHADER, m_vertexShaderText);
  m_geometryShaderId = createShader(GL_GEOMETRY_SHADER, m_geometryShaderText);
  m_fragmentShaderId = createShader(GL_FRAGMENT_SHADER, m_fragmentShaderText);

  m_programId = glCreateProgram();
  CheckOpenGLError("glCreateProgram");
  glAttachShader(m_programId, m_vertexShaderId);
  CheckOpenGLError("glAttachShader");
  glAttachShader(m_programId, m_fragmentShaderId);
  CheckOpenGLError("glAttachShader");
  glAttachShader(m_programId, m_geometryShaderId);

  glLinkProgram(m_programId);
  CheckOpenGLError("glLinkProgram");

#ifdef QT_DEBUG
  GLint success_SP;
  glGetProgramiv(m_programId, GL_LINK_STATUS, &success_SP);
  if (success_SP == 0) {
    std::string ErrorLog(1024, ' ');
    glGetProgramInfoLog(m_programId, static_cast<int>(ErrorLog.size()), nullptr,
                        ErrorLog.data());
    SPDLOG_ERROR("Error linking shader program: " + ErrorLog);
  }

  GLint succes_Program;
  glValidateProgram(m_programId);
  glGetProgramiv(m_programId, GL_VALIDATE_STATUS, &succes_Program);

  if (succes_Program == 1)
    SPDLOG_INFO("The status of the shader program: OK!");
  else
    SPDLOG_ERROR("The status of the shader program: FAIL!");
#endif

  //  m_rotationLocation = glGetUniformLocation(m_programId, "rotation");
  //  CheckOpenGLError("glGetUniformLocation");
  //  m_positionLocation = glGetUniformLocation(m_programId, "position");
  //  CheckOpenGLError("glGetUniformLocation");
  //  m_scaleLocation = glGetUniformLocation(m_programId, "scale");
  //  CheckOpenGLError("glGetUniformLocation");
  m_modelLocation = glGetUniformLocation(m_programId, "model");
  m_projectionLocation = glGetUniformLocation(m_programId, "projection");
  CheckOpenGLError("glGetUniformLocation");
  //  m_viewPositionLocation = glGetUniformLocation(m_programId,
  //  "viewPosition"); CheckOpenGLError("glGetUniformLocation");
  //  m_viewRotationLocation = glGetUniformLocation(m_programId,
  //  "viewRotation"); CheckOpenGLError("glGetUniformLocation");
  m_viewLocation = glGetUniformLocation(m_programId, "view");
  CheckOpenGLError("glGetUniformLocation");
  m_color = glGetUniformLocation(m_programId, "in_Color");
  CheckOpenGLError("glGetUniformLocation");
  //  m_meshTranslationOffsetLocation =
  //      glGetUniformLocation(m_programId, "translationOffset");
  m_modelOffsetLocation = glGetUniformLocation(m_programId, "modelOffset");
  CheckOpenGLError("glGetUniformLocation");
  m_thickness = glGetUniformLocation(m_programId, "thickness");
  CheckOpenGLError("glGetUniformLocation");
  m_background_color = glGetUniformLocation(m_programId, "background_color");
  CheckOpenGLError("glGetUniformLocation");
  m_color = glGetUniformLocation(m_programId, "in_Color");
  CheckOpenGLError("glGetUniformLocation");

  for (uint32_t i = 0; i < MAX_NUMBER_PLANES; i++) {
    std::string planeName = "clipPlane[" + std::to_string(i) + "]";
    m_clipPlane[i] = glGetUniformLocation(m_programId, planeName.c_str());
    CheckOpenGLError("glGetUniformLocation");
  }

  for (uint32_t i = 0; i < MAX_NUMBER_PLANES; i++) {
    std::string activeClipPlane = "activeClipPlane[" + std::to_string(i) + "]";
    m_activeClipPlane[i] =
        glGetUniformLocation(m_programId, activeClipPlane.c_str());
    CheckOpenGLError("glGetUniformLocation");
  }
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

// void rendering::ShaderProgram::SetRotation(GLfloat rotationX, GLfloat
// rotationY,
//                                            GLfloat rotationZ) {
//   Use();
//   glUniform3f(m_rotationLocation, rotationX, rotationY, rotationZ);
//   CheckOpenGLError("glUniform3f");
// }
//
// void rendering::ShaderProgram::SetPosition(GLfloat x, GLfloat y, GLfloat z) {
//   Use();
//   glUniform3f(m_positionLocation, x, y, z);
//   CheckOpenGLError("glUniform3f");
// }
//
// void rendering::ShaderProgram::SetScale(GLfloat x, GLfloat y, GLfloat z) {
//   Use();
//   glUniform3f(m_scaleLocation, x, y, z);
//   CheckOpenGLError("glUniform3f");
// }

void rendering::ShaderProgram::SetModel(const GLfloat *matrix4x4) {
  Use();
  glUniformMatrix4fv(m_modelLocation, 1, GL_FALSE, matrix4x4);
  CheckOpenGLError("glUniformMatrix4fv");
}

void rendering::ShaderProgram::SetProjection(const GLfloat *matrix4x4) {
  Use();
  glUniformMatrix4fv(m_projectionLocation, 1, GL_FALSE, matrix4x4);
  CheckOpenGLError("glUniformMatrix4fv");
}

// void rendering::ShaderProgram::SetViewPosition(GLfloat viewPosX,
//                                                GLfloat viewPosY,
//                                                GLfloat viewPosZ) {
//   Use();
//   glUniform3f(m_viewPositionLocation, viewPosX, viewPosY, viewPosZ);
//   CheckOpenGLError("glUniform3f");
// }
// void rendering::ShaderProgram::SetViewRotation(GLfloat viewRotationX,
//                                                GLfloat viewRotationY,
//                                                GLfloat viewRotationZ) {
//   Use();
//   glUniform3f(m_viewRotationLocation, viewRotationX, viewRotationY,
//               viewRotationZ);
//   CheckOpenGLError("glUniform3f");
// }
void rendering::ShaderProgram::SetView(const GLfloat *matrix4x4) {
  Use();
  glUniformMatrix4fv(m_viewLocation, 1, GL_FALSE, matrix4x4);
  CheckOpenGLError("glUniformMatrix4fv");
}
void rendering::ShaderProgram::SetColor(GLfloat r, GLfloat g, GLfloat b,
                                        GLfloat a) {
  Use();
  glUniform4f(m_color, r, g, b, a);
  CheckOpenGLError("glUniform4f");
}

// void rendering::ShaderProgram::SetMeshTranslationOffset(GLfloat x, GLfloat y,
//                                                         GLfloat z) {
//
//   Use();
//   glUniform3f(m_meshTranslationOffsetLocation, x, y, z);
//   CheckOpenGLError("glUniform3f");
// }

void rendering::ShaderProgram::SetModelOffset(const GLfloat *matrix4x4) {
  Use();
  glUniformMatrix4fv(m_modelOffsetLocation, 1, GL_FALSE, matrix4x4);
  CheckOpenGLError("glUniformMatrix4fv");
}

void rendering::ShaderProgram::SetThickness(GLfloat thickness) {
  Use();
  glUniform1f(m_thickness, thickness);
  CheckOpenGLError("glUniform1f");
}

void rendering::ShaderProgram::SetBackgroundColor(GLfloat r, GLfloat g,
                                                  GLfloat b) {
  Use();
  glUniform3f(m_background_color, r, g, b);
  CheckOpenGLError("glUniform3f");
}

void rendering::ShaderProgram::Use() {
  glUseProgram(m_programId);
  CheckOpenGLError("glUseProgram");
}

void rendering::ShaderProgram::SetClippingPlane(GLfloat a, GLfloat b, GLfloat c,
                                                GLfloat d,
                                                uint32_t planeIndex) {

  assert(planeIndex < MAX_NUMBER_PLANES);

  // set the clipping plane
  glUniform4f(m_clipPlane[planeIndex], a, b, c, d);
  CheckOpenGLError("glUniform4f");
}

void rendering::ShaderProgram::EnableClippingPlane(uint32_t planeIndex) {

  assert(planeIndex < MAX_NUMBER_PLANES);

  Use();

  // activate OpenGL's clip-plane internal state
  glEnable(GL_CLIP_DISTANCE0 + planeIndex);
  CheckOpenGLError("glEnable");

  // activate plane test inside vertex/geometry shader
  glUniform1i(m_activeClipPlane[planeIndex], true);
  CheckOpenGLError("glUniform1i");
}

void rendering::ShaderProgram::DisableClippingPlane(uint32_t planeIndex) {

  assert(planeIndex < MAX_NUMBER_PLANES);

  Use();

  // disable OpenGL's clip-plane internal state
  glDisable(GL_CLIP_DISTANCE0 + planeIndex);
  CheckOpenGLError("glDisable");

  // deactivate plane test inside vertex/geometry shader
  glUniform1i(m_activeClipPlane[planeIndex], false);
  CheckOpenGLError("glUniform1i");
}

void rendering::ShaderProgram::DisableAllClippingPlanes() {

  for (uint32_t i = 0; i < MAX_NUMBER_PLANES; i++) {
    DisableClippingPlane(i);
  }
}
