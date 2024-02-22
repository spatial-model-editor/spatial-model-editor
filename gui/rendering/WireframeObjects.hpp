//
// Created by acaramizaru on 6/30/23.
//

#pragma once

#include "ShaderProgram.hpp"
#include "Utils.hpp"
#include <vector>

#include "sme/mesh3d.hpp"

#include <QOpenGLWidget>
#include <QtOpenGL>

namespace rendering {

class WireframeObjects : protected QOpenGLFunctions {

public:
  WireframeObjects(const sme::mesh::Mesh3d &info, const QOpenGLWidget *Widget,
                   const std::vector<QColor> &color = std::vector<QColor>(0),
                   const QVector3D &position = QVector3D(0.0f, 0.0f, 0.0f),
                   const QVector3D &rotation = QVector3D(0.0f, 0.0f, 0.0f),
                   const QVector3D &scale = QVector3D(1.0f, 1.0f, 1.0f));
  WireframeObjects(const WireframeObjects &cpy) = delete;
  ~WireframeObjects();

  void Render(std::unique_ptr<rendering::ShaderProgram> &program,
              float lineWidth = 1);

  void SetRotation(GLfloat rotationX, GLfloat rotationY, GLfloat rotationZ);
  void SetRotation(const QVector3D &rotation);
  QVector3D GetRotation() const;

  void SetPosition(GLfloat positionX, GLfloat positionY, GLfloat positionZ);
  void SetPosition(const QVector3D &position);
  QVector3D GetPosition() const;

  void SetScale(GLfloat scaleX, GLfloat scaleY, GLfloat scaleZ);
  void SetScale(const QVector3D &scale);
  QVector3D GetScale() const;

  //  void SetColor(const QColor &color);
  void SetColor(const QColor &color, uint32_t meshID);
  void ResetDefaultColor(uint32_t meshID);

  uint32_t GetNumberOfSubMeshes();

  std::vector<QColor> GetDefaultColors() const;
  std::vector<QColor> GetCurrentColors() const;

  void setSubmeshVisibility(uint32_t meshID, bool visibility);

  /**
   * @brief If color is not valid returns -1 otherwise, return positive index.
   *
   **/
  std::size_t meshIDFromColor(const QColor &color) const;

private:
  std::vector<QVector4D> m_vertices;
  std::vector<std::vector<GLuint>> m_indices;
  //  QColor m_color;
  std::vector<QColor> m_colors;
  std::vector<QColor> m_default_colors;

  QOpenGLContext *m_openGLContext;

  std::vector<GLfloat> m_verticesBuffer;

  std::unique_ptr<QOpenGLVertexArrayObject> m_vao;
  GLuint m_vbo;
  std::vector<GLuint> m_elementBufferIds;
  std::vector<bool> m_visibleSubmesh;

  QVector3D m_rotation;
  QVector3D m_position;
  QVector3D m_scale;

  void CreateVBO();
  void DestroyVBO();
};

} // namespace rendering
