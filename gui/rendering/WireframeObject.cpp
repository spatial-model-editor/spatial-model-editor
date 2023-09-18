//
// Created by acaramizaru on 6/30/23.
//

#include "WireframeObject.hpp"

rendering::WireframeObject::WireframeObject(rendering::ObjectInfo info,
                                            QColor color,
                                            rendering::SMesh &mesh,
                                            QOpenGLWidget* Widget,
                                            Vector3 position, Vector3 rotation,
                                            Vector3 scale)
    : m_mesh(mesh),m_openGLContext(Widget->context()), m_position(position), m_rotation(rotation), m_scale(scale),
      m_color(color) {

  m_openGLContext->makeCurrent(m_openGLContext->surface());
  QOpenGLFunctions::initializeOpenGLFunctions();
  //m_openGLContext->makeCurrent(nullptr);

  m_vertices = info.vertices;

  for (rendering::Face f : info.faces) {
    m_indices.push_back(f.vertexIndices[0] - 1);
    m_indices.push_back(f.vertexIndices[1] - 1);

    m_indices.push_back(f.vertexIndices[1] - 1);
    m_indices.push_back(f.vertexIndices[2] - 1);

    m_indices.push_back(f.vertexIndices[2] - 1);
    m_indices.push_back(f.vertexIndices[0] - 1);
  }

  // auto cArr = m_color.ToArray();
  std::vector<uint8_t> cArr = {(uint8_t)m_color.red(), (uint8_t)m_color.green(),
                               (uint8_t)m_color.blue(),
                               (uint8_t)m_color.alpha()};

  for (auto v : m_vertices) {
    auto vArr = v.ToArray();
    m_verticesBuffer.insert(m_verticesBuffer.end(), vArr.begin(), vArr.end());
    m_colorBuffer.insert(m_colorBuffer.end(), cArr.begin(), cArr.end());
  }

  CreateVBO();
}

rendering::WireframeObject::~WireframeObject(void) { DestroyVBO(); }

void rendering::WireframeObject::SetColor(QColor color) {
  m_color = color;
  UpdateVBOColor();
}

rendering::SMesh rendering::WireframeObject::GetMesh() { return m_mesh; }

void rendering::WireframeObject::UpdateVBOColor() {

  m_openGLContext->makeCurrent(m_openGLContext->surface());

  int size = m_colorBuffer.size() / 4;

  // auto cArr = m_color.ToArray();
  std::vector<uint8_t> cArr = {(uint8_t)m_color.red(), (uint8_t)m_color.green(),
                               (uint8_t)m_color.blue(),
                               (uint8_t)m_color.alpha()};

  m_colorBuffer.clear();

  for (int iter = 0; iter < size; iter++)
    m_colorBuffer.insert(m_colorBuffer.end(), cArr.begin(), cArr.end());

  // GLsizeiptr colorBufferSize = m_colorBuffer.size() * sizeof(GLfloat);
  GLsizeiptr colorBufferSize = m_colorBuffer.size() * sizeof(uint8_t);

  m_vao->bind();

  glBindBuffer(GL_ARRAY_BUFFER, m_colorBufferId);
  glBufferData(GL_ARRAY_BUFFER, colorBufferSize, m_colorBuffer.data(),
               GL_DYNAMIC_DRAW);
  // glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, nullptr);
  glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_FALSE, 0, nullptr);
  glEnableVertexAttribArray(1);

  //m_openGLContext->makeCurrent(nullptr);
}

void rendering::WireframeObject::CreateVBO(void) {

  m_openGLContext->makeCurrent(m_openGLContext->surface());

  GLsizeiptr vboSize = m_verticesBuffer.size() * sizeof(GLfloat);
  // GLsizeiptr colorBufferSize = m_colorBuffer.size() * sizeof(GLfloat);
  GLsizeiptr colorBufferSize = m_colorBuffer.size() * sizeof(uint8_t);
  GLsizeiptr indexBufferSize = m_indices.size() * sizeof(GLuint);

  m_vao = std::unique_ptr<QOpenGLVertexArrayObject>(
      new QOpenGLVertexArrayObject(nullptr));
  m_vao->create();
  m_vao->bind();

  glGenBuffers(1, &m_vbo);
  CheckOpenGLError("glGenBuffers");
  glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
  CheckOpenGLError("glBindBuffer");
  glBufferData(GL_ARRAY_BUFFER, vboSize, m_verticesBuffer.data(),
               GL_STATIC_DRAW);
  CheckOpenGLError("glBufferData");
  glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, nullptr);
  CheckOpenGLError("glVertexAttribPointer");
  glEnableVertexAttribArray(0);
  CheckOpenGLError("glEnableVertexAttribArray");

  glGenBuffers(1, &m_colorBufferId);
  CheckOpenGLError("glGenBuffers");
  glBindBuffer(GL_ARRAY_BUFFER, m_colorBufferId);
  CheckOpenGLError("glBindBuffer");
  // glBufferData(GL_ARRAY_BUFFER, colorBufferSize, m_colorBuffer.data(),
  // GL_STATIC_DRAW);
  glBufferData(GL_ARRAY_BUFFER, colorBufferSize, m_colorBuffer.data(),
               GL_DYNAMIC_DRAW);
  CheckOpenGLError("glBufferData");
  glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_FALSE, 0, nullptr);
  CheckOpenGLError("glVertexAttribPointer");
  glEnableVertexAttribArray(1);
  CheckOpenGLError("glEnableVertexAttribArray");

  glGenBuffers(1, &m_elementBufferId);
  CheckOpenGLError("glGenBuffers");
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_elementBufferId);
  CheckOpenGLError("glBindBuffer");
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexBufferSize, m_indices.data(),
               GL_STATIC_DRAW);
  CheckOpenGLError("glBufferData");

  //m_openGLContext->makeCurrent(nullptr);
}

void rendering::WireframeObject::DestroyVBO(void) {

  m_openGLContext->makeCurrent(m_openGLContext->surface());

  glDisableVertexAttribArray(1);
  glDisableVertexAttribArray(0);

  glBindBuffer(GL_ARRAY_BUFFER, 0);

  glDeleteBuffers(1, &m_colorBufferId);
  glDeleteBuffers(1, &m_vbo);
  glDeleteBuffers(1, &m_elementBufferId);

  m_vao->release();
  m_vao->destroy();

  // delete m_vao;

  //m_openGLContext->makeCurrent(nullptr);
}

void rendering::WireframeObject::Render(
    std::unique_ptr<rendering::ShaderProgram> &program, float lineWidth) {

  glLineWidth(lineWidth);
  glEnable(GL_LINE_SMOOTH);

  program->SetPosition(m_position.x, m_position.y, m_position.z);
  program->SetRotation(m_rotation.x, m_rotation.y, m_rotation.z);
  program->SetScale(m_scale.x, m_scale.y, m_scale.z);
  //  glBindVertexArray(m_vao);
  m_vao->bind();
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_elementBufferId);
  glDrawElements(GL_LINES, m_indices.size(), GL_UNSIGNED_INT, (void *)nullptr);
}

void rendering::WireframeObject::SetRotation(GLfloat rotationX,
                                             GLfloat rotationY,
                                             GLfloat rotationZ) {
  m_rotation.x = rotationX;
  m_rotation.y = rotationY;
  m_rotation.z = rotationZ;
}

void rendering::WireframeObject::SetRotation(rendering::Vector3 rotation) {
  m_rotation = rotation;
}

rendering::Vector3 rendering::WireframeObject::GetRotation() {
  return m_rotation;
}

void rendering::WireframeObject::SetPosition(GLfloat positionX,
                                             GLfloat positionY,
                                             GLfloat positionZ) {
  m_position.x = positionX;
  m_position.y = positionY;
  m_position.z = positionZ;
}

void rendering::WireframeObject::SetPosition(rendering::Vector3 position) {
  m_position = position;
}

rendering::Vector3 rendering::WireframeObject::GetPosition() {
  return m_position;
}

void rendering::WireframeObject::SetScale(GLfloat scaleX, GLfloat scaleY,
                                          GLfloat scaleZ) {
  m_scale.x = scaleX;
  m_scale.y = scaleY;
  m_scale.z = scaleZ;
}

void rendering::WireframeObject::SetScale(rendering::Vector3 scale) {
  m_scale = scale;
}

rendering::Vector3 rendering::WireframeObject::GetScale() { return m_scale; }
