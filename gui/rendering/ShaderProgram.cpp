//
// Created by acaramizaru on 6/30/23.
//

#include "ShaderProgram.hpp"

#include "Utils.hpp"

ShaderProgram::ShaderProgram(std::string vertexShaderName, std::string fragmentShaderName)
{
  m_vertexShaderText = Utils::LoadFile(vertexShaderName);
  m_fragmentShaderText = Utils::LoadFile(fragmentShaderName);

  m_vertexShaderId = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(m_vertexShaderId, 1, &m_vertexShaderText.c_str(), NULL);
  glCompileShader(m_vertexShaderId);

  m_fragmentShaderId = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(m_fragmentShaderId, 1, &m_fragmentShaderText.c_str(), NULL);
  glCompileShader(m_fragmentShaderId);


  m_programId = glCreateProgram();
  glAttachShader(m_programId, m_vertexShaderId);
  glAttachShader(m_programId, m_fragmentShaderId);

  glLinkProgram(m_programId);

  m_rotationLocation = glGetUniformLocation(m_programId, "rotation");
  m_positionLocation = glGetUniformLocation(m_programId, "position");
  m_scaleLocation = glGetUniformLocation(m_programId, "scale");
  m_projectionLocation = glGetUniformLocation(m_programId, "projection");
  m_viewPositionLocation = glGetUniformLocation(m_programId, "viewPosition");
  m_viewRotationLocation = glGetUniformLocation(m_programId, "viewRotation");
}

ShaderProgram::~ShaderProgram()
{
  glUseProgram(0);

  glDetachShader(m_programId, m_vertexShaderId);
  glDetachShader(m_programId, m_fragmentShaderId);

  glDeleteShader(m_vertexShaderId);
  glDeleteShader(m_fragmentShaderId);

  glDeleteProgram(m_programId);

  delete m_vertexShaderText;
  delete m_fragmentShaderText;
}

void ShaderProgram::SetRotation(GLfloat rotationX, GLfloat rotationY, GLfloat rotationZ)
{
  glProgramUniform3f(m_programId, m_rotationLocation, rotationX, rotationY, rotationZ);
}

void ShaderProgram::SetPosition(GLfloat x, GLfloat y, GLfloat z)
{
  glProgramUniform3f(m_programId, m_positionLocation, x, y, z);
}

void ShaderProgram::SetScale(GLfloat x, GLfloat y, GLfloat z)
{
  glProgramUniform3f(m_programId, m_scaleLocation, x, y, z);
}

void ShaderProgram::SetProjection(GLfloat* matrix4)
{
  glProgramUniformMatrix4fv(m_programId, m_projectionLocation, 1, GL_FALSE, matrix4);
}

void ShaderProgram::SetViewPosition(GLfloat viewPosX, GLfloat viewPosY, GLfloat viewPosZ)
{
  glProgramUniform3f(m_programId, m_viewPositionLocation, viewPosX, viewPosY, viewPosZ);
}
void ShaderProgram::SetViewRotation(GLfloat viewRotationX, GLfloat viewRotationY, GLfloat viewRotationZ)
{
  glProgramUniform3f(m_programId, m_viewRotationLocation, viewRotationX, viewRotationY, viewRotationZ);
}

void ShaderProgram::Use(void)
{
  glUseProgram(m_programId);
}