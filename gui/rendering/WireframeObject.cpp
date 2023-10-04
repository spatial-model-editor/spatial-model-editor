//
// Created by acaramizaru on 6/30/23.
//

#include "WireframeObject.hpp"

rendering::WireframeObject::WireframeObject(const rendering::ObjectInfo &info,
                                            QColor color,
                                            const rendering::SMesh &mesh,
                                            QOpenGLWidget *Widget,
                                            QVector3D position,
                                            QVector3D rotation, QVector3D scale)
    : m_mesh(mesh), m_openGLContext(Widget->context()), m_position(position),
      m_rotation(rotation), m_scale(scale), m_color(color), m_vertices(info.vertices) {

  m_openGLContext->makeCurrent(m_openGLContext->surface());
  QOpenGLFunctions::initializeOpenGLFunctions();

  for (rendering::Face f : info.faces) {
    m_indices.push_back(f.vertexIndices[0] - 1);
    m_indices.push_back(f.vertexIndices[1] - 1);

    m_indices.push_back(f.vertexIndices[1] - 1);
    m_indices.push_back(f.vertexIndices[2] - 1);

    m_indices.push_back(f.vertexIndices[2] - 1);
    m_indices.push_back(f.vertexIndices[0] - 1);
  }

  std::vector<uint8_t> cArr = {(uint8_t)m_color.red(), (uint8_t)m_color.green(),
                               (uint8_t)m_color.blue(),
                               (uint8_t)m_color.alpha()};

  for (const auto &v : m_vertices) {
    m_verticesBuffer.insert(m_verticesBuffer.end(),
                            {v.x(), v.y(), v.z(), v.w()});
    m_colorBuffer.insert(m_colorBuffer.end(), cArr.begin(), cArr.end());
  }

  CreateVBO();
}

rendering::WireframeObject::~WireframeObject(void) { DestroyVBO(); }

void rendering::WireframeObject::SetColor(QColor color) {
  m_color = color;
  UpdateVBOColor();
}

rendering::SMesh rendering::WireframeObject::GetMesh() const { return m_mesh; }

void rendering::WireframeObject::UpdateVBOColor() {

  m_openGLContext->makeCurrent(m_openGLContext->surface());

  int size = static_cast<int>(m_colorBuffer.size() / 4);

  std::vector<uint8_t> cArr = {(uint8_t)m_color.red(), (uint8_t)m_color.green(),
                               (uint8_t)m_color.blue(),
                               (uint8_t)m_color.alpha()};

  m_colorBuffer.clear();

  for (int iter = 0; iter < size; iter++)
    m_colorBuffer.insert(m_colorBuffer.end(), cArr.begin(), cArr.end());

  GLsizeiptr colorBufferSize = m_colorBuffer.size() * sizeof(uint8_t);

  m_vao->bind();

  glBindBuffer(GL_ARRAY_BUFFER, m_colorBufferId);
  glBufferData(GL_ARRAY_BUFFER, colorBufferSize, m_colorBuffer.data(),
               GL_DYNAMIC_DRAW);
  glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_FALSE, 0, nullptr);
  glEnableVertexAttribArray(1);
}

void rendering::WireframeObject::CreateVBO(void) {

  m_openGLContext->makeCurrent(m_openGLContext->surface());

  GLsizeiptr vboSize = m_verticesBuffer.size() * sizeof(GLfloat);
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
}

void rendering::WireframeObject::Render(
    std::unique_ptr<rendering::ShaderProgram> &program, float lineWidth) {

  glLineWidth(lineWidth);
  glEnable(GL_LINE_SMOOTH);

  program->SetPosition(m_position.x(), m_position.y(), m_position.z());
  program->SetRotation(m_rotation.x(), m_rotation.y(), m_rotation.z());
  program->SetScale(m_scale.x(), m_scale.y(), m_scale.z());
  m_vao->bind();
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_elementBufferId);
  glDrawElements(GL_LINES, static_cast<int>(m_indices.size()), GL_UNSIGNED_INT, (void *)nullptr);
}

void rendering::WireframeObject::SetRotation(GLfloat rotationX,
                                             GLfloat rotationY,
                                             GLfloat rotationZ) {
  m_rotation.setX(rotationX);
  m_rotation.setY(rotationY);
  m_rotation.setY(rotationZ);
}

void rendering::WireframeObject::SetRotation(QVector3D rotation) {
  m_rotation = rotation;
}

QVector3D rendering::WireframeObject::GetRotation() const{ return m_rotation; }

void rendering::WireframeObject::SetPosition(GLfloat positionX,
                                             GLfloat positionY,
                                             GLfloat positionZ) {
  m_position.setX(positionX);
  m_position.setY(positionY);
  m_position.setZ(positionZ);
}

void rendering::WireframeObject::SetPosition(QVector3D position) {
  m_position = position;
}

QVector3D rendering::WireframeObject::GetPosition() const { return m_position; }

void rendering::WireframeObject::SetScale(GLfloat scaleX, GLfloat scaleY,
                                          GLfloat scaleZ) {
  m_scale.setX(scaleX);
  m_scale.setY(scaleY);
  m_scale.setZ(scaleZ);
}

void rendering::WireframeObject::SetScale(QVector3D scale) { m_scale = scale; }

QVector3D rendering::WireframeObject::GetScale() const { return m_scale; }
