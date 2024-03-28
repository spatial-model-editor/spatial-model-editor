#pragma once

#include "sme/mesh2d.hpp"
#include "sme/mesh3d.hpp"
#include "sme/model.hpp"
#include <QFile>
#include <QString>
#include <memory>
#include <sbml/SBMLTypes.h>
#include <sbml/extension/SBMLDocumentPlugin.h>
#include <sbml/packages/spatial/common/SpatialExtensionTypes.h>
#include <sbml/packages/spatial/extension/SpatialExtension.h>

namespace sme::test {

enum struct Mod {
  ABtoC,
  Brusselator,
  CircadianClock,
  GrayScott,
  GrayScott3D,
  LiverSimplified,
  LiverCells,
  SingleCompartmentDiffusion,
  SingleCompartmentDiffusion3D,
  VerySimpleModel,
  VerySimpleModel3D
};

model::Model getExampleModel(Mod exampleModel);

model::Model getTestModel(const QString &filename);

std::unique_ptr<libsbml::SBMLDocument> getExampleSbmlDoc(Mod exampleModel);

std::unique_ptr<libsbml::SBMLDocument> getTestSbmlDoc(const QString &filename);

std::unique_ptr<libsbml::SBMLDocument> toSbmlDoc(model::Model &model);

void createBinaryFile(const QString &test_resource_filename,
                      const QString &output_filename);

} // namespace sme::test
