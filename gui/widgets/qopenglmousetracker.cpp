//
// Created by acaramizaru on 7/25/23.
//

#include "qopenglmousetracker.h"
#include "rendering/Shaders/fragment.hpp"
#include "rendering/Shaders/vertex.hpp"

QOpenGLMouseTracker::QOpenGLMouseTracker(QWidget *parent, float lineWidth,
                                         float lineSelectPrecision,
                                         rendering::Vector4 selectedObjectColor,
                                         float cameraFOV, float cameraNearZ,
                                         float cameraFarZ, float frameRate)
    : camera(cameraFOV, size().width(), size().height(), cameraNearZ,
             cameraFarZ) {

  this->frameRate = frameRate;

  this->timer = new QTimer(this);
  connect(this->timer, SIGNAL(timeout()), this, SLOT(update()));
  timer->start(1 / frameRate * 1000);

  setLineWidth(lineWidth);
  setLineSelectPrecision(lineSelectPrecision);
  setSelectedObjectColor(selectedObjectColor);
}

QOpenGLMouseTracker::~QOpenGLMouseTracker() {
  delete mainProgram;

  for (color_mesh &obj : meshSet)
    delete obj.second;
}

void QOpenGLMouseTracker::initializeGL() {
  mainProgram = new rendering::ShaderProgram(rendering::text_vertex,
                                             rendering::text_fragment);

  mainProgram->Use();
}

void QOpenGLMouseTracker::renderScene(float lineWidth) {
  for (color_mesh &obj : meshSet) {
    obj.second->Render(mainProgram, lineWidth);
  }
}

void QOpenGLMouseTracker::paintGL() {
  camera.UpdateView(mainProgram);
  camera.UpdateProjection(mainProgram);

  renderScene(lineWidth);

  QOpenGLFramebufferObject fboPicking(size());
  fboPicking.bind();

  renderScene(lineSelectPrecision);

  offscreenPickingImage = fboPicking.toImage();

  fboPicking.bindDefault();
}

void QOpenGLMouseTracker::resizeGL(int w, int h) {
  camera.SetFrustum(camera.getFOV(), w, h, camera.getNear(), camera.getFar());
  this->update();
}

void QOpenGLMouseTracker::SetCameraFrustum(GLfloat FOV, GLfloat width,
                                           GLfloat height, GLfloat nearZ,
                                           GLfloat farZ) {
  camera.SetFrustum(FOV, width, height, nearZ, farZ);
}

void QOpenGLMouseTracker::mousePressEvent(QMouseEvent *event) {
  m_xAtPress = event->pos().x();
  m_yAtPress = event->pos().y();

  int xAtRelease = event->position().x();
  int yAtRelease = event->position().y();

  xAtRelease = std::clamp(xAtRelease, 0, offscreenPickingImage.width() - 1);
  yAtRelease = std::clamp(yAtRelease, 0, offscreenPickingImage.height() - 1);

  QRgb pixel = offscreenPickingImage.pixel(xAtRelease, yAtRelease);
  QColor color(pixel);

  rendering::Vector4 colorVector =
      rendering::Vector4(color.redF(), color.greenF(), color.blueF());

  bool objectSelected = false;

  for (color_mesh &obj : meshSet) {
    if (obj.first.ToArray() == colorVector.ToArray()) {
      obj.second->SetColor(selectedObjectColor);
      objectSelected = true;
      lastColour = pixel;
      emit mouseClicked(pixel, obj.second->GetMesh());
    }
  }

  if (!objectSelected) {
    for (color_mesh &obj : meshSet) {
      auto defaultColor = obj.first;
      obj.second->SetColor(defaultColor);
    }
  }
}

void QOpenGLMouseTracker::mouseMoveEvent(QMouseEvent *event) {

  int xAtPress = event->pos().x();
  int yAtPress = event->pos().y();

  xAtPress = std::clamp(xAtPress, 0, offscreenPickingImage.width() - 1);
  yAtPress = std::clamp(yAtPress, 0, offscreenPickingImage.height() - 1);

  int x_len = xAtPress - m_xAtPress;
  int y_len = yAtPress - m_yAtPress;

  m_xAtPress = xAtPress;
  m_yAtPress = yAtPress;

  // apply rotation of the camera
  rendering::Vector3 cameraOrientation = GetCameraOrientation();
  SetCameraOrientation(cameraOrientation.x + y_len * (1 / frameRate),
                       cameraOrientation.y + x_len * (1 / frameRate),
                       cameraOrientation.z);

  QRgb pixel = offscreenPickingImage.pixel(xAtPress, yAtPress);
  QColor color(pixel);

  rendering::Vector4 colorVector =
      rendering::Vector4(color.redF(), color.greenF(), color.blueF());

  for (color_mesh &obj : meshSet) {
    if (obj.first.ToArray() == colorVector.ToArray()) {
      emit mouseOver(obj.second->GetMesh());
    }
  }
}

void QOpenGLMouseTracker::wheelEvent(QWheelEvent *event) {
  auto Degrees = event->angleDelta().y() / 8;

  auto forwardVector = camera.GetForwardVector();
  auto position = camera.GetPosition();

  camera.SetPosition(position + forwardVector * Degrees * (1 / frameRate));

  emit mouseWheelEvent(event);
}

void QOpenGLMouseTracker::SetCameraPosition(float x, float y, float z) {
  camera.SetPosition(x, y, z);
}

void QOpenGLMouseTracker::SetCameraOrientation(float x, float y, float z) {
  camera.SetRotation(x, y, z);
}

rendering::Vector3 QOpenGLMouseTracker::GetCameraPosition() {
  return camera.GetPosition();
}

rendering::Vector3 QOpenGLMouseTracker::GetCameraOrientation() {
  return camera.GetRotation();
}

void QOpenGLMouseTracker::addMesh(rendering::SMesh &mesh,
                                  rendering::Vector4 color) {
  rendering::ObjectInfo objectInfo = rendering::ObjectLoader::Load(mesh);

  meshSet.push_back(std::make_pair(
      color, new rendering::WireframeObject(objectInfo, color, mesh)));
}

void QOpenGLMouseTracker::setFPS(float frameRate) {
  this->frameRate = frameRate;
  timer->start(1 / frameRate * 1000);
}

void QOpenGLMouseTracker::setLineWidth(float lineWidth) {
  this->lineWidth = lineWidth;
}

void QOpenGLMouseTracker::setLineSelectPrecision(float lineSelectPrecision) {
  this->lineSelectPrecision = lineSelectPrecision;
}

void QOpenGLMouseTracker::setSelectedObjectColor(rendering::Vector4 color) {
  this->selectedObjectColor = color;
}

const QRgb &QOpenGLMouseTracker::getColour() const { return lastColour; }
