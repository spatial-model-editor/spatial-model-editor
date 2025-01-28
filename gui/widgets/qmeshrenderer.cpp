#include "qmeshrenderer.hpp"
#include "sme/logger.hpp"
#include <vtkCamera.h>
#include <vtkPropPicker.h>
#include <vtkProperty.h>

QMeshRenderer::QMeshRenderer(QWidget *parent) : SmeVtkWidget(parent) {}

void QMeshRenderer::mousePressEvent(QMouseEvent *ev) {
  if (ev->buttons() == Qt::NoButton) {
    return;
  }
  auto *interactor = renderWindow->GetInteractor();
  const auto *pos = interactor->GetEventPosition();
  const auto &actors =
      renderMode == RenderMode::Solid ? solidActors : wireframeActors;
  vtkNew<vtkPropPicker> picker;
  if (picker->Pick(pos[0], pos[1], 0, renderer) != 0) {
    auto iter = std::ranges::find_if(
        actors, [&](const auto &actor) { return picker->GetActor() == actor; });
    if (iter != actors.end()) {
      auto compartmentIndex =
          static_cast<int>(std::distance(actors.begin(), iter));
      SPDLOG_TRACE("Click on compartmentIndex {}", compartmentIndex);
      emit mouseClicked(compartmentIndex);
    }
  }
}

void QMeshRenderer::clear() {
  renderer->RemoveAllViewProps();
  points->Reset();
  grids.clear();
  edges.clear();
  solidActors.clear();
  solidMappers.clear();
  wireframeActors.clear();
  wireframeMappers.clear();
}

void QMeshRenderer::setRenderMode(RenderMode mode) {
  renderMode = mode;
  for (auto &solidActor : solidActors) {
    solidActor->SetVisibility(renderMode == RenderMode::Solid);
  }
  for (auto &wireframeActor : wireframeActors) {
    wireframeActor->SetVisibility(renderMode == RenderMode::Wireframe);
  }
  renderWindow->Render();
}

void QMeshRenderer::setMesh(const sme::mesh::Mesh3d &mesh,
                            std::size_t compartmentIndex) {
  if (!isVisible() || compartmentIndex >= mesh.getNumberOfCompartments()) {
    return;
  }
  clear();
  for (const auto &v : mesh.getVertices()) {
    points->InsertNextPoint(v.p.x(), v.p.y(), v.z);
  }
  for (const auto &compartmentTetrahedronIndices :
       mesh.getTetrahedronIndices()) {
    auto &grid = grids.emplace_back();
    auto &edge = edges.emplace_back();
    auto &solidActor = solidActors.emplace_back();
    auto &solidMapper = solidMappers.emplace_back();
    auto &wireframeActor = wireframeActors.emplace_back();
    auto &wireframeMapper = wireframeMappers.emplace_back();
    for (const auto &t : compartmentTetrahedronIndices) {
      grid->InsertNextCell(VTK_TETRA, 4,
                           reinterpret_cast<const vtkIdType *>(t.data()));
    }
    grid->SetPoints(points);
    edge->SetInputData(grid);
    solidMapper->SetInputData(grid);
    solidActor->SetMapper(solidMapper);
    solidActor->GetProperty()->EdgeVisibilityOn();
    renderer->AddActor(solidActor);
    wireframeMapper->SetInputConnection(edge->GetOutputPort());
    wireframeActor->SetMapper(wireframeMapper);
    renderer->AddActor(wireframeActor);
  }
  setColours(mesh.getColors(), compartmentIndex);
  renderer->ResetCameraClippingRange();
  renderer->ResetCamera();
  setRenderMode(renderMode);
}

std::array<double, 3> makeEdgeColor(const QColor &color) {
  // lighter or darker edge color depending on lightness of color
  auto edge = color.lightnessF() > 0.3 ? color.darker(300) : color.lighter(300);
  if (edge.lightnessF() < 0.1) {
    // black remains black when lightened, so also add some grey if edge is dark
    edge = QColor(edge.red() + 50, edge.green() + 50, edge.blue() + 50);
  }
  return {edge.redF(), edge.greenF(), edge.blueF()};
}

void QMeshRenderer::setColours(const std::vector<QRgb> &colors,
                               std::size_t compartmentIndex) {
  for (std::size_t i = 0; i < solidActors.size(); ++i) {
    QColor color(colors[i]);
    std::array<double, 3> floatColor{color.redF(), color.greenF(),
                                     color.blueF()};
    solidActors[i]->GetProperty()->SetColor(floatColor.data());
    wireframeActors[i]->GetProperty()->SetColor(floatColor.data());
    solidActors[i]->GetProperty()->SetOpacity(i == compartmentIndex ? 1.0
                                                                    : 0.15);
    wireframeActors[i]->GetProperty()->SetOpacity(i == compartmentIndex ? 1.0
                                                                        : 0.05);
    auto edgeColor = makeEdgeColor(color);
    solidActors[i]->GetProperty()->SetEdgeColor(edgeColor.data());
  }
}
