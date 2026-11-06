#include "smevtkwidget.hpp"
#include "sme/logger.hpp"
#include <QShowEvent>
#include <vtkCamera.h>

SmeVtkWidget::SmeVtkWidget(QWidget *parent) : QVTKOpenGLNativeWidget(parent) {
  // Mouse tracking already delivers moves without pressed buttons. HoverMove
  // would forward the same movement to VTK again, rendering twice during drags.
  setAttribute(Qt::WA_Hover, false);
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
  renderTimer.setSingleShot(true);
  connect(&renderTimer, &QTimer::timeout, this, [this]() {
    if (renderPending && isVisible()) {
      vtkRender();
    }
  });
  renderObserverTag = renderWindow->AddObserver(vtkCommand::EndEvent, this,
                                                &SmeVtkWidget::rendered);
}

SmeVtkWidget::~SmeVtkWidget() {
  renderTimer.stop();
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
  scheduleRender();
}

void SmeVtkWidget::vtkRender() {
  if (!isVisible()) {
    scheduleRender();
    return;
  }
  renderWindow->Render();
}

void SmeVtkWidget::showEvent(QShowEvent *event) {
  QVTKOpenGLNativeWidget::showEvent(event);
  if (renderPending) {
    scheduleRender();
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
                                          &SmeVtkWidget::scheduleRender);
}

void SmeVtkWidget::scheduleRender() {
  renderPending = true;
  if (isVisible() && !renderTimer.isActive()) {
    // Coalesce queued changes and render their final state once Qt processes
    // the timer event. Explicit renders of visible views remain immediate.
    renderTimer.start(0);
  }
}

void SmeVtkWidget::rendered() {
  // The interactor, or an explicit render, may already have refreshed the view
  // before the queued callback runs.
  renderTimer.stop();
  renderPending = false;
}
