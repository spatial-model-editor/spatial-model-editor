//
// Created by acaramizaru on 7/25/23.
//

#include "qopenglmousetracker.hpp"
#include <QMenu>

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
      m_camera(std::make_shared<rendering::Camera>(
          cameraFOV, static_cast<float>(size().width()),
          static_cast<float>(size().height()), cameraNearZ, cameraFarZ)),
      m_frameRate(frameRate),
      m_backgroundColor(QWidget::palette().color(QWidget::backgroundRole())),
      m_lastColour(QWidget::palette().color(QWidget::backgroundRole()).rgb()) {

  // A default camera is added.
  m_sceneGraph->add(m_camera);

  m_labelPlaneSelected = new QLabel(this);
  m_labelPlaneSelected->setText("Normal Mode");

  //  m_labelPlaneSelected->show();
}

std::weak_ptr<rendering::ClippingPlane> QOpenGLMouseTracker::BuildClippingPlane(
    const QVector3D &normal, const QVector3D &point, bool active,
    bool localFrameCoord, const std::shared_ptr<rendering::Node> &parent) {

  // TODO: Implement global frame use case.
  assert(localFrameCoord == true);

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

  if (parent == nullptr) {
    m_sceneGraph->add(clippingPlane, localFrameCoord);
  } else {
    parent->add(clippingPlane, localFrameCoord);
  }

  return clippingPlane;
}

void QOpenGLMouseTracker::DestroyClippingPlane(
    std::weak_ptr<rendering::ClippingPlane> &clippingPlane) {

  auto clippingPlane_locked = clippingPlane.lock();
  if (!clippingPlane_locked) {
    SPDLOG_DEBUG("Weak ptr already invalid.");
    return;
  }

  auto it = m_clippingPlanes.find(clippingPlane_locked);

  if (it == m_clippingPlanes.end())
    return;

  m_clippingPlanesPool.insert(*it);
  m_clippingPlanes.erase(it);

  clippingPlane_locked->remove();

  clippingPlane.reset();

  update();
}

std::vector<std::weak_ptr<rendering::Node>>
QOpenGLMouseTracker::GetDefaultClippingPlanes() {

  std::vector<std::weak_ptr<rendering::Node>> defaultPlanes;

  auto planeX_locked = planeX.lock();
  if (!planeX_locked && m_SubMeshes) {
    planeX = BuildClippingPlane(QVector3D(1, 0, 0).normalized(),
                                QVector3D(-22, 0, 0), true, true, m_SubMeshes);
    planeX.lock()->name = "Plane X";
    defaultPlanes.push_back(planeX);
  }

  auto planeY_locked = planeY.lock();
  if (!planeY_locked && m_SubMeshes) {
    planeY = BuildClippingPlane(QVector3D(0, 1, 0).normalized(),
                                QVector3D(0, -22, 0), true, true, m_SubMeshes);
    planeY.lock()->name = "Plane Y";
    defaultPlanes.push_back(planeY);
  }

  auto planeZ_locked = planeZ.lock();
  if (!planeZ_locked && m_SubMeshes) {
    planeZ = BuildClippingPlane(QVector3D(0, 0, 1).normalized(),
                                QVector3D(0, 0, -22), true, true, m_SubMeshes);
    planeZ.lock()->name = "Plane Z";
    defaultPlanes.push_back(planeZ);
  }

  auto planeCamera_locked = planeCamera.lock();
  if (!planeCamera_locked && m_camera) {
    planeCamera = BuildClippingPlane(QVector3D(0, 0, 1).normalized(),
                                     QVector3D(0, 0, 30), true, true, m_camera);
    planeCamera.lock()->name = "Camera Plane";
    defaultPlanes.push_back(planeCamera);
  }

  return std::move(defaultPlanes);
}

std::vector<std::weak_ptr<rendering::Node>>
QOpenGLMouseTracker::GetAllClippingPlanes(
    const std::shared_ptr<rendering::Node> &parent) {

  std::vector<std::weak_ptr<rendering::Node>> listOfPlanes;

  if (parent == nullptr) {
    m_sceneGraph->GetAllNodesOfType(typeid(rendering::ClippingPlane),
                                    listOfPlanes);
  } else {
    parent->GetAllNodesOfType(typeid(rendering::ClippingPlane), listOfPlanes);
  }

  auto defaultPlanes = GetDefaultClippingPlanes();

  listOfPlanes.insert(listOfPlanes.end(), defaultPlanes.begin(),
                      defaultPlanes.end());

  return std::move(listOfPlanes);
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

  //  std::string ext =
  //      QString::fromLatin1(
  //          (const char *)context()->functions()->glGetString(GL_EXTENSIONS))
  //          .replace(' ', "\n\t")
  //          .toStdString();
  //  CheckOpenGLError("glGetString(GL_EXTENSIONS)");
  //
  //  std::string vendor(
  //      (const char *)context()->functions()->glGetString(GL_VENDOR));
  //  CheckOpenGLError("glGetString(GL_VENDOR)");
  //  std::string renderer(
  //      (const char *)context()->functions()->glGetString(GL_RENDERER));
  //  CheckOpenGLError("glGetString(GL_RENDERER)");
  //  std::string gl_version(
  //      (const char *)context()->functions()->glGetString(GL_VERSION));
  //  CheckOpenGLError("glGetString(GL_VERSION)");
  //
  //  SPDLOG_INFO("OpenGL: " + vendor + std::string(" ") + renderer +
  //              std::string(" ") + gl_version + std::string(" ") +
  //              std::string("\n\n\t") + ext + std::string("\n"));

  m_mainProgram = std::make_unique<rendering::ShaderProgram>(
      rendering::shader::colorAsUniform::text_vertex_color_as_uniform,
      rendering::shader::default_::text_geometry,
      rendering::shader::default_::text_fragment);
}

void QOpenGLMouseTracker::updateScene() const {

  if (m_SubMeshes) {
    m_SubMeshes->setBackground(m_backgroundColor);
  }

  m_sceneGraph->updateSceneGraph(1 / m_frameRate);
}

void QOpenGLMouseTracker::drawScene() {

  m_sceneGraph->drawSceneGraph(*m_mainProgram);
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

  updateScene();

  drawScene();

  if (!m_selectedPlane.expired()) {
    auto selectedPlaneShared = m_selectedPlane.lock();
    m_labelPlaneSelected->setText(
        (" " + selectedPlaneShared->name + " ").c_str());
  } else {
    m_labelPlaneSelected->setText("Normal Mode");
  }

  QOpenGLFramebufferObject fboPicking(size());
  fboPicking.bind();

  context()->functions()->glViewport(0, 0, size().width(), size().height());

  if (m_SubMeshes) {
    m_SubMeshes->SetAllThickness(m_lineSelectPrecision);
  }

  drawScene();

  if (m_SubMeshes) {
    m_SubMeshes->ResetAllToDefaultThickness();
  }

  m_offscreenPickingImage = fboPicking.toImage();

  QOpenGLFramebufferObject::bindDefault();
}

void QOpenGLMouseTracker::resizeGL(int w, int h) {
  m_camera->SetFrustum(m_camera->getFOV(), static_cast<float>(w),
                       static_cast<float>(h), m_camera->getNear(),
                       m_camera->getFar());
  this->update();
}

void QOpenGLMouseTracker::SetCameraFrustum(GLfloat FOV, GLfloat width,
                                           GLfloat height, GLfloat nearZ,
                                           GLfloat farZ) const {
  m_camera->SetFrustum(FOV, width, height, nearZ, farZ);
}

void QOpenGLMouseTracker::keyReleaseEvent(QKeyEvent *event) {
  if (event->key() == Qt::Key_Escape) {
    m_selectedPlane.reset();
    m_labelPlaneSelected->setText("Normal Mode");
  }
}

void QOpenGLMouseTracker::mousePressEvent(QMouseEvent *event) {

  if (m_SubMeshes == nullptr) {
    return;
  }
  m_xAtPress = static_cast<int>(event->position().x());
  m_yAtPress = static_cast<int>(event->position().y());

  m_xAtPress = std::clamp(m_xAtPress, 0, m_offscreenPickingImage.width() - 1);
  m_yAtPress = std::clamp(m_yAtPress, 0, m_offscreenPickingImage.height() - 1);

  auto m_selectedPlane_locked = m_selectedPlane.lock();
  if (!m_selectedPlane_locked) {
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
  }

  if ((event->button() == Qt::RightButton) &&
      (m_offscreenPickingImage.width() >
       static_cast<int>(event->position().x())) &&
      (static_cast<int>(event->position().x()) > 0) &&
      (m_offscreenPickingImage.height() >
       static_cast<int>(event->position().y())) &&
      (static_cast<int>(event->position().y()) > 0)) {

    auto planes = GetAllClippingPlanes();

    std::shared_ptr<QMenu> menu = std::make_shared<QMenu>(this);
    menu->setTitle(tr("Planes"));

    for (auto &var : planes) {
      auto elem =
          std::dynamic_pointer_cast<rendering::ClippingPlane>(var.lock());

      if (elem->getStatus() == false) {
        continue;
      }

      QAction *item = new QAction(elem->name.c_str(), this);

      const auto connection_result =
          connect(item, &QAction::triggered, [=, this]() {
            for (const auto &var : planes) {
              auto elem = std::dynamic_pointer_cast<rendering::ClippingPlane>(
                  var.lock());

              if (elem->name == item->text().toStdString()) {
                m_selectedPlane = elem;
                break;
              }
            }
          });

      menu->addAction(item);
    }

    menu->exec(QCursor::pos());
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

  auto m_selectedPlane_locked = m_selectedPlane.lock();
  if (!m_selectedPlane_locked) {

    auto forwardVector = m_camera->GetForwardVector();
    auto position = m_camera->getPos();

    m_camera->setPos(position + forwardVector * static_cast<float>(Degrees) *
                                    (1 / m_frameRate));

    emit mouseWheelEvent(event);

  } else {
    m_selectedPlane_locked->TranslateAlongsideNormal(
        static_cast<float>(Degrees) * (2 / m_frameRate));

    auto [position, forwardVector] = m_selectedPlane_locked->GetClipPlane();

    SPDLOG_DEBUG("plane position :" + std::to_string(position.x()) + " " +
                 std::to_string(position.y()) + " " +
                 std::to_string(position.z()));
    SPDLOG_DEBUG("plane Degrees :" + std::to_string(Degrees));
  }

  update();
}

void QOpenGLMouseTracker::SetCameraPosition(float x, float y, float z) const {
  m_camera->setPos(x, y, z);
}

void QOpenGLMouseTracker::SetCameraOrientation(float x, float y,
                                               float z) const {
  m_camera->setRot(x, y, z);
}

QVector3D QOpenGLMouseTracker::GetCameraPosition() const {
  return m_camera->getPos();
}

QVector3D QOpenGLMouseTracker::GetCameraOrientation() const {
  return m_camera->getRot();
}

void QOpenGLMouseTracker::SetSubMeshesOrientation(float x, float y,
                                                  float z) const {
  m_SubMeshes->setRot(x, y, z);
}

void QOpenGLMouseTracker::SetSubMeshesPosition(float x, float y,
                                               float z) const {
  m_SubMeshes->setPos(x, y, z);
}

QVector3D QOpenGLMouseTracker::GetSubMeshesOrientation() const {
  return m_SubMeshes->getRot();
}

QVector3D QOpenGLMouseTracker::GetSubMeshesPosition() const {
  return m_SubMeshes->getPos();
}

std::shared_ptr<rendering::Node>
QOpenGLMouseTracker::SetSubMeshes(const sme::mesh::Mesh3d &mesh,
                                  const std::vector<QColor> &colors) {

  if (m_SubMeshes) {

    std::vector<std::weak_ptr<rendering::Node>> planes;
    m_SubMeshes->GetAllNodesOfType(typeid(rendering::ClippingPlane), planes);

    for (auto &var : planes) {
      auto elem =
          std::dynamic_pointer_cast<rendering::ClippingPlane>(var.lock());
      std::weak_ptr<rendering::ClippingPlane> weak_eleme = elem;
      DestroyClippingPlane(weak_eleme);
    }

    m_SubMeshes->remove();
    m_SubMeshes.reset();
  }

  if (colors.empty()) {
    m_SubMeshes = std::make_shared<rendering::WireframeObjects>(
        mesh, this, mesh.getColors(), m_lineWidth, mesh.getOffset());
  } else {
    m_SubMeshes = std::make_shared<rendering::WireframeObjects>(
        mesh, this, colors, m_lineWidth, mesh.getOffset());
  }

  m_sceneGraph->add(m_SubMeshes);

  update();

  return m_SubMeshes;
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
                                               bool visibility) const {
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

  m_sceneGraph->children.clear();

  update();
}
