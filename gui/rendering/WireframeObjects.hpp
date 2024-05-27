//
// Created by acaramizaru on 6/30/23.
//

#pragma once

#include "ShaderProgram.hpp"
#include "Utils.hpp"
#include "sme/mesh3d.hpp"
#include <optional>
#include <vector>

#include <QOpenGLWidget>
#include <QtOpenGL>

#include "Node.hpp"

namespace rendering {

class WireframeObjects : public Node, protected QOpenGLFunctions {

public:
  WireframeObjects(const sme::mesh::Mesh3d &info, const QOpenGLWidget *Widget,
                   const std::vector<QColor> &color = std::vector<QColor>(0),
                   GLfloat meshThickness = 0.005f,
                   const QVector3D &meshPositionOffset = QVector3D(0.0f, 0.0f,
                                                                   0.0f),
                   const QVector3D &position = QVector3D(0.0f, 0.0f, 0.0f),
                   const QVector3D &rotation = QVector3D(0.0f, 0.0f, 0.0f),
                   const QVector3D &scale = QVector3D(1.0f, 1.0f, 1.0f));

  WireframeObjects(const WireframeObjects &cpy) = delete;

  ~WireframeObjects() override;

  void Render(const std::unique_ptr<rendering::ShaderProgram> &program,
              std::optional<float> lineWidth = {});

  void SetColor(const QColor &color, uint32_t meshID);
  void ResetToDefaultColor(uint32_t meshID);

  uint32_t GetNumberOfSubMeshes() const;

  std::vector<QColor> GetDefaultColors() const;
  std::vector<QColor> GetCurrentColors() const;

  void SetThickness(const GLfloat thickness, uint32_t meshID);
  void SetAllThickness(const GLfloat thickness);
  void ResetToDefaultThickness(uint32_t meshID);
  void ResetAllToDefaultThickness();
  std::vector<GLfloat> GetDefaultThickness() const;
  std::vector<GLfloat> GetCurrentThickness() const;

  void setSubmeshVisibility(uint32_t meshID, bool visibility);

  /**
   * @brief If color is not valid returns -1 otherwise, return positive index.
   *
   **/
  std::size_t meshIDFromColor(const QColor &color) const;

  void setBackground(QColor backgroundColor);

protected:
  //  void update(float delta) override;
  void draw(std::unique_ptr<rendering::ShaderProgram> &program) override;

private:
  std::vector<QVector4D> m_vertices;
  std::vector<std::vector<GLuint>> m_indices;

  std::vector<bool> m_visibleSubmesh;
  QOpenGLContext *m_openGLContext;

  std::vector<QColor> m_defaultColors;
  std::vector<QColor> m_colors;
  std::vector<GLfloat> m_meshThickness;
  std::vector<GLfloat> m_defaultThickness;
  QVector3D m_translationOffset;

  std::vector<GLfloat> m_verticesBuffer;

  std::unique_ptr<QOpenGLVertexArrayObject> m_vao;
  GLuint m_vbo;
  std::vector<GLuint> m_elementBufferIds;

  QColor clearColor;

  void CreateVBO();
  void DestroyVBO();
  void RenderSetup(const std::unique_ptr<rendering::ShaderProgram> &program);
};

} // namespace rendering
