//
// Created by acaramizaru on 6/30/23.
//

#include "ShaderProgram.hpp"

#include "Utils.hpp"

ShaderProgram::ShaderProgram(std::string vertexShaderName, std::string fragmentShaderName)
{
  QOpenGLFunctions::initializeOpenGLFunctions();

  m_vertexShaderText = Utils::LoadFile(vertexShaderName);
  m_fragmentShaderText = Utils::LoadFile(fragmentShaderName);

  m_vertexShaderId = glCreateShader(GL_VERTEX_SHADER);
  Utils::TraceGLError("glCreateShader");
  char *vertexShaderText = new char[m_vertexShaderText.length() + 1];
  strcpy(vertexShaderText, m_vertexShaderText.c_str());
  glShaderSource(m_vertexShaderId, 1, (const char**)&vertexShaderText, NULL);
  Utils::TraceGLError("glShaderSource");
  glCompileShader(m_vertexShaderId);
  Utils::TraceGLError("glCompileShader");
  delete[] vertexShaderText;

  m_fragmentShaderId = glCreateShader(GL_FRAGMENT_SHADER);
  Utils::TraceGLError("glCreateShader");
  char *fragmentShaderText = new char[m_fragmentShaderText.length()+1];
  strcpy(fragmentShaderText, m_fragmentShaderText.c_str());
  glShaderSource(m_fragmentShaderId, 1, (const char **)&fragmentShaderText, NULL);
  Utils::TraceGLError("Utils::TraceGLError");
  glCompileShader(m_fragmentShaderId);
  Utils::TraceGLError("glCompileShader");
  delete[] fragmentShaderText;


  m_programId = glCreateProgram();
  Utils::TraceGLError("glCreateProgram");
  glAttachShader(m_programId, m_vertexShaderId);
  Utils::TraceGLError("glAttachShader");
  glAttachShader(m_programId, m_fragmentShaderId);
  Utils::TraceGLError("glAttachShader");

  glLinkProgram(m_programId);
  Utils::TraceGLError("glLinkProgram");

  m_rotationLocation = glGetUniformLocation(m_programId, "rotation");
  Utils::TraceGLError("glGetUniformLocation");
  m_positionLocation = glGetUniformLocation(m_programId, "position");
  Utils::TraceGLError("glGetUniformLocation");
  m_scaleLocation = glGetUniformLocation(m_programId, "scale");
  Utils::TraceGLError("glGetUniformLocation");
  m_projectionLocation = glGetUniformLocation(m_programId, "projection");
  Utils::TraceGLError("glGetUniformLocation");
  m_viewPositionLocation = glGetUniformLocation(m_programId, "viewPosition");
  Utils::TraceGLError("glGetUniformLocation");
  m_viewRotationLocation = glGetUniformLocation(m_programId, "viewRotation");
  Utils::TraceGLError("glGetUniformLocation");
}

ShaderProgram::~ShaderProgram()
{
  glUseProgram(0);
  Utils::TraceGLError("glUseProgram");

  glDetachShader(m_programId, m_vertexShaderId);
  Utils::TraceGLError("glDetachShader");
  glDetachShader(m_programId, m_fragmentShaderId);
  Utils::TraceGLError("glDetachShader");

  glDeleteShader(m_vertexShaderId);
  Utils::TraceGLError("glDeleteShader");
  glDeleteShader(m_fragmentShaderId);
  Utils::TraceGLError("glDeleteShader");

  glDeleteProgram(m_programId);
  Utils::TraceGLError("glDeleteProgram");

}

void ShaderProgram::SetRotation(GLfloat rotationX, GLfloat rotationY, GLfloat rotationZ)
{
  Use();
  //glProgramUniform3f(m_programId, m_rotationLocation, rotationX, rotationY, rotationZ);
  glUniform3f(m_rotationLocation, rotationX, rotationY, rotationZ);
  Utils::TraceGLError("glUniform3f");
}

void ShaderProgram::SetPosition(GLfloat x, GLfloat y, GLfloat z)
{
  Use();
  //glProgramUniform3f(m_programId, m_positionLocation, x, y, z);
  glUniform3f(m_positionLocation, x,y,z);
  Utils::TraceGLError("glUniform3f");
}

void ShaderProgram::SetScale(GLfloat x, GLfloat y, GLfloat z)
{
  Use();
  //glProgramUniform3f(m_programId, m_scaleLocation, x, y, z);
  glUniform3f(m_scaleLocation, x, y, z);
  Utils::TraceGLError("glUniform3f");
}

void ShaderProgram::SetProjection(GLfloat* matrix4)
{
  Use();
  //glProgramUniformMatrix4fv(m_programId, m_projectionLocation, 1, GL_FALSE, matrix4);
  glUniformMatrix4fv(m_projectionLocation,1,GL_FALSE, matrix4);
  Utils::TraceGLError("glUniformMatrix4fv");
}

void ShaderProgram::SetViewPosition(GLfloat viewPosX, GLfloat viewPosY, GLfloat viewPosZ)
{
  Use();
  //glProgramUniform3f(m_programId, m_viewPositionLocation, viewPosX, viewPosY, viewPosZ);
  glUniform3f(m_viewPositionLocation, viewPosX, viewPosY, viewPosZ);
  Utils::TraceGLError("glUniform3f");
}
void ShaderProgram::SetViewRotation(GLfloat viewRotationX, GLfloat viewRotationY, GLfloat viewRotationZ)
{
  Use();
  //glProgramUniform3f(m_programId, m_viewRotationLocation, viewRotationX, viewRotationY, viewRotationZ);
  glUniform3f(m_viewRotationLocation, viewRotationX, viewRotationY, viewRotationZ);
  Utils::TraceGLError("glUniform3f");
}

void ShaderProgram::Use(void)
{
  glUseProgram(m_programId);
  Utils::TraceGLError("glUseProgram");
}