//
// Created by acaramizaru on 6/30/23.
//

#include "WireframeObjects.hpp"
#include <limits>
#include <memory>

template <typename T>
qopengl_GLsizeiptr sizeofGLVector(const std::vector<T> &v) {
  return static_cast<qopengl_GLsizeiptr>(v.size() * sizeof(T));
}

rendering::WireframeObjects::WireframeObjects(
    const sme::mesh::Mesh3d &info, const QOpenGLWidget *Widget,
    const std::vector<QColor> &colors, GLfloat meshThickness,
    const QVector3D &meshPositionOffset, const QVector3D &position,
    const QVector3D &rotation, const QVector3D &scale)
    : m_vertices(info.getVerticesAsQVector4DArrayInHomogeneousCoord()),
      m_visibleSubmesh(std::min(colors.size(), info.getNumberOfCompartment()),
                       true),
      m_openGLContext(Widget->context()), m_defaultColors(colors),
      m_colors(colors), m_meshThickness(colors.size(), meshThickness),
      m_defaultThickness(colors.size(), meshThickness),
      m_translationOffset(meshPositionOffset),
      Node("WireframeObjects", position, rotation, scale) {

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

void rendering::WireframeObjects::ResetToDefaultColor(uint32_t meshID) {
  assert(meshID < m_colors.size());
  m_colors[meshID] = m_defaultColors[meshID];
}

uint32_t rendering::WireframeObjects::GetNumberOfSubMeshes() const {
  return static_cast<uint32_t>(m_indices.size());
}

std::vector<QColor> rendering::WireframeObjects::GetDefaultColors() const {
  return m_defaultColors;
}

std::vector<QColor> rendering::WireframeObjects::GetCurrentColors() const {
  return m_colors;
}

void rendering::WireframeObjects::SetThickness(const GLfloat thickness,
                                               uint32_t meshID) {
  assert(meshID < m_meshThickness.size());
  m_meshThickness[meshID] = thickness;
}

void rendering::WireframeObjects::ResetToDefaultThickness(uint32_t meshID) {
  assert(meshID < m_meshThickness.size());
  m_meshThickness[meshID] = m_defaultThickness[meshID];
}

std::vector<GLfloat> rendering::WireframeObjects::GetDefaultThickness() const {
  return m_defaultThickness;
}

std::vector<GLfloat> rendering::WireframeObjects::GetCurrentThickness() const {
  return m_meshThickness;
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
  return std::numeric_limits<std::size_t>::max();
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
  for (std::size_t i = 0; i < m_elementBufferIds.size(); i++) {
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_elementBufferIds[i]);
    CheckOpenGLError("glBindBuffer");
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeofGLVector(m_indices[i]),
                 m_indices[i].data(), GL_STATIC_DRAW);
    CheckOpenGLError("glBufferData");
  }
}

void rendering::WireframeObjects::DestroyVBO() {

  //    m_openGLContext->makeCurrent(m_openGLContext->surface());

  glBindBuffer(GL_ARRAY_BUFFER, 0);

  glDeleteBuffers(1, &m_vbo);
  glDeleteBuffers(static_cast<GLsizei>(m_elementBufferIds.size()),
                  m_elementBufferIds.data());

  m_vao->release();
  m_vao->destroy();
}

void rendering::WireframeObjects::RenderSetup(
    const std::unique_ptr<rendering::ShaderProgram> &program) {

  glDisable(GL_CULL_FACE);
  CheckOpenGLError("glDisable(GL_CULL_FACE)");
  glEnable(GL_DEPTH_TEST);
  CheckOpenGLError("glEnable(GL_DEPTH_TEST)");
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
}

void rendering::WireframeObjects::Render(
    const std::unique_ptr<rendering::ShaderProgram> &program,
    std::optional<float> lineWidth) {

  m_openGLContext->makeCurrent(m_openGLContext->surface());

  glEnableVertexAttribArray(0);
  CheckOpenGLError("glEnableVertexAttribArray");

  RenderSetup(program);

  m_vao->bind();
  for (std::size_t i = 0; i < m_indices.size(); i++) {
    if (i >= m_colors.size())
      break;
    if (!m_visibleSubmesh[i])
      continue;
    program->SetColor(m_colors[i].redF(), m_colors[i].greenF(),
                      m_colors[i].blueF(), m_colors[i].alphaF());
    program->SetThickness(lineWidth.value_or(m_meshThickness[i]));
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_elementBufferIds[i]);
    glDrawElements(GL_TRIANGLES, static_cast<int>(m_indices[i].size()),
                   GL_UNSIGNED_INT, (void *)nullptr);
  }

  glDisableVertexAttribArray(0);
  CheckOpenGLError("glDisableVertexAttribArray");
}
