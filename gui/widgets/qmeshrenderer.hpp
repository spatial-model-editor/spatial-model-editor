#pragma once

#include "sme/image_stack.hpp"
#include "sme/mesh3d.hpp"
#include "smevtkwidget.hpp"
#include <QMouseEvent>
#include <vtkActor.h>
#include <vtkCellArray.h>
#include <vtkClipDataSet.h>
#include <vtkDataSetMapper.h>
#include <vtkExtractEdges.h>
#include <vtkNew.h>
#include <vtkPlane.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkUnstructuredGrid.h>

class QVoxelRenderer;

class QMeshRenderer : public SmeVtkWidget {
  Q_OBJECT
public:
  enum class RenderMode {
    Solid,
    Wireframe,
  };

  explicit QMeshRenderer(QWidget *parent = nullptr);
  void setMesh(const sme::mesh::Mesh3d &mesh, std::size_t compartmentIndex,
               bool resetCamera = true);
  void syncClippingPlane(QVoxelRenderer *qVoxelRenderer);
  void setCompartmentIndex(std::size_t compartmentIndex);
  void setMembraneIndex(std::size_t membraneIndex);
  void setColors(std::vector<QRgb> newColors);
  void setRenderMode(RenderMode mode);
  void clear();

signals:
  void mouseClicked(int compartmentIndex);

protected:
  void mousePressEvent(QMouseEvent *ev) override;
  void mouseReleaseEvent(QMouseEvent *ev) override;

private:
  void updateAndRender();
  vtkPlane *clippingPlane = nullptr;
  QPoint lastMouseClickPos{};
  std::size_t currentCompartmentIndex{0};
  RenderMode renderMode{RenderMode::Solid};
  vtkNew<vtkPoints> points{};
  std::vector<vtkNew<vtkUnstructuredGrid>> grids{};
  std::vector<vtkNew<vtkCellArray>> triangles{};
  std::vector<vtkNew<vtkPolyData>> membranes{};
  std::vector<vtkNew<vtkClipDataSet>> clippedMembranes{};
  std::vector<vtkNew<vtkClipDataSet>> clippedTetrahedrons{};
  std::vector<vtkNew<vtkExtractEdges>> edges{};
  std::vector<vtkNew<vtkClipDataSet>> clippedEdges{};
  std::vector<vtkNew<vtkDataSetMapper>> solidMappers{};
  std::vector<vtkNew<vtkDataSetMapper>> wireframeMappers{};
  std::vector<vtkNew<vtkActor>> solidActors{};
  std::vector<vtkNew<vtkActor>> wireframeActors{};
};
