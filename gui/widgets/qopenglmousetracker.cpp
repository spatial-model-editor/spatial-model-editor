//
// Created by acaramizaru on 7/25/23.
//

#include "qopenglmousetracker.h"

QOpenGLMouseTracker::QOpenGLMouseTracker(QWidget *parent, float lineWidth,
                                         float lineSelectPrecision,
                                         QColor selectedObjectColor,
                                         float cameraFOV, float cameraNearZ,
                                         float cameraFarZ, float frameRate)
    : camera(cameraFOV, size().width(), size().height(), cameraNearZ,
             cameraFarZ) {

  setFPS(frameRate);

  setLineWidth(lineWidth);
  setLineSelectPrecision(lineSelectPrecision);
  setSelectedObjectColor(selectedObjectColor);
}

void QOpenGLMouseTracker::initializeGL() {

#ifdef QT_DEBUG

  SPDLOG_INFO("GL_KHR_debug extension available: " +
              std::to_string(
                  context()->hasExtension(QByteArrayLiteral("GL_KHR_debug"))));

  // create debug logger
  m_debugLogger = new QOpenGLDebugLogger(this);

  // initialize logger & display messages
  if (m_debugLogger->initialize()) {
    SPDLOG_INFO("QOpenGLDebugLogger initialized!");
    connect(m_debugLogger, &QOpenGLDebugLogger::messageLogged, this,
            [](const QOpenGLDebugMessage &msg) {
              rendering::Utils::GLDebugMessageCallback(
                  msg.source(), msg.type(), msg.id(), msg.severity(),
                  msg.message().toStdString().c_str());
            });
    m_debugLogger->startLogging(QOpenGLDebugLogger::SynchronousLogging);
  } else {
    SPDLOG_INFO("QOpenGLDebugLogger was NOT initialized!");
  }

#endif

  std::string ext =
      QString::fromLatin1(
          (const char *)context()->functions()->glGetString(GL_EXTENSIONS))
          .replace(' ', "\n\t")
          .toStdString();
  CheckOpenGLError("glGetString(GL_EXTENSIONS)");

  std::string vendor(
      (const char *)context()->functions()->glGetString(GL_VENDOR));
  CheckOpenGLError("glGetString(GL_VENDOR)");
  std::string renderer(
      (const char *)context()->functions()->glGetString(GL_RENDERER));
  CheckOpenGLError("glGetString(GL_RENDERER)");
  std::string gl_version(
      (const char *)context()->functions()->glGetString(GL_VERSION));
  CheckOpenGLError("glGetString(GL_VERSION)");

  SPDLOG_INFO("OpenGL: " + vendor + std::string(" ") + renderer +
              std::string(" ") + gl_version + std::string(" ") +
              std::string("\n\n\t") + ext + std::string("\n"));

  mainProgram =
      std::unique_ptr<rendering::ShaderProgram>(new rendering::ShaderProgram(
          rendering::text_vertex, rendering::text_fragment));
}

void QOpenGLMouseTracker::renderScene(float lineWidth) {
  for (color_mesh &obj : meshSet) {
    obj.second->Render(mainProgram, lineWidth);
  }
}

void QOpenGLMouseTracker::paintGL() {

  if (!context()->isValid()) {
    SPDLOG_ERROR("Widget context is not valid");
    return;
  }

  if (QOpenGLContext::currentContext() != context()) {
    SPDLOG_ERROR("Widget Context is not the active one!");
    return;
  }

  camera.UpdateView(mainProgram);
  camera.UpdateProjection(mainProgram);

  renderScene(lineWidth);

  QOpenGLFramebufferObject fboPicking(size());
  fboPicking.bind();

  context()->functions()->glViewport(0, 0, size().width(), size().height());

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

  m_xAtPress = event->position().x();
  m_yAtPress = event->position().y();

  m_xAtPress = std::clamp(m_xAtPress, 0, offscreenPickingImage.width() - 1);
  m_yAtPress = std::clamp(m_yAtPress, 0, offscreenPickingImage.height() - 1);

  lastColour = offscreenPickingImage.pixel(m_xAtPress, m_yAtPress);
  QColor color(lastColour);

  bool objectSelected = false;

  SPDLOG_INFO("mousePressEvent at: X:" + std::to_string(m_xAtPress) +
              std::string(" Y:") + std::to_string(m_yAtPress));
  SPDLOG_INFO("color :" + color.name().toStdString());

  for (color_mesh &obj : meshSet) {
    if (obj.first == color) {
      obj.second->SetColor(selectedObjectColor);
      objectSelected = true;
      emit mouseClicked(lastColour, obj.second->GetMesh());

      SPDLOG_INFO("Object touched!");
    }
  }

  if (!objectSelected) {
    for (color_mesh &obj : meshSet) {
      auto defaultColor = obj.first;
      obj.second->SetColor(defaultColor);
    }

    SPDLOG_INFO("Reset state for selected objects to UNSELECTED objects!");
  }

  repaint();
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

  repaint();

  QRgb pixel = offscreenPickingImage.pixel(xAtPress, yAtPress);
  QColor color(pixel);

  for (color_mesh &obj : meshSet) {
    if (obj.first == color) {
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

  repaint();
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

void QOpenGLMouseTracker::addMesh(const rendering::SMesh &mesh, QColor color) {
  rendering::ObjectInfo objectInfo = rendering::ObjectLoader::Load(mesh);

  meshSet.push_back(std::make_pair(
      color,
      std::unique_ptr<rendering::WireframeObject>(
          new rendering::WireframeObject(objectInfo, color, mesh, this))));
}

void QOpenGLMouseTracker::setFPS(float frameRate) {
  this->frameRate = frameRate;
}

void QOpenGLMouseTracker::setLineWidth(float lineWidth) {
  this->lineWidth = lineWidth;
}

void QOpenGLMouseTracker::setLineSelectPrecision(float lineSelectPrecision) {
  this->lineSelectPrecision = lineSelectPrecision;
}

void QOpenGLMouseTracker::setSelectedObjectColor(QColor color) {
  this->selectedObjectColor = color;
}

QRgb QOpenGLMouseTracker::getColour() const { return lastColour; }

QColor QOpenGLMouseTracker::getSelectedObjectColor() {
  return selectedObjectColor;
}
