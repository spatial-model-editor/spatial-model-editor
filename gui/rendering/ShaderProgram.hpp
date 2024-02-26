//
// Created by acaramizaru on 6/30/23.
//

#pragma once

#include <QtOpenGL>
#include <string>

#define MAX_NUMBER_PLANES 8

namespace rendering {

class ShaderProgram : protected QOpenGLFunctions {

protected:
  void Init();

public:
  explicit ShaderProgram(const std::string &vertexShaderFileName,
                         const std::string &fragmentShaderFileName);
  ShaderProgram(const ShaderProgram &) = delete;
  ShaderProgram(const char *vertexProgram, const char *fragmentProgram);
  ~ShaderProgram();

  ShaderProgram &operator=(ShaderProgram other) = delete;

  void Use();

  void SetRotation(GLfloat rotationX, GLfloat rotationY, GLfloat rotationZ);
  void SetPosition(GLfloat x, GLfloat y, GLfloat z);
  void SetScale(GLfloat x, GLfloat y, GLfloat z);
  void SetProjection(const GLfloat *matrix4);
  void SetViewPosition(GLfloat viewPosX, GLfloat viewPosY, GLfloat viewPosZ);
  void SetViewRotation(GLfloat viewRotationX, GLfloat viewRotationY,
                       GLfloat viewRotationZ);
  void SetColor(GLfloat r, GLfloat g, GLfloat b, GLfloat a);

  void SetMeshTranslationOffset(GLfloat x, GLfloat y, GLfloat z);

  void SetClipPlane(GLfloat a, GLfloat b, GLfloat c, GLfloat d,
                    uint32_t planeIndex);
  void DisablePlaneIndex(uint32_t planeIndex);
  void DisableAllPlanes();

private:
  std::string m_vertexShaderText;
  std::string m_fragmentShaderText;

  GLuint m_vertexShaderId;
  GLuint m_fragmentShaderId;
  GLuint m_programId;

  // uniforms
  GLint m_rotationLocation;
  GLint m_positionLocation;
  GLint m_scaleLocation;
  GLint m_projectionLocation;
  GLint m_viewPositionLocation;
  GLint m_viewRotationLocation;
  GLint m_color;

  GLint m_meshTranslationOffsetLocation;

  GLint m_clipPlane[MAX_NUMBER_PLANES];
  GLint m_activeClipPlane[MAX_NUMBER_PLANES];
};

} // namespace rendering
