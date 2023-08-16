//
// Created by acaramizaru on 6/30/23.
//

#include "WireframeObject.hpp"

WireframeObject::WireframeObject(ObjectInfo info, Vector4 color)
{
  QOpenGLFunctions::initializeOpenGLFunctions();

  m_vertices = info.vertices;
  m_color = color;

  for (Face f : info.faces)
  {
    m_indices.push_back(f.vertexIndices[0] - 1);
    m_indices.push_back(f.vertexIndices[1] - 1);

    m_indices.push_back(f.vertexIndices[1] - 1);
    m_indices.push_back(f.vertexIndices[2] - 1);

    m_indices.push_back(f.vertexIndices[2] - 1);
    m_indices.push_back(f.vertexIndices[0] - 1);
  }

  auto cArr = m_color.ToArray();
  for (auto v : m_vertices)
  {
    auto vArr = v.ToArray();
    m_verticesBuffer.insert(m_verticesBuffer.end(), vArr.begin(), vArr.end());
    m_colorBuffer.insert(m_colorBuffer.end(), cArr.begin(), cArr.end());
  }

  m_position = Vector3(0.0f, 0.0f, 0.0f);
  m_rotation = Vector3(0.0f, 0.0f, 0.0f);
  m_scale = Vector3(1.0f, 1.0f, 1.0f);
  CreateVBO();
}

WireframeObject::~WireframeObject(void)
{
  DestroyVBO();
}

void WireframeObject::SetColor(Vector4 color)
{
  m_color = color;
  UpdateVBOColor();
}

void WireframeObject::UpdateVBOColor()
{
  int size = m_colorBuffer.size() / m_color.ToArray().size();

  auto cArr = m_color.ToArray();

  m_colorBuffer.clear();

  for(int iter=0; iter < size; iter++)
    m_colorBuffer.insert(m_colorBuffer.end(), cArr.begin(), cArr.end());

  GLsizeiptr colorBufferSize = m_colorBuffer.size() * sizeof(GLfloat);

  m_vao->bind();

  glBindBuffer(GL_ARRAY_BUFFER, m_colorBufferId);
  glBufferData(GL_ARRAY_BUFFER, colorBufferSize, m_colorBuffer.data(), GL_DYNAMIC_DRAW);
  glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(1);
}

void WireframeObject::CreateVBO(void)
{
  GLsizeiptr vboSize = m_verticesBuffer.size() * sizeof(GLfloat);
  GLsizeiptr colorBufferSize = m_colorBuffer.size() * sizeof(GLfloat);
  GLsizeiptr indexBufferSize = m_indices.size() * sizeof(GLuint);

//  glGenVertexArrays(1, &m_vao);
//  glBindVertexArray(m_vao);

  m_vao = new QOpenGLVertexArrayObject(nullptr);
  m_vao->create();
  m_vao->bind();


  glGenBuffers(1, &m_vbo);
  Utils::TraceGLError("glGenBuffers");
  glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
  Utils::TraceGLError("glBindBuffer");
  glBufferData(GL_ARRAY_BUFFER, vboSize, m_verticesBuffer.data(), GL_STATIC_DRAW);
  Utils::TraceGLError("glBufferData");
  glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);
  Utils::TraceGLError("glVertexAttribPointer");
  glEnableVertexAttribArray(0);
  Utils::TraceGLError("glEnableVertexAttribArray");

  glGenBuffers(1, &m_colorBufferId);
  Utils::TraceGLError("glGenBuffers");
  glBindBuffer(GL_ARRAY_BUFFER, m_colorBufferId);
  Utils::TraceGLError("glBindBuffer");
  //glBufferData(GL_ARRAY_BUFFER, colorBufferSize, m_colorBuffer.data(), GL_STATIC_DRAW);
  glBufferData(GL_ARRAY_BUFFER, colorBufferSize, m_colorBuffer.data(), GL_DYNAMIC_DRAW);
  Utils::TraceGLError("glBufferData");
  glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, 0);
  Utils::TraceGLError("glVertexAttribPointer");
  glEnableVertexAttribArray(1);
  Utils::TraceGLError("glEnableVertexAttribArray");

  glGenBuffers(1, &m_elementBufferId);
  Utils::TraceGLError("glGenBuffers");
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_elementBufferId);
  Utils::TraceGLError("glBindBuffer");
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexBufferSize, m_indices.data(), GL_STATIC_DRAW);
  Utils::TraceGLError("glBufferData");
}

void WireframeObject::DestroyVBO(void)
{
  glDisableVertexAttribArray(1);
  glDisableVertexAttribArray(0);

  glBindBuffer(GL_ARRAY_BUFFER, 0);

  glDeleteBuffers(1, &m_colorBufferId);
  glDeleteBuffers(1, &m_vbo);
  glDeleteBuffers(1, &m_elementBufferId);

//  glBindVertexArray(0);
//  glDeleteVertexArrays(1, &m_vao);

  m_vao->release();
  m_vao->destroy();
}

void WireframeObject::Render(ShaderProgram* program, float lineWidth)
{
  //scale geometry for a more reliable picking
  //https://registry.khronos.org/OpenGL-Refpages/gl4/html/glLineWidth.xhtml

  glLineWidth(lineWidth);
  glEnable(GL_LINE_SMOOTH);

  program->SetPosition(m_position.x, m_position.y, m_position.z);
  program->SetRotation(m_rotation.x, m_rotation.y, m_rotation.z);
  program->SetScale(m_scale.x, m_scale.y, m_scale.z);
  //  glBindVertexArray(m_vao);
  m_vao->bind();
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_elementBufferId);
  glDrawElements(GL_LINES, m_indices.size(), GL_UNSIGNED_INT, (void*)0);
}

void WireframeObject::SetRotation(GLfloat rotationX, GLfloat rotationY, GLfloat rotationZ)
{
  m_rotation.x = rotationX;
  m_rotation.y = rotationY;
  m_rotation.z = rotationZ;
}

void WireframeObject::SetRotation(Vector3 rotation)
{
  m_rotation = rotation;
}

void WireframeObject::SetPosition(GLfloat positionX, GLfloat positionY, GLfloat positionZ)
{
  m_position.x = positionX;
  m_position.y = positionY;
  m_position.z = positionZ;
}

void WireframeObject::SetPosition(Vector3 position)
{
  m_position = position;
}

void WireframeObject::SetScale(GLfloat scaleX, GLfloat scaleY, GLfloat scaleZ)
{
  m_scale.x = scaleX;
  m_scale.y = scaleY;
  m_scale.z = scaleZ;
}

void WireframeObject::SetScale(Vector3 scale)
{
  m_scale = scale;
}