#pragma once

#include "sme/image_stack.hpp"
#include "sme/mesh3d.hpp"
#include "smevtkwidget.hpp"
#include <QMouseEvent>
#include <vtkActor.h>
#include <vtkDataSetMapper.h>
#include <vtkExtractEdges.h>
#include <vtkNew.h>
#include <vtkPoints.h>
#include <vtkUnstructuredGrid.h>

class QMeshRenderer : public SmeVtkWidget {
  Q_OBJECT
public:
  enum class RenderMode {
    Solid,
    Wireframe,
  };

  explicit QMeshRenderer(QWidget *parent = nullptr);
  void setMesh(const sme::mesh::Mesh3d &mesh, std::size_t compartmentIndex);
  void setRenderMode(RenderMode mode);
  void clear();

signals:
  void mouseClicked(int compartmentIndex);

protected:
  void mousePressEvent(QMouseEvent *ev) override;

private:
  void setColours(const std::vector<QRgb> &colors,
                  std::size_t compartmentIndex);
  RenderMode renderMode{RenderMode::Solid};
  vtkNew<vtkPoints> points;
  std::vector<vtkNew<vtkUnstructuredGrid>> grids;
  std::vector<vtkNew<vtkExtractEdges>> edges;
  std::vector<vtkNew<vtkDataSetMapper>> solidMappers;
  std::vector<vtkNew<vtkDataSetMapper>> wireframeMappers;
  std::vector<vtkNew<vtkActor>> solidActors;
  std::vector<vtkNew<vtkActor>> wireframeActors;
};
