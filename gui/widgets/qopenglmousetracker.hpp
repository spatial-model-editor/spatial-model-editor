//
// Created by acaramizaru on 7/25/23.
//

#pragma once

#include <set>

#include <QOpenGLWidget>
#include <QTimer>
#include <QWidget>
#include <QtOpenGL>
#include <optional>

#include "rendering/rendering.hpp"

class QOpenGLMouseTracker : public QOpenGLWidget {
  Q_OBJECT
public:
  friend class rendering::ClippingPlane;

  QOpenGLMouseTracker(QWidget *parent = nullptr, float lineWidth = 0.005f,
                      float lineSelectPrecision = 0.2f,
                      float lineWidthSelectedSubmesh = 0.1f,
                      QColor selectedObjectColor = QColor(255, 255, 0),
                      float cameraFOV = 60.0f, float cameraNearZ = 0.001f,
                      float cameraFarZ = 2000.0f, float frameRate = 30.0f);

  void SetCameraFrustum(GLfloat FOV, GLfloat width, GLfloat height,
                        GLfloat nearZ, GLfloat farZ);
  void SetCameraPosition(float x, float y, float z);
  void SetCameraOrientation(float x, float y, float z);

  QVector3D GetCameraPosition() const;
  QVector3D GetCameraOrientation() const;

  void SetSubMeshesOrientation(float x, float y, float z);
  void SetSubMeshesPosition(float x, float y, float z);

  QVector3D GetSubMeshesOrientation() const;
  QVector3D GetSubMeshesPosition() const;

  /**
   * @brief Number of visible submeshes is influenced by the number of colors.
   *
   * The colors are distributed to submeshes in the same order they are
   * provided. In case of a smaller number of colors, submeshes with higher
   * index will not be visible.
   *
   * Default colors used are taken from 'mesh'
   */

  void SetSubMeshes(const sme::mesh::Mesh3d &mesh,
                    const std::vector<QColor> &colors = std::vector<QColor>(0));

  void setFPS(float frameRate);
  void setLineWidth(float lineWidth = 0.005f);
  void setLineSelectPrecision(float lineSelectPrecision = 0.2f);

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

  void setBackgroundColor(QColor background);
  QColor getBackgroundColor() const;

  void clear();

signals:
  void mouseClicked(QRgb color, uint32_t meshID);
  void mouseOver(uint32_t meshID);
  void mouseWheelEvent(QWheelEvent *ev);

protected:
  QRgb m_lastColour;

#ifdef QT_DEBUG
  QOpenGLDebugLogger *m_debugLogger;
#endif

  float m_frameRate;

  float m_lineWidth;
  float m_lineSelectPrecision;
  float m_lineWidthSelectedSubmesh;

  QColor m_selectedObjectColor;

  std::unique_ptr<rendering::WireframeObjects> m_SubMeshes{};

  std::unique_ptr<rendering::ShaderProgram> m_mainProgram{};
  rendering::Camera m_camera;

  QImage m_offscreenPickingImage;

  int m_xAtPress;
  int m_yAtPress;

  QColor m_backgroundColor;

  std::set<rendering::ClippingPlane *> m_clippingPlanes;

  void addClippingPlane(rendering::ClippingPlane *clippingPlane);
  void deleteClippingPlane(rendering::ClippingPlane *clippingPlane);

  void initializeGL() override;
  void resizeGL(int w, int h) override;
  void paintGL() override;

  void renderScene(std::optional<float> widthLine = {});

  void updateAllClippingPlanes();

  void mouseMoveEvent(QMouseEvent *event) override;

  void mousePressEvent(QMouseEvent *event) override;

  void wheelEvent(QWheelEvent *event) override;
};
