//
// Created by acaramizaru on 7/25/23.
//

#ifndef SPATIALMODELEDITOR_QOPENGLMOUSETRACKER_H
#define SPATIALMODELEDITOR_QOPENGLMOUSETRACKER_H

#include <QWidget>
#include <QOpenGLWidget>
#include <QtOpenGL>

#include "rendering/rendering.hpp"

#include <QTimer>

class QOpenGLMouseTracker:
    public QOpenGLWidget,
    protected QOpenGLFunctions
{
public:

  QOpenGLMouseTracker(
      QWidget *parent = nullptr,
      float lineWidth=1.0f,
      float lineSelectPrecision=10.0f,
      rendering::Vector4 selectedObjectColor = rendering::Vector4(1.0f, 1.0f, 0.0f),
      float cameraFOV=60.0f,
      float cameraNearZ = 0.001f,
      float cameraFarZ = 2000.0f,
      float frameRate = 60.0f
      );
  ~QOpenGLMouseTracker();

  void SetCameraFrustum(GLfloat FOV, GLfloat width, GLfloat height, GLfloat nearZ, GLfloat farZ);
  void SetCameraPosition(float x, float y, float z);
  void SetCameraSetRotation(float x, float y, float z);

  rendering::Vector3 GetCameraPosition();
  rendering::Vector3 GetCameraOrientation();

  void addMesh(rendering::SMesh mesh, rendering::Vector4 color);

  void setFPS(float frameRate);
  void setLineWidth(float lineWidth=1.0f);
  void setLineSelectPrecision(float lineSelectPrecision=10.0f);

  void setSelectedObjectColor(rendering::Vector4 color = rendering::Vector4(1.0f, 1.0f, 0.0f))
  {
    this->selectedObjectColor = color;
  }

protected:

  QTimer *timer;
  float frameRate;

  float lineWidth;
  float lineSelectPrecision;

  rendering::Vector4 selectedObjectColor;

  typedef std::pair<rendering::Vector4, rendering::WireframeObject*> color_mesh;

  std::vector<color_mesh> meshSet;

  rendering::ShaderProgram* mainProgram;
  rendering::Camera camera;

  QImage offscreenPickingImage;

  int m_xAtPress;
  int m_yAtPress;

  void initializeGL() override;
  void resizeGL(int w, int h) override;
  void paintGL() override;

  QOpenGLFramebufferObject *createFramebufferObject(const QSize &size);

  void render(float widthLine);

  void mouseMoveEvent(QMouseEvent *event);

  void mousePressEvent(QMouseEvent *event);

  void wheelEvent(QWheelEvent *event);
};


#endif // SPATIALMODELEDITOR_QOPENGLMOUSETRACKER_H
