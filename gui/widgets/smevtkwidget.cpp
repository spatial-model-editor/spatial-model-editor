#include "smevtkwidget.hpp"
#include "sme/logger.hpp"
#include <QShowEvent>
#include <vtkCamera.h>

SmeVtkWidget::SmeVtkWidget(QWidget *parent) : QVTKOpenGLNativeWidget(parent) {
  setRenderWindow(renderWindow);
  renderWindow->AddRenderer(renderer);
  axesWidget->SetOrientationMarker(axesActor);
  axesWidget->SetInteractor(renderWindow->GetInteractor());
  // set relative size of axes widget
  axesWidget->SetViewport(0.0, 0.0, 0.3, 0.3);
  axesWidget->SetEnabled(1);
  axesWidget->InteractiveOn();
  // inherit widget background color
  [this]() {
    float r{0};
    float g{0};
    float b{0};
    palette().color(QWidget::backgroundRole()).getRgbF(&r, &g, &b);
    renderer->SetBackground(r, g, b);
  }();
  renderer->GetActiveCamera()->Azimuth(45);
  renderer->GetActiveCamera()->Elevation(30);
  cameraRenderTimer.setSingleShot(true);
  connect(&cameraRenderTimer, &QTimer::timeout, this, [this]() {
    if (cameraRenderPending && isVisible()) {
      vtkRender();
    }
  });
  renderObserverTag = renderWindow->AddObserver(vtkCommand::EndEvent, this,
                                                &SmeVtkWidget::cameraRendered);
}

SmeVtkWidget::~SmeVtkWidget() {
  cameraRenderTimer.stop();
  if (observedCamera != nullptr) {
    observedCamera->RemoveObserver(cameraObserverTag);
  }
  renderWindow->RemoveObserver(renderObserverTag);
}

void SmeVtkWidget::syncCamera(SmeVtkWidget *smeVtkWidget) {
  if (renderer->GetActiveCamera() ==
      smeVtkWidget->renderer->GetActiveCamera()) {
    SPDLOG_TRACE("Camera is already synced with this widget");
    return;
  }
  SPDLOG_TRACE("Syncing camera with supplied SmeVtkWidget");
  renderer->SetActiveCamera(smeVtkWidget->renderer->GetActiveCamera());
  observeCamera();
  smeVtkWidget->observeCamera();
  scheduleCameraRender();
}

void SmeVtkWidget::vtkRender() { renderWindow->Render(); }

void SmeVtkWidget::showEvent(QShowEvent *event) {
  QVTKOpenGLNativeWidget::showEvent(event);
  if (cameraRenderPending) {
    scheduleCameraRender();
  }
}

void SmeVtkWidget::observeCamera() {
  auto *camera = renderer->GetActiveCamera();
  if (observedCamera == camera) {
    return;
  }
  if (observedCamera != nullptr) {
    observedCamera->RemoveObserver(cameraObserverTag);
  }
  observedCamera = camera;
  cameraObserverTag = camera->AddObserver(vtkCommand::ModifiedEvent, this,
                                          &SmeVtkWidget::scheduleCameraRender);
}

void SmeVtkWidget::scheduleCameraRender() {
  cameraRenderPending = true;
  if (isVisible() && !cameraRenderTimer.isActive()) {
    // Several camera setters can run during one interaction. Render their
    // final result once Qt processes the queued timer event.
    cameraRenderTimer.start(0);
  }
}

void SmeVtkWidget::cameraRendered() {
  // The active view's interactor, or an explicit data update, may already have
  // rendered the camera before the queued callback runs.
  cameraRenderTimer.stop();
  cameraRenderPending = false;
}
