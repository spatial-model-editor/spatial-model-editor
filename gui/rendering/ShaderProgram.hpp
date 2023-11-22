//
// Created by acaramizaru on 6/30/23.
//

#ifndef SPATIALMODELEDITOR_SHADERPROGRAM_H
#define SPATIALMODELEDITOR_SHADERPROGRAM_H

#include <QtOpenGL>
#include <string>

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

private:
  std::string m_vertexShaderText;
  std::string m_fragmentShaderText;

  GLuint m_vertexShaderId;
  GLuint m_fragmentShaderId;
  GLuint m_programId;

  GLint m_rotationLocation;
  GLint m_positionLocation;
  GLint m_scaleLocation;
  GLint m_projectionLocation;
  GLint m_viewPositionLocation;
  GLint m_viewRotationLocation;
};

} // namespace rendering

#endif // SPATIALMODELEDITOR_SHADERPROGRAM_H
