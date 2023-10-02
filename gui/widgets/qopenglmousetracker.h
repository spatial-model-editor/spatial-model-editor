//
// Created by acaramizaru on 7/25/23.
//

#ifndef SPATIALMODELEDITOR_QOPENGLMOUSETRACKER_H
#define SPATIALMODELEDITOR_QOPENGLMOUSETRACKER_H

#include <QOpenGLWidget>
#include <QWidget>
#include <QtOpenGL>

#include "rendering/rendering.hpp"

#include <QTimer>

class QOpenGLMouseTracker : public QOpenGLWidget {
  Q_OBJECT
public:
  QOpenGLMouseTracker(QWidget *parent = nullptr, float lineWidth = 1.0f,
                      float lineSelectPrecision = 10.0f,
                      QColor selectedObjectColor = QColor(255, 255, 0),
                      float cameraFOV = 60.0f, float cameraNearZ = 0.001f,
                      float cameraFarZ = 2000.0f, float frameRate = 30.0f);
  ~QOpenGLMouseTracker() = default;

  void SetCameraFrustum(GLfloat FOV, GLfloat width, GLfloat height,
                        GLfloat nearZ, GLfloat farZ);
  void SetCameraPosition(float x, float y, float z);
  void SetCameraOrientation(float x, float y, float z);

  rendering::Vector3 GetCameraPosition();
  rendering::Vector3 GetCameraOrientation();

  void addMesh(const rendering::SMesh &mesh, QColor color);

  void setFPS(float frameRate);
  void setLineWidth(float lineWidth = 1.0f);
  void setLineSelectPrecision(float lineSelectPrecision = 10.0f);

  void setSelectedObjectColor(QColor color = QColor(255, 255, 0));

  QColor getSelectedObjectColor();

  // colour of pixel at last mouse click position
  [[nodiscard]] QRgb getColour() const;

signals:
  void mouseClicked(QRgb color, rendering::SMesh mesh);
  void mouseOver(const rendering::SMesh &mesh);
  void mouseWheelEvent(QWheelEvent *ev);

protected:
  QRgb lastColour{};

#ifdef QT_DEBUG
  QOpenGLDebugLogger *m_debugLogger;
#endif

  float frameRate;

  float lineWidth;
  float lineSelectPrecision;

  QColor selectedObjectColor;

  typedef std::pair<QColor, std::unique_ptr<rendering::WireframeObject>>
      color_mesh;

  std::vector<color_mesh> meshSet;

  std::unique_ptr<rendering::ShaderProgram> mainProgram;
  rendering::Camera camera;

  QImage offscreenPickingImage;

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

#endif // SPATIALMODELEDITOR_QOPENGLMOUSETRACKER_H
