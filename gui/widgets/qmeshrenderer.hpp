#pragma once

#include "sme/image_stack.hpp"
#include "sme/mesh3d.hpp"
#include "smevtkwidget.hpp"
#include <QMouseEvent>
#include <vtkActor.h>
#include <vtkNew.h>
#include <vtkPlane.h>

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

Q_SIGNALS:
  void mouseClicked(int compartmentIndex);

protected:
  void mousePressEvent(QMouseEvent *ev) override;
  void mouseReleaseEvent(QMouseEvent *ev) override;

private:
  void updateAndRender();
  vtkPlane *clippingPlane = nullptr;
  QPoint lastMouseClickPos{};
  std::size_t currentCompartmentIndex{0};
  std::size_t numberOfCompartments{0};
  RenderMode renderMode{RenderMode::Solid};
  std::vector<vtkNew<vtkActor>> solidActors{};
  std::vector<vtkNew<vtkActor>> wireframeActors{};
};
