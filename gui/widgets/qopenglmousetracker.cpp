//
// Created by acaramizaru on 7/25/23.
//

#include "qopenglmousetracker.hpp"

QOpenGLMouseTracker::QOpenGLMouseTracker(QWidget *parent, float lineWidth,
                                         float lineSelectPrecision,
                                         float lineWidthSelectedSubmesh,
                                         QColor selectedObjectColor,
                                         float cameraFOV, float cameraNearZ,
                                         float cameraFarZ, float frameRate)
    : QOpenGLWidget(parent), m_lineWidth(lineWidth),
      m_lineSelectPrecision(lineSelectPrecision),
      m_lineWidthSelectedSubmesh(lineWidthSelectedSubmesh),
      m_selectedObjectColor(selectedObjectColor),
      m_camera(cameraFOV, static_cast<float>(size().width()),
               static_cast<float>(size().height()), cameraNearZ, cameraFarZ),
      m_frameRate(frameRate),
      m_backgroundColor(QWidget::palette().color(QWidget::backgroundRole())),
      m_lastColour(QWidget::palette().color(QWidget::backgroundRole()).rgb()) {}

std::shared_ptr<rendering::ClippingPlane>
QOpenGLMouseTracker::BuildClippingPlane(GLfloat a, GLfloat b, GLfloat c,
                                        GLfloat d, bool active) {

  auto it = m_clippingPlanesPool.begin();

  if (it == m_clippingPlanesPool.end())
    return std::shared_ptr<rendering::ClippingPlane>(nullptr);

  auto clippingPlane = *it;
  clippingPlane->SetClipPlane(a, b, c, d);

  if (active) {
    clippingPlane->Enable();
  } else {
    clippingPlane->Disable();
  }

  m_clippingPlanes.insert(clippingPlane);
  m_clippingPlanesPool.erase(clippingPlane);

  update();

  return clippingPlane;
}

std::shared_ptr<rendering::ClippingPlane>
QOpenGLMouseTracker::BuildClippingPlane(const QVector3D &normal,
                                        const QVector3D &point, bool active) {

  auto it = m_clippingPlanesPool.begin();

  if (it == m_clippingPlanesPool.end())
    return std::shared_ptr<rendering::ClippingPlane>(nullptr);

  auto clippingPlane = *it;
  clippingPlane->SetClipPlane(normal, point);

  if (active) {
    clippingPlane->Enable();
  } else {
    clippingPlane->Disable();
  }

  m_clippingPlanes.insert(clippingPlane);
  m_clippingPlanesPool.erase(clippingPlane);

  update();

  return clippingPlane;
}

void QOpenGLMouseTracker::DestroyClippingPlane(
    std::shared_ptr<rendering::ClippingPlane> clippingPlane) {

  auto it = m_clippingPlanes.find(clippingPlane);

  if (it == m_clippingPlanes.end())
    return;

  m_clippingPlanesPool.insert(*it);
  m_clippingPlanes.erase(it);

  update();
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

  m_mainProgram = std::make_unique<rendering::ShaderProgram>(
      rendering::shader::colorAsUniform::text_vertex_color_as_uniform,
      rendering::shader::default_::text_geometry,
      rendering::shader::default_::text_fragment);
}

void QOpenGLMouseTracker::renderScene(std::optional<float> lineWidth) {
  if (m_SubMeshes) {
    m_SubMeshes->setBackground(m_backgroundColor);
    m_SubMeshes->Render(m_mainProgram, lineWidth);
  }
}

void QOpenGLMouseTracker::updateAllClippingPlanes() {

  // start with a clean opengl set of clipping planes.
  m_mainProgram->DisableAllClippingPlanes();

  for (auto &plane : m_clippingPlanes) {
    plane->UpdateClipPlane(m_mainProgram);
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

  updateAllClippingPlanes();

  renderScene();

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

  if (m_SubMeshes == nullptr) {
    return;
  }
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
      m_SubMeshes->SetThickness(m_lineWidthSelectedSubmesh, i);
      objectSelected = true;
      emit mouseClicked(m_lastColour, i);

      SPDLOG_INFO("Object touched!");
    }
  }

  if (!objectSelected) {
    for (uint32_t i = 0; i < defaultColors.size(); i++) {
      m_SubMeshes->ResetToDefaultThickness(i);
    }

    SPDLOG_INFO("Reset state for selected objects to UNSELECTED objects!");
  }

  update();
}

void QOpenGLMouseTracker::mouseMoveEvent(QMouseEvent *event) {
  if (m_SubMeshes == nullptr) {
    return;
  }

  int xAtPress = event->pos().x();
  int yAtPress = event->pos().y();

  xAtPress = std::clamp(xAtPress, 0, m_offscreenPickingImage.width() - 1);
  yAtPress = std::clamp(yAtPress, 0, m_offscreenPickingImage.height() - 1);

  int x_len = xAtPress - m_xAtPress;
  int y_len = yAtPress - m_yAtPress;

  m_xAtPress = xAtPress;
  m_yAtPress = yAtPress;

  // apply rotation to all sub-meshes
  QVector3D subMeshesOrientation = GetSubMeshesOrientation();
  SetSubMeshesOrientation(subMeshesOrientation.x() - static_cast<float>(y_len),
                          subMeshesOrientation.y() - static_cast<float>(x_len),
                          subMeshesOrientation.z());

  update();

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
  auto position = m_camera.getPos();

  m_camera.setPos(position + forwardVector * static_cast<float>(Degrees) *
                                 (1 / m_frameRate));

  emit mouseWheelEvent(event);

  update();
}

void QOpenGLMouseTracker::SetCameraPosition(float x, float y, float z) {
  m_camera.setPos(x, y, z);
}

void QOpenGLMouseTracker::SetCameraOrientation(float x, float y, float z) {
  m_camera.setRot(x, y, z);
}

QVector3D QOpenGLMouseTracker::GetCameraPosition() const {
  return m_camera.getPos();
}

QVector3D QOpenGLMouseTracker::GetCameraOrientation() const {
  return m_camera.getRot();
}

void QOpenGLMouseTracker::SetSubMeshesOrientation(float x, float y, float z) {
  m_SubMeshes->setRot(x, y, z);
}

void QOpenGLMouseTracker::SetSubMeshesPosition(float x, float y, float z) {
  m_SubMeshes->setPos(x, y, z);
}

QVector3D QOpenGLMouseTracker::GetSubMeshesOrientation() const {
  return m_SubMeshes->getRot();
}

QVector3D QOpenGLMouseTracker::GetSubMeshesPosition() const {
  return m_SubMeshes->getPos();
}

void QOpenGLMouseTracker::SetSubMeshes(const sme::mesh::Mesh3d &mesh,
                                       const std::vector<QColor> &colors) {
  if (colors.empty()) {
    m_SubMeshes = std::make_unique<rendering::WireframeObjects>(
        mesh, this, mesh.getColors(), m_lineWidth, mesh.getOffset());
  } else {
    m_SubMeshes = std::make_unique<rendering::WireframeObjects>(
        mesh, this, colors, m_lineWidth, mesh.getOffset());
  }
  update();
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

void QOpenGLMouseTracker::setBackgroundColor(QColor background) {
  m_backgroundColor = background;
}

QColor QOpenGLMouseTracker::getBackgroundColor() const {
  return m_backgroundColor;
}

void QOpenGLMouseTracker::clear() {
  m_SubMeshes.reset();
  update();
}
