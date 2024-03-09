//
// Created by acaramizaru on 6/30/23.
//

#include "WireframeObjects.hpp"
#include <memory>

template <typename T>
qopengl_GLsizeiptr sizeofGLVector(const std::vector<T> &v) {
  return static_cast<qopengl_GLsizeiptr>(v.size() * sizeof(T));
}

rendering::WireframeObjects::WireframeObjects(
    const sme::mesh::Mesh3d &info, const QOpenGLWidget *Widget,
    const std::vector<QColor> &colors, const QVector3D &meshPositionOffset,
    const QVector3D &position, const QVector3D &rotation,
    const QVector3D &scale)
    : m_vertices(info.getVerticesAsQVector4DArrayInHomogeneousCoord()),
      m_visibleSubmesh(std::min(colors.size(), info.getNumberOfCompartment()),
                       true),
      m_openGLContext(Widget->context()), m_default_colors(colors),
      m_colors(colors), m_translationOffset(meshPositionOffset),
      m_position(position), m_rotation(rotation), m_scale(scale) {

  m_openGLContext->makeCurrent(m_openGLContext->surface());
  QOpenGLFunctions::initializeOpenGLFunctions();

  m_indices.reserve(info.getNumberOfCompartment());
  for (size_t i = 0; i < info.getNumberOfCompartment(); i++) {
    m_indices.push_back(info.getMeshTrianglesIndicesAsFlatArray(i));
  }

  for (const auto &v : m_vertices) {
    m_verticesBuffer.insert(m_verticesBuffer.end(),
                            {v.x(), v.y(), v.z(), v.w()});
  }

  clearColor = QColor(0, 0, 0, 0);

  CreateVBO();
}

rendering::WireframeObjects::~WireframeObjects() { DestroyVBO(); }

void rendering::WireframeObjects::SetColor(const QColor &color,
                                           uint32_t meshID) {
  assert(meshID < m_colors.size());
  m_colors[meshID] = color;
}

void rendering::WireframeObjects::ResetDefaultColor(uint32_t meshID) {
  assert(meshID < m_colors.size());
  m_colors[meshID] = m_default_colors[meshID];
}

uint32_t rendering::WireframeObjects::GetNumberOfSubMeshes() const {
  return static_cast<uint32_t>(m_indices.size());
}

std::vector<QColor> rendering::WireframeObjects::GetDefaultColors() const {
  return m_default_colors;
}

std::vector<QColor> rendering::WireframeObjects::GetCurrentColors() const {
  return m_colors;
}

void rendering::WireframeObjects::setSubmeshVisibility(uint32_t meshID,
                                                       bool visibility) {
  assert(meshID < m_visibleSubmesh.size());
  m_visibleSubmesh[meshID] = visibility;
}

std::size_t
rendering::WireframeObjects::meshIDFromColor(const QColor &color) const {
  for (std::size_t i = 0; i < m_colors.size(); i++)
    if (m_colors[i] == color) {
      return i;
    }
  return -1;
}

void rendering::WireframeObjects::setBackground(QColor backgroundColor) {
  clearColor = backgroundColor;
}

void rendering::WireframeObjects::CreateVBO() {

  m_openGLContext->makeCurrent(m_openGLContext->surface());

  m_vao = std::make_unique<QOpenGLVertexArrayObject>(nullptr);
  m_vao->create();
  m_vao->bind();

  glGenBuffers(1, &m_vbo);
  CheckOpenGLError("glGenBuffers");
  glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
  CheckOpenGLError("glBindBuffer");
  glBufferData(GL_ARRAY_BUFFER, sizeofGLVector(m_verticesBuffer),
               m_verticesBuffer.data(), GL_STATIC_DRAW);
  CheckOpenGLError("glBufferData");
  glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, nullptr);
  CheckOpenGLError("glVertexAttribPointer");

  glEnableVertexAttribArray(0);
  CheckOpenGLError("glEnableVertexAttribArray");

  m_elementBufferIds.resize(m_indices.size());
  glGenBuffers(static_cast<GLsizei>(m_elementBufferIds.size()),
               m_elementBufferIds.data());
  CheckOpenGLError("glGenBuffers");
  for (int i = 0; i < m_elementBufferIds.size(); i++) {
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_elementBufferIds[i]);
    CheckOpenGLError("glBindBuffer");
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeofGLVector(m_indices[i]),
                 m_indices[i].data(), GL_STATIC_DRAW);
    CheckOpenGLError("glBufferData");
  }
}

void rendering::WireframeObjects::DestroyVBO() {

  m_openGLContext->makeCurrent(m_openGLContext->surface());

  glBindBuffer(GL_ARRAY_BUFFER, 0);

  glDeleteBuffers(1, &m_vbo);
  glDeleteBuffers(static_cast<GLsizei>(m_elementBufferIds.size()),
                  m_elementBufferIds.data());

  m_vao->release();
  m_vao->destroy();
}

void rendering::WireframeObjects::Render(
    const std::unique_ptr<rendering::ShaderProgram> &program, float lineWidth) {

  m_openGLContext->makeCurrent(m_openGLContext->surface());

  glEnableVertexAttribArray(0);
  CheckOpenGLError("glEnableVertexAttribArray");

  // glLineWidth(lineWidth);
  program->SetThickness(lineWidth);
  // glDisable(GL_CULL_FACE);
  glEnable(GL_DEPTH_TEST);
  glClearColor(clearColor.redF(), clearColor.greenF(), clearColor.blueF(),
               clearColor.alphaF());
  program->SetBackgroundColor(clearColor.redF(), clearColor.greenF(),
                              clearColor.blueF());
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

  program->SetMeshTranslationOffset(m_translationOffset.x(),
                                    m_translationOffset.y(),
                                    m_translationOffset.z());
  program->SetPosition(m_position.x(), m_position.y(), m_position.z());
  program->SetRotation(m_rotation.x(), m_rotation.y(), m_rotation.z());
  program->SetScale(m_scale.x(), m_scale.y(), m_scale.z());
  m_vao->bind();
  for (int i = 0; i < m_indices.size(); i++) {
    if (i >= m_colors.size())
      break;
    if (!m_visibleSubmesh[i])
      continue;
    program->SetColor(m_colors[i].redF(), m_colors[i].greenF(),
                      m_colors[i].blueF(), m_colors[i].alphaF());
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_elementBufferIds[i]);
    glDrawElements(GL_TRIANGLES, static_cast<int>(m_indices[i].size()),
                   GL_UNSIGNED_INT, (void *)nullptr);
  }

  glDisableVertexAttribArray(0);
  CheckOpenGLError("glDisableVertexAttribArray");
}

void rendering::WireframeObjects::SetRotation(GLfloat rotationX,
                                              GLfloat rotationY,
                                              GLfloat rotationZ) {
  m_rotation.setX(rotationX);
  m_rotation.setY(rotationY);
  m_rotation.setZ(rotationZ);
}

void rendering::WireframeObjects::SetRotation(const QVector3D &rotation) {
  m_rotation = rotation;
}

QVector3D rendering::WireframeObjects::GetRotation() const {
  return m_rotation;
}

void rendering::WireframeObjects::SetPosition(GLfloat positionX,
                                              GLfloat positionY,
                                              GLfloat positionZ) {
  m_position.setX(positionX);
  m_position.setY(positionY);
  m_position.setZ(positionZ);
}

void rendering::WireframeObjects::SetPosition(const QVector3D &position) {
  m_position = position;
}

QVector3D rendering::WireframeObjects::GetPosition() const {
  return m_position;
}

void rendering::WireframeObjects::SetScale(GLfloat scaleX, GLfloat scaleY,
                                           GLfloat scaleZ) {
  m_scale.setX(scaleX);
  m_scale.setY(scaleY);
  m_scale.setZ(scaleZ);
}

void rendering::WireframeObjects::SetScale(const QVector3D &scale) {
  m_scale = scale;
}

QVector3D rendering::WireframeObjects::GetScale() const { return m_scale; }
