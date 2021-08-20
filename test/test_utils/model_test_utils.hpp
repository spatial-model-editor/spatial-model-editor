#pragma once

#include "model.hpp"
#include "mesh.hpp"
#include <sbml/SBMLTypes.h>
#include <sbml/extension/SBMLDocumentPlugin.h>
#include <sbml/packages/spatial/common/SpatialExtensionTypes.h>
#include <sbml/packages/spatial/extension/SpatialExtension.h>
#include <QString>
#include <memory>

namespace sme::test {

enum struct Mod {
  ABtoC,
  Brusselator,
  CircadianClock,
  GrayScott,
  LiverSimplified,
  LiverCells,
  SingleCompartmentDiffusion,
  VerySimpleModel
};

model::Model getExampleModel(Mod exampleModel);

model::Model getTestModel(const QString &filename);

std::unique_ptr<libsbml::SBMLDocument> getTestSbmlDoc(const QString &filename);

std::unique_ptr<libsbml::SBMLDocument> toSbmlDoc(model::Model &model);

} // namespace sme::test
