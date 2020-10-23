#include "catch_wrapper.hpp"
#include "id.hpp"
#include "sbml_utils.hpp"
#include <QFile>
#include <QStringList>
#include <sbml/SBMLTypes.h>
#include <sbml/extension/SBMLDocumentPlugin.h>
#include <sbml/packages/spatial/common/SpatialExtensionTypes.h>
#include <sbml/packages/spatial/extension/SpatialExtension.h>

SCENARIO("Model id", "[core/model/id][core/model][core][model][id]") {
  GIVEN("ABtoC model") {
    QFile f(":/models/ABtoC.xml");
    f.open(QIODevice::ReadOnly);
    std::unique_ptr<libsbml::SBMLDocument> doc(
        libsbml::readSBMLFromString(f.readAll().toStdString().c_str()));
    auto *model = doc->getModel();
    auto *geom = getOrCreateGeometry(model);

    // true if sId not already in model (in any namespace)
    REQUIRE(model::isSIdAvailable("boogie", model) == true);
    REQUIRE(model::isSIdAvailable("A", model) == false);
    REQUIRE(model::isSIdAvailable("parametricGeometry", model) == false);

    // true if sId not already in spatial namespace
    REQUIRE(model::isSpatialIdAvailable("boogie", geom) == true);
    REQUIRE(model::isSpatialIdAvailable("A", geom) == true);
    REQUIRE(model::isSpatialIdAvailable("parametricGeometry", geom) == false);

    REQUIRE(model::nameToUniqueSId("valid_id", model) == "valid_id");

    // name valid but already taken
    REQUIRE(model::nameToUniqueSId("A", model) == "A_");

    // spaces -> underscores
    REQUIRE(model::nameToUniqueSId("name with spaces", model) ==
            "name_with_spaces");

    // dashes -> underscores
    REQUIRE(model::nameToUniqueSId("name-with-spaces", model) ==
            "name_with_spaces");

    // forward slashes -> underscores
    REQUIRE(model::nameToUniqueSId("name/with/spaces", model) ==
            "name_with_spaces");

    // all other non-alphanumeric chars ignored
    REQUIRE(model::nameToUniqueSId("!? invalid / Chars!!-", model) ==
            "_invalid___Chars_");

    // first char must be a letter or underscore
    REQUIRE(model::nameToUniqueSId("1name", model) == "_1name");

    QStringList names{"ab", "abc", "_ab", "ab_"};
    REQUIRE(model::makeUnique("a", names, "_") == "a");
    REQUIRE(model::makeUnique("ab", names, "_") == "ab__");
    REQUIRE(model::makeUnique("ab", names, "1") == "ab1");
    REQUIRE(model::makeUnique("abc", names, "_") == "abc_");
    REQUIRE(model::makeUnique("a", names, "_") == "a");
  }
}
