//
// Created by acaramizaru on 6/30/23.
//

#pragma once

#include <QtOpenGL>
#include <string>

namespace rendering {

class ShaderProgram : protected QOpenGLFunctions {

protected:
  void Init();

public:
  ShaderProgram(const ShaderProgram &) = delete;
  ShaderProgram(const std::string &vertexProgram,
                const std::string &geometryProgram,
                const std::string &fragmentProgram);
  ~ShaderProgram();

  ShaderProgram &operator=(ShaderProgram other) = delete;

  GLuint createShader(GLenum type, const std::string &src);

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
  void SetThickness(GLfloat thickness);
  void SetBackgroundColor(GLfloat r, GLfloat g, GLfloat b);

private:
  std::string m_vertexShaderText;
  std::string m_geometryShaderText;
  std::string m_fragmentShaderText;

  GLuint m_vertexShaderId;
  GLuint m_geometryShaderId;
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
  GLint m_thickness;
  GLint m_background_color;
};

} // namespace rendering
