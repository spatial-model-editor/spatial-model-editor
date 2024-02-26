//
// Created by acaramizaru on 7/25/23.
//

#include "qopenglmousetracker.hpp"

QOpenGLMouseTracker::QOpenGLMouseTracker(float lineWidth,
                                         float lineSelectPrecision,
                                         QColor selectedObjectColor,
                                         float cameraFOV, float cameraNearZ,
                                         float cameraFarZ, float frameRate)
    : m_camera(cameraFOV, static_cast<float>(size().width()),
               static_cast<float>(size().height()), cameraNearZ, cameraFarZ),
      m_lineWidth(lineWidth), m_lineSelectPrecision(lineSelectPrecision),
      m_selectedObjectColor(selectedObjectColor), m_frameRate(frameRate),
      m_backgroundColor(QWidget::palette().color(QWidget::backgroundRole())) {}

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

  m_mainProgram =
      std::unique_ptr<rendering::ShaderProgram>(new rendering::ShaderProgram(
          rendering::text_vertex_color_as_uniform, rendering::text_fragment));
}

void QOpenGLMouseTracker::renderScene(float lineWidth) {

  if (m_SubMeshes) {
    m_SubMeshes->setBackground(m_backgroundColor);
    m_SubMeshes->Render(m_mainProgram, lineWidth);
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

  m_camera.UpdateView(m_mainProgram);
  m_camera.UpdateProjection(m_mainProgram);

  renderScene(m_lineWidth);

  QOpenGLFramebufferObject fboPicking(size());
  fboPicking.bind();

  context()->functions()->glViewport(0, 0, size().width(), size().height());

  renderScene(m_lineSelectPrecision);

  m_offscreenPickingImage = fboPicking.toImage();

  QOpenGLFramebufferObject::bindDefault();
}

void QOpenGLMouseTracker::resizeGL(int w, int h) {
  m_camera.SetFrustum(m_camera.getFOV(), static_cast<float>(w),
                      static_cast<float>(h), m_camera.getNear(),
                      m_camera.getFar());
  this->update();
}

void QOpenGLMouseTracker::SetCameraFrustum(GLfloat FOV, GLfloat width,
                                           GLfloat height, GLfloat nearZ,
                                           GLfloat farZ) {
  m_camera.SetFrustum(FOV, width, height, nearZ, farZ);
}

void QOpenGLMouseTracker::mousePressEvent(QMouseEvent *event) {

  m_xAtPress = static_cast<int>(event->position().x());
  m_yAtPress = static_cast<int>(event->position().y());

  m_xAtPress = std::clamp(m_xAtPress, 0, m_offscreenPickingImage.width() - 1);
  m_yAtPress = std::clamp(m_yAtPress, 0, m_offscreenPickingImage.height() - 1);

  m_lastColour = m_offscreenPickingImage.pixel(m_xAtPress, m_yAtPress);
  QColor color(m_lastColour);

  bool objectSelected = false;

  SPDLOG_INFO("mousePressEvent at: X:" + std::to_string(m_xAtPress) +
              std::string(" Y:") + std::to_string(m_yAtPress));
  SPDLOG_INFO("color :" + color.name().toStdString());

  auto defaultColors = m_SubMeshes->GetDefaultColors();
  for (uint32_t i = 0; i < defaultColors.size(); i++) {
    if (defaultColors[i] == color) {
      m_SubMeshes->SetColor(m_selectedObjectColor, i);
      objectSelected = true;
      emit mouseClicked(m_lastColour, i);

      SPDLOG_INFO("Object touched!");
    }
  }

  if (!objectSelected) {
    for (uint32_t i = 0; i < defaultColors.size(); i++) {
      m_SubMeshes->ResetDefaultColor(i);
    }

    SPDLOG_INFO("Reset state for selected objects to UNSELECTED objects!");
  }

  repaint();
}

void QOpenGLMouseTracker::mouseMoveEvent(QMouseEvent *event) {

  int xAtPress = event->pos().x();
  int yAtPress = event->pos().y();

  xAtPress = std::clamp(xAtPress, 0, m_offscreenPickingImage.width() - 1);
  yAtPress = std::clamp(yAtPress, 0, m_offscreenPickingImage.height() - 1);

  int x_len = xAtPress - m_xAtPress;
  int y_len = yAtPress - m_yAtPress;

  m_xAtPress = xAtPress;
  m_yAtPress = yAtPress;

  //  // apply rotation of the camera
  //  QVector3D cameraOrientation = GetCameraOrientation();
  //  SetCameraOrientation(
  //      cameraOrientation.x() + static_cast<float>(y_len) * (1 / m_frameRate),
  //      cameraOrientation.y() + static_cast<float>(x_len) * (1 / m_frameRate),
  //      cameraOrientation.z());

  // apply rotation to all sub-meshes
  QVector3D subMeshesOrientation = GetSubMeshesOrientation();
  SetSubMeshesOrientation(subMeshesOrientation.x() - static_cast<float>(y_len),
                          subMeshesOrientation.y() - static_cast<float>(x_len),
                          subMeshesOrientation.z());

  repaint();

  QRgb pixel = m_offscreenPickingImage.pixel(xAtPress, yAtPress);
  QColor color(pixel);

  auto defaultColors = m_SubMeshes->GetDefaultColors();
  for (uint32_t i = 0; i < defaultColors.size(); i++) {
    if (defaultColors[i] == color) {
      emit mouseOver(i);
    }
  }
}

void QOpenGLMouseTracker::wheelEvent(QWheelEvent *event) {
  auto Degrees = event->angleDelta().y() / 8;

  auto forwardVector = m_camera.GetForwardVector();
  auto position = m_camera.GetPosition();

  m_camera.SetPosition(position + forwardVector * static_cast<float>(Degrees) *
                                      (1 / m_frameRate));
  //  m_camera.SetPosition(position + forwardVector *
  //  static_cast<float>(Degrees));

  emit mouseWheelEvent(event);

  repaint();
}

void QOpenGLMouseTracker::SetCameraPosition(float x, float y, float z) {
  m_camera.SetPosition(x, y, z);
}

void QOpenGLMouseTracker::SetCameraOrientation(float x, float y, float z) {
  m_camera.SetRotation(x, y, z);
}

QVector3D QOpenGLMouseTracker::GetCameraPosition() const {
  return m_camera.GetPosition();
}

QVector3D QOpenGLMouseTracker::GetCameraOrientation() const {
  return m_camera.GetRotation();
}

void QOpenGLMouseTracker::SetSubMeshesOrientation(float x, float y, float z) {
  m_SubMeshes->SetRotation(x, y, z);
}

void QOpenGLMouseTracker::SetSubMeshesPosition(float x, float y, float z) {
  m_SubMeshes->SetPosition(x, y, z);
}

QVector3D QOpenGLMouseTracker::GetSubMeshesOrientation() const {
  return m_SubMeshes->GetRotation();
}

QVector3D QOpenGLMouseTracker::GetSubMeshesPosition() const {
  return m_SubMeshes->GetPosition();
}

void QOpenGLMouseTracker::SetSubMeshes(const sme::mesh::Mesh3d &mesh,
                                       const std::vector<QColor> &colors) {

  if (colors.size() == 0) {
    m_SubMeshes = std::unique_ptr<rendering::WireframeObjects>(
        new rendering::WireframeObjects(mesh, this, mesh.getColors(),
                                        mesh.getOffset()));
  } else {
    m_SubMeshes = std::unique_ptr<rendering::WireframeObjects>(
        new rendering::WireframeObjects(mesh, this, colors, mesh.getOffset()));
  }
}

void QOpenGLMouseTracker::setFPS(float frameRate) { m_frameRate = frameRate; }

void QOpenGLMouseTracker::setLineWidth(float lineWidth) {
  m_lineWidth = lineWidth;
}

void QOpenGLMouseTracker::setLineSelectPrecision(float lineSelectPrecision) {
  m_lineSelectPrecision = lineSelectPrecision;
}

void QOpenGLMouseTracker::setSelectedObjectColor(QColor color) {
  m_selectedObjectColor = color;
}

QRgb QOpenGLMouseTracker::getColour() const { return m_lastColour; }

void QOpenGLMouseTracker::setSubmeshVisibility(uint32_t meshID,
                                               bool visibility) {
  m_SubMeshes->setSubmeshVisibility(meshID, visibility);
}

std::size_t QOpenGLMouseTracker::meshIDFromColor(const QColor &color) const {
  return m_SubMeshes->meshIDFromColor(color);
}

QColor QOpenGLMouseTracker::getSelectedObjectColor() const {
  return m_selectedObjectColor;
}

void QOpenGLMouseTracker::setBackground(QColor background) {
  m_backgroundColor = background;
}
