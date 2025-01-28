#include "catch_wrapper.hpp"
#include "model_test_utils.hpp"
#include "qmeshrenderer.hpp"
#include "qt_test_utils.hpp"
#include "sme/image_stack.hpp"

using namespace sme::test;

// set to e.g. 1000 to interactively inspect the rendering
constexpr int delay_ms{0};

TEST_CASE("QMeshRenderer",
          "[qmeshrenderer][gui/widgets/qmeshrenderer][gui/widgets][gui]") {
  QMeshRenderer meshRenderer;
  std::vector<int> mouseClicks;
  QObject::connect(&meshRenderer, &QMeshRenderer::mouseClicked,
                   [&mouseClicks](int i) { mouseClicks.push_back(i); });
  meshRenderer.show();
  meshRenderer.resize(200, 200);
  auto model = sme::test::getExampleModel(Mod::VerySimpleModel3D);
  auto &mesh = *model.getGeometry().getMesh3d();
  // highlight each compartment in turn
  for (std::size_t i = 0; i < mesh.getNumberOfCompartments(); ++i) {
    meshRenderer.setMesh(mesh, i);
    wait(delay_ms);
  }
  // change the compartment colours
  mesh.setColors({0xff00ff, 0x00ff00, 0x0000ff});
  meshRenderer.setMesh(mesh, 1);
  wait(delay_ms);
  // change the render mode
  meshRenderer.setRenderMode(QMeshRenderer::RenderMode::Wireframe);
  wait(delay_ms);
  meshRenderer.setRenderMode(QMeshRenderer::RenderMode::Solid);
  wait(delay_ms);
  // click outside a compartment
  REQUIRE(mouseClicks.size() == 0);
  sendMouseClick(&meshRenderer, {1, 1});
  REQUIRE(mouseClicks.size() == 0);
  // click on outer compartment with index 0
  sendMouseClick(&meshRenderer, {100, 100});
  REQUIRE(mouseClicks.size() == 1);
  REQUIRE(mouseClicks[0] == 0);
}
