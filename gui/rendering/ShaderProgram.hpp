//
// Created by acaramizaru on 6/30/23.
//

#pragma once

#include <QtOpenGL>
#include <string>

constexpr uint32_t MAX_NUMBER_PLANES = 8;

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

  ShaderProgram &operator=(const ShaderProgram &other) = delete;

  GLuint createShader(GLenum type, const std::string &src);

  void Use();

  void SetModel(const GLfloat *matrix4x4);

  void SetProjection(const GLfloat *matrix4x4);
  void SetView(const GLfloat *matrix4x4);

  void SetColor(GLfloat r, GLfloat g, GLfloat b, GLfloat a);

  void SetModelOffset(const GLfloat *matrix4x4);
  void SetThickness(GLfloat thickness);
  void SetBackgroundColor(GLfloat r, GLfloat g, GLfloat b);

  void SetClippingPlane(GLfloat a, GLfloat b, GLfloat c, GLfloat d,
                        uint32_t planeIndex);
  void EnableClippingPlane(uint32_t planeIndex);
  void DisableClippingPlane(uint32_t planeIndex);
  void DisableAllClippingPlanes();

private:
  std::string m_vertexShaderText;
  std::string m_geometryShaderText;
  std::string m_fragmentShaderText;

  GLuint m_vertexShaderId;
  GLuint m_geometryShaderId;
  GLuint m_fragmentShaderId;
  GLuint m_programId;

  // uniforms
  GLint m_modelLocation;

  GLint m_projectionLocation;
  GLint m_viewLocation;

  GLint m_color;

  GLint m_modelOffsetLocation;
  GLint m_thickness;
  GLint m_background_color;

  std::array<GLint, MAX_NUMBER_PLANES> m_clipPlane;
  std::array<GLint, MAX_NUMBER_PLANES> m_activeClipPlane;
};

} // namespace rendering
