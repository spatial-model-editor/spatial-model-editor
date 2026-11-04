#include "qmeshrenderer.hpp"
#include "qvoxelrenderer.hpp"
#include "sme/logger.hpp"
#include "sme/utils.hpp"
#include <vtkCamera.h>
#include <vtkCellArray.h>
#include <vtkClipDataSet.h>
#include <vtkDataSetMapper.h>
#include <vtkExtractEdges.h>
#include <vtkIdTypeArray.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkPropPicker.h>
#include <vtkProperty.h>
#include <vtkTableBasedClipDataSet.h>
#include <vtkUnstructuredGrid.h>

static std::array<double, 3> makeEdgeColor(const QColor &color) {
  // lighter or darker edge color depending on lightness of color
  auto edge = color.lightnessF() > 0.3 ? color.darker(300) : color.lighter(300);
  if (edge.lightnessF() < 0.1) {
    // black remains black when lightened, so also add some grey if edge is dark
    edge = QColor(edge.red() + 50, edge.green() + 50, edge.blue() + 50);
  }
  return {edge.redF(), edge.greenF(), edge.blueF()};
}

template <std::size_t CellSize>
static vtkNew<vtkCellArray>
makeCellArray(const std::vector<std::array<std::size_t, CellSize>> &indices) {
  vtkNew<vtkIdTypeArray> connectivity;
  connectivity->SetNumberOfValues(static_cast<vtkIdType>(indices.size()) *
                                  static_cast<vtkIdType>(CellSize));
  auto *output = connectivity->GetPointer(0);
  for (const auto &cell : indices) {
    for (auto id : cell) {
      *output++ = static_cast<vtkIdType>(id);
    }
  }
  connectivity->Modified();
  vtkNew<vtkCellArray> cells;
  // Fixed-size cells use implicit offsets. SetData retains the connectivity
  // array, so the pipeline owns the storage after this helper returns.
  cells->SetData(static_cast<vtkIdType>(CellSize), connectivity);
  return cells;
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
  numberOfCompartments = mesh.getNumberOfCompartments();
  vtkNew<vtkPoints> points;
  points->SetNumberOfPoints(static_cast<vtkIdType>(mesh.getVertices().size()));
  for (vtkIdType i = 0; const auto &v : mesh.getVertices()) {
    points->SetPoint(i, static_cast<float>(v.p.x()),
                     static_cast<float>(v.p.y()), static_cast<float>(v.z));
    ++i;
  }
  points->Modified();
  for (const auto &compartmentTetrahedronIndices :
       mesh.getTetrahedronIndices()) {
    vtkNew<vtkUnstructuredGrid> grid;
    vtkNew<vtkExtractEdges> edge;
    auto &solidActor = solidActors.emplace_back();
    vtkNew<vtkDataSetMapper> solidMapper;
    auto &wireframeActor = wireframeActors.emplace_back();
    vtkNew<vtkDataSetMapper> wireframeMapper;
    auto tetrahedra = makeCellArray(compartmentTetrahedronIndices);
    grid->SetCells(VTK_TETRA, tetrahedra);
    grid->SetPoints(points);
    edge->SetInputData(grid);
    if (clippingPlane != nullptr) {
      vtkNew<vtkTableBasedClipDataSet> clippedTetrahedron;
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
      vtkNew<vtkClipDataSet> clippedEdge;
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
    auto triangle = makeCellArray(membraneTriangleIndices);
    vtkNew<vtkPolyData> membrane;
    vtkNew<vtkExtractEdges> edge;
    auto &solidActor = solidActors.emplace_back();
    vtkNew<vtkDataSetMapper> solidMapper;
    auto &wireframeActor = wireframeActors.emplace_back();
    vtkNew<vtkDataSetMapper> wireframeMapper;
    membrane->SetPoints(points);
    membrane->SetPolys(triangle);
    if (clippingPlane != nullptr) {
      vtkNew<vtkClipDataSet> clippedMembrane;
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
      vtkNew<vtkClipDataSet> clippedEdge;
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
  setCompartmentIndex(numberOfCompartments + membraneIndex);
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
  numberOfCompartments = 0;
  renderMode = RenderMode::Solid;
  renderer->RemoveAllViewProps();
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
      Q_EMIT mouseClicked(compartmentIndex);
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
  std::size_t iMin =
      currentCompartmentIndex < numberOfCompartments ? 0 : numberOfCompartments;
  std::size_t iMax = currentCompartmentIndex < numberOfCompartments
                         ? numberOfCompartments
                         : solidActors.size();
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
