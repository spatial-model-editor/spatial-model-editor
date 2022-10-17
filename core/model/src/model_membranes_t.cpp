#include "catch_wrapper.hpp"
#include "sme/model_membranes.hpp"
#include <QImage>

using namespace sme;

TEST_CASE("SBML membranes",
          "[core/model/membranes][core/model][core][model][membranes]") {
  SECTION("ModelMembranes") {
    SECTION("No image") {
      model::ModelMembranes m;
      REQUIRE(m.getIds().isEmpty());
      REQUIRE(m.getNames().isEmpty());
      REQUIRE(m.getMembranes().empty());
      REQUIRE(m.getIdColourPairs().empty());
    }
    SECTION("Image with 2 compartments, 1 membrane") {
      QImage img(3, 3, QImage::Format_RGB32);
      QRgb col0 = qRgb(0, 255, 0);
      QRgb col1 = qRgb(123, 123, 0);
      img.fill(col1);
      img.setPixel(1, 1, col0);
      img = img.convertToFormat(QImage::Format_Indexed8);
      std::vector<std::unique_ptr<geometry::Compartment>> compartments;
      compartments.push_back(
          std::make_unique<geometry::Compartment>("c0", img, col0));
      compartments.push_back(
          std::make_unique<geometry::Compartment>("c1", img, col1));
      QStringList names{"c0 name", "c1 name"};
      model::ModelMembranes ms;
      ms.updateCompartmentImages(img);
      REQUIRE(ms.getIds().isEmpty());
      REQUIRE(ms.getMembranes().empty());
      REQUIRE(ms.getNames().isEmpty());
      // set name is no-op if not found
      ms.setName("dontexist", "new name");
      REQUIRE(ms.getNames().isEmpty());
      ms.setHasUnsavedChanges(false);
      ms.updateCompartments(compartments);
      ms.setHasUnsavedChanges(true);
      REQUIRE(ms.getIds().size() == 1);
      REQUIRE(ms.getMembranes().size() == 1);
      REQUIRE(ms.getNames().isEmpty());
      ms.setHasUnsavedChanges(false);
      ms.updateCompartmentNames(names);
      ms.setHasUnsavedChanges(true);
      REQUIRE(ms.getIds().size() == 1);
      REQUIRE(ms.getIds()[0] == "c1_c0_membrane");
      REQUIRE(ms.getNames().size() == 1);
      REQUIRE(ms.getNames()[0] == "c1 name <-> c0 name");
      ms.setHasUnsavedChanges(false);
      // setting name to same value is a no-op
      ms.setName("c1_c0_membrane", "c1 name <-> c0 name");
      REQUIRE(ms.getHasUnsavedChanges() == false);
      // but setting a new name is an unsaved change
      ms.setName("c1_c0_membrane", "mem");
      REQUIRE(ms.getHasUnsavedChanges() == true);
      REQUIRE(ms.getIdColourPairs().size() == 1);
      REQUIRE(ms.getIdColourPairs()[0].first == "c1_c0_membrane");
      REQUIRE(ms.getIdColourPairs()[0].second ==
              std::pair<QRgb, QRgb>{col1, col0});
      REQUIRE(ms.getMembranes().size() == 1);
      const auto &m = ms.getMembranes()[0];
      REQUIRE(m.getId() == "c1_c0_membrane");
      REQUIRE(m.getCompartmentA()->getId() == "c1");
      REQUIRE(m.getCompartmentB()->getId() == "c0");
      REQUIRE(m.getImage().size() == img.size());
      REQUIRE(m.getIndexPairs().size() == 4);
    }
  }
}
