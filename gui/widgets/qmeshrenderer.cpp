#include "qmeshrenderer.hpp"
#include "qvoxelrenderer.hpp"
#include "sme/logger.hpp"
#include "sme/utils.hpp"
#include <vtkCamera.h>
#include <vtkPropPicker.h>
#include <vtkProperty.h>

static std::array<double, 3> makeEdgeColor(const QColor &color) {
  // lighter or darker edge color depending on lightness of color
  auto edge = color.lightnessF() > 0.3 ? color.darker(300) : color.lighter(300);
  if (edge.lightnessF() < 0.1) {
    // black remains black when lightened, so also add some grey if edge is dark
    edge = QColor(edge.red() + 50, edge.green() + 50, edge.blue() + 50);
  }
  return {edge.redF(), edge.greenF(), edge.blueF()};
}

QMeshRenderer::QMeshRenderer(QWidget *parent) : SmeVtkWidget(parent) {}

void QMeshRenderer::setMesh(const sme::mesh::Mesh3d &mesh,
                            std::size_t compartmentIndex, bool resetCamera) {
  if (!isVisible()) {
    return;
  }
  if (compartmentIndex >=
      mesh.getNumberOfCompartments() + mesh.getNumberOfMembranes()) {
    compartmentIndex = 0;
  }
  clear();
  points->SetNumberOfPoints(static_cast<vtkIdType>(mesh.getVertices().size()));
  for (vtkIdType i = 0; const auto &v : mesh.getVertices()) {
    points->SetPoint(i, static_cast<float>(v.p.x()),
                     static_cast<float>(v.p.y()), static_cast<float>(v.z));
    ++i;
  }
  points->Modified();
  for (const auto &compartmentTetrahedronIndices :
       mesh.getTetrahedronIndices()) {
    auto &grid = grids.emplace_back();
    auto &clippedTetrahedron = clippedTetrahedrons.emplace_back();
    auto &edge = edges.emplace_back();
    auto &clippedEdge = clippedEdges.emplace_back();
    auto &solidActor = solidActors.emplace_back();
    auto &solidMapper = solidMappers.emplace_back();
    auto &wireframeActor = wireframeActors.emplace_back();
    auto &wireframeMapper = wireframeMappers.emplace_back();
    grid->AllocateExact(
        static_cast<vtkIdType>(compartmentTetrahedronIndices.size()), 4);
    for (const auto &t : compartmentTetrahedronIndices) {
      grid->InsertNextCell(VTK_TETRA, 4,
                           reinterpret_cast<const vtkIdType *>(t.data()));
    }
    grid->SetPoints(points);
    edge->SetInputData(grid);
    if (clippingPlane != nullptr) {
      clippedTetrahedron->SetClipFunction(clippingPlane);
      clippedTetrahedron->SetInputData(grid);
      solidMapper->SetInputConnection(clippedTetrahedron->GetOutputPort());
    } else {
      solidMapper->SetInputData(grid);
    }
    solidActor->SetMapper(solidMapper);
    solidActor->GetProperty()->EdgeVisibilityOn();
    renderer->AddActor(solidActor);
    if (clippingPlane != nullptr) {
      clippedEdge->SetClipFunction(clippingPlane);
      clippedEdge->SetInputConnection(edge->GetOutputPort());
      wireframeMapper->SetInputConnection(clippedEdge->GetOutputPort());
    } else {
      wireframeMapper->SetInputConnection(edge->GetOutputPort());
    }
    wireframeActor->SetMapper(wireframeMapper);
    renderer->AddActor(wireframeActor);
  }
  for (const auto &membraneTriangleIndices :
       mesh.getMembraneTriangleIndices()) {
    auto &triangle = triangles.emplace_back();
    auto &membrane = membranes.emplace_back();
    auto &clippedMembrane = clippedMembranes.emplace_back();
    auto &edge = edges.emplace_back();
    auto &clippedEdge = clippedEdges.emplace_back();
    auto &solidActor = solidActors.emplace_back();
    auto &solidMapper = solidMappers.emplace_back();
    auto &wireframeActor = wireframeActors.emplace_back();
    auto &wireframeMapper = wireframeMappers.emplace_back();
    triangle->AllocateExact(
        static_cast<vtkIdType>(membraneTriangleIndices.size()), 3);
    for (const auto &triangleIndex : membraneTriangleIndices) {
      triangle->InsertNextCell(
          3, reinterpret_cast<const vtkIdType *>(triangleIndex.data()));
    }
    membrane->SetPoints(points);
    membrane->SetPolys(triangle);
    if (clippingPlane != nullptr) {
      clippedMembrane->SetClipFunction(clippingPlane);
      clippedMembrane->SetInputData(membrane);
      solidMapper->SetInputConnection(clippedMembrane->GetOutputPort());
    } else {
      solidMapper->SetInputData(membrane);
    }
    solidActor->SetMapper(solidMapper);
    solidActor->GetProperty()->EdgeVisibilityOn();
    renderer->AddActor(solidActor);
    edge->SetInputData(membrane);
    if (clippingPlane != nullptr) {
      clippedEdge->SetClipFunction(clippingPlane);
      clippedEdge->SetInputConnection(edge->GetOutputPort());
      wireframeMapper->SetInputConnection(clippedEdge->GetOutputPort());
    } else {
      wireframeMapper->SetInputConnection(edge->GetOutputPort());
    }
    wireframeActor->SetMapper(wireframeMapper);
    renderer->AddActor(wireframeActor);
  }
  if (resetCamera) {
    renderer->ResetCameraClippingRange();
    renderer->ResetCamera();
  }
  currentCompartmentIndex = compartmentIndex;
  setColors(mesh.getColors());
}

void QMeshRenderer::syncClippingPlane(QVoxelRenderer *qVoxelRenderer) {
  clippingPlane = qVoxelRenderer->getClippingPlane();
  qVoxelRenderer->renderOnClippingPaneChange(this);
}

void QMeshRenderer::setCompartmentIndex(std::size_t compartmentIndex) {
  currentCompartmentIndex = compartmentIndex;
  updateAndRender();
}

void QMeshRenderer::setMembraneIndex(std::size_t membraneIndex) {
  setCompartmentIndex(grids.size() + membraneIndex);
}

void QMeshRenderer::setColors(std::vector<QRgb> colors) {
  for (std::size_t i = 0; i < colors.size(); ++i) {
    QColor color(colors[i]);
    std::array<double, 3> floatColor{color.redF(), color.greenF(),
                                     color.blueF()};
    solidActors[i]->GetProperty()->SetColor(floatColor.data());
    wireframeActors[i]->GetProperty()->SetColor(floatColor.data());
    auto edgeColor = makeEdgeColor(color);
    solidActors[i]->GetProperty()->SetEdgeColor(edgeColor.data());
  }
  updateAndRender();
}

void QMeshRenderer::setRenderMode(RenderMode mode) {
  renderMode = mode;
  updateAndRender();
}

void QMeshRenderer::clear() {
  currentCompartmentIndex = 0;
  renderMode = RenderMode::Solid;
  renderer->RemoveAllViewProps();
  points->Reset();
  grids.clear();
  clippedTetrahedrons.clear();
  triangles.clear();
  membranes.clear();
  clippedMembranes.clear();
  edges.clear();
  clippedEdges.clear();
  solidMappers.clear();
  wireframeMappers.clear();
  solidActors.clear();
  wireframeActors.clear();
}

void QMeshRenderer::mousePressEvent(QMouseEvent *ev) {
  if (ev->buttons() == Qt::NoButton) {
    return;
  }
  lastMouseClickPos = ev->pos();
}

void QMeshRenderer::mouseReleaseEvent(QMouseEvent *ev) {
  if (ev->pos() != lastMouseClickPos) {
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

void QMeshRenderer::updateAndRender() {
  for (auto &solidActor : solidActors) {
    solidActor->SetVisibility(false);
  }
  for (auto &wireframeActor : wireframeActors) {
    wireframeActor->SetVisibility(false);
  }
  std::size_t iMin = currentCompartmentIndex < grids.size() ? 0 : grids.size();
  std::size_t iMax = currentCompartmentIndex < grids.size()
                         ? grids.size()
                         : grids.size() + triangles.size();
  for (std::size_t i = iMin; i < iMax; ++i) {
    if (renderMode == RenderMode::Solid) {
      solidActors[i]->SetVisibility(true);
      solidActors[i]->GetProperty()->SetOpacity(
          i == currentCompartmentIndex ? 1.0 : 0.15);
    } else if (renderMode == RenderMode::Wireframe) {
      wireframeActors[i]->SetVisibility(true);
      wireframeActors[i]->GetProperty()->SetOpacity(
          i == currentCompartmentIndex ? 1.0 : 0.05);
    }
  }
  vtkRender();
}
