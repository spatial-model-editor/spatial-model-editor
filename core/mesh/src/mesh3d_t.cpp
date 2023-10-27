//
// Created by acaramizaru on 6/26/23.
//

#include <sme/mesh3d.hpp>

#include <catch_wrapper.hpp>

TEST_CASE("Mesh3d", "[core/mesh/mesh3d][core/mesh][core][mesh]") {

  SECTION("CGAL mesh3d from image, with sme input") {

    auto model = sme::model::Model();
    model.importFile("/home/hcaramizaru/Dropbox/Work-shared/Heidelberg-IWR/"
                     "spatial-model-editor-resources/cell-3d-model.sme");
    REQUIRE(model.getIsValid());

    //    auto img_3 = CGAL::Image_3();
    //    img_3.read("/home/acaramizaru/Dropbox/Work-shared/Heidelberg-IWR/spatial-model-editor-resources/cell-3d-model.sme");
    //    REQUIRE(img_3.is_valid());

    sme::mesh::Mesh3d testMesh(model);

    REQUIRE(testMesh.getNumberOfSubMeshes() == 5);
  }
}
