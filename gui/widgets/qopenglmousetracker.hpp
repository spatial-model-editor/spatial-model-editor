//
// Created by acaramizaru on 7/25/23.
//

#pragma once

#include <QOpenGLWidget>
#include <QTimer>
#include <QWidget>
#include <QtOpenGL>

#include "rendering/rendering.hpp"

class QOpenGLMouseTracker : public QOpenGLWidget {
  Q_OBJECT
public:
  QOpenGLMouseTracker(float lineWidth = 1.0f, float lineSelectPrecision = 10.0f,
                      QColor selectedObjectColor = QColor(255, 255, 0),
                      float cameraFOV = 60.0f, float cameraNearZ = 0.001f,
                      float cameraFarZ = 2000.0f, float frameRate = 30.0f);
  ~QOpenGLMouseTracker() = default;

  void SetCameraFrustum(GLfloat FOV, GLfloat width, GLfloat height,
                        GLfloat nearZ, GLfloat farZ);
  void SetCameraPosition(float x, float y, float z);
  void SetCameraOrientation(float x, float y, float z);

  QVector3D GetCameraPosition() const;
  QVector3D GetCameraOrientation() const;

  /**
   * @brief Number of visible submeshes is influenced by the number of colors.
   *
   * The colors are distributed to submeshes in the same order they are
   * provided. In case of a smaller number of colors, submeshes with higher
   * index will not be visible.
   */

  void SetSubMeshes(const sme::mesh::Mesh3d &mesh,
                    const std::vector<QColor> &colors = std::vector<QColor>(0));

  void setFPS(float frameRate);
  void setLineWidth(float lineWidth = 1.0f);
  void setLineSelectPrecision(float lineSelectPrecision = 10.0f);

  void setSelectedObjectColor(QColor color = QColor(255, 255, 0));

  QColor getSelectedObjectColor() const;

  // colour of pixel at last mouse click position
  [[nodiscard]] QRgb getColour() const;

  void setSubmeshVisibility(uint32_t meshID, bool visibility);

  /**
   * @brief If color is not valid returns -1 otherwise, return positive index.
   *
   */
  std::size_t meshIDFromColor(const QColor &color) const;

signals:
  void mouseClicked(QRgb color, uint32_t meshID);
  void mouseOver(uint32_t meshID);
  void mouseWheelEvent(QWheelEvent *ev);

protected:
  QRgb m_lastColour{};

#ifdef QT_DEBUG
  QOpenGLDebugLogger *m_debugLogger;
#endif

  float m_frameRate;

  float m_lineWidth;
  float m_lineSelectPrecision;

  QColor m_selectedObjectColor;

  std::unique_ptr<rendering::WireframeObjects> m_SubMeshes;

  std::unique_ptr<rendering::ShaderProgram> m_mainProgram;
  rendering::Camera m_camera;

  QImage m_offscreenPickingImage;

  int m_xAtPress;
  int m_yAtPress;

  void initializeGL() override;
  void resizeGL(int w, int h) override;
  void paintGL() override;

  void renderScene(float widthLine);

  void mouseMoveEvent(QMouseEvent *event) override;

  void mousePressEvent(QMouseEvent *event) override;

  void wheelEvent(QWheelEvent *event) override;
};
