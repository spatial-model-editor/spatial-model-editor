// SBML document wrapper
// - uses libSBML to read/write SBML document
// - provides the contents in Qt containers for display
// - augments the model with spatial information
// - keeps track of geometry, membranes, mesh, colors, etc

#pragma once

#include "sme/image_stack.hpp"
#include "sme/model_compartments.hpp"
#include "sme/model_events.hpp"
#include "sme/model_functions.hpp"
#include "sme/model_geometry.hpp"
#include "sme/model_math.hpp"
#include "sme/model_membranes.hpp"
#include "sme/model_parameters.hpp"
#include "sme/model_reactions.hpp"
#include "sme/model_settings.hpp"
#include "sme/model_species.hpp"
#include "sme/model_types.hpp"
#include "sme/model_units.hpp"
#include "sme/optimize_options.hpp"
#include "sme/serialization.hpp"
#include "sme/simulate.hpp"
#include "sme/simulate_options.hpp"
#include <QColor>
#include <QImage>
#include <QStringList>
#include <memory>
#include <optional>

namespace libsbml {
class SBMLDocument;
}

namespace sme::mesh {
class Mesh2d;
}

namespace sme::model {

struct SpeciesGeometry {
  const common::Volume &compartmentImageSize;
  const std::vector<common::Voxel> &compartmentVoxels;
  const common::VoxelF &physicalOrigin;
  const common::VolumeF &voxelSize;
  const ModelUnits &modelUnits;
};

class Model {
private:
  std::unique_ptr<libsbml::SBMLDocument> doc;
  std::unique_ptr<sme::common::SmeFileContents> smeFileContents;
  std::unique_ptr<Settings> settings;
  bool isValid{false};
  QString errorMessage{};
  QString currentFilename{};

  std::unique_ptr<ModelCompartments> modelCompartments;
  std::unique_ptr<ModelEvents> modelEvents;
  std::unique_ptr<ModelGeometry> modelGeometry;
  std::unique_ptr<ModelMembranes> modelMembranes;
  std::unique_ptr<ModelSpecies> modelSpecies;
  std::unique_ptr<ModelReactions> modelReactions;
  std::unique_ptr<ModelFunctions> modelFunctions;
  std::unique_ptr<ModelParameters> modelParameters;
  std::unique_ptr<ModelUnits> modelUnits;
  std::unique_ptr<ModelMath> modelMath;

  void initModelData(bool emptySpatialModel = false);
  void setHasUnsavedChanges(bool unsavedChanges);
  void updateSBMLDoc();

public:
  [[nodiscard]] bool getIsValid() const;
  [[nodiscard]] const QString &getErrorMessage() const;
  [[nodiscard]] bool getHasUnsavedChanges() const;
  [[nodiscard]] const QString &getCurrentFilename() const;

  void setName(const QString &name);
  [[nodiscard]] QString getName() const;

  ModelCompartments &getCompartments();
  [[nodiscard]] const ModelCompartments &getCompartments() const;
  ModelGeometry &getGeometry();
  [[nodiscard]] const ModelGeometry &getGeometry() const;
  ModelMembranes &getMembranes();
  [[nodiscard]] const ModelMembranes &getMembranes() const;
  ModelSpecies &getSpecies();
  [[nodiscard]] const ModelSpecies &getSpecies() const;
  ModelReactions &getReactions();
  [[nodiscard]] const ModelReactions &getReactions() const;
  ModelFunctions &getFunctions();
  [[nodiscard]] const ModelFunctions &getFunctions() const;
  ModelParameters &getParameters();
  [[nodiscard]] const ModelParameters &getParameters() const;
  ModelEvents &getEvents();
  [[nodiscard]] const ModelEvents &getEvents() const;
  ModelUnits &getUnits();
  [[nodiscard]] const ModelUnits &getUnits() const;
  ModelMath &getMath();
  [[nodiscard]] const ModelMath &getMath() const;
  simulate::SimulationData &getSimulationData();
  [[nodiscard]] const simulate::SimulationData &getSimulationData() const;
  SimulationSettings &getSimulationSettings();
  [[nodiscard]] const SimulationSettings &getSimulationSettings() const;
  MeshParameters &getMeshParameters();
  [[nodiscard]] const MeshParameters &getMeshParameters() const;
  simulate::OptimizeOptions &getOptimizeOptions();
  [[nodiscard]] const simulate::OptimizeOptions &getOptimizeOptions() const;
  [[nodiscard]] const std::vector<QRgb> &getSampledFieldColors() const;

  explicit Model();
  Model(Model &&) noexcept = default;
  Model &operator=(Model &&) noexcept = default;
  Model &operator=(const Model &) = delete;
  Model(const Model &) = delete;
  ~Model();

  void createSBMLFile(const std::string &name);
  void importSBMLFile(const std::string &filename);
  void importSBMLString(const std::string &xml,
                        const std::string &filename = {});
  void exportSBMLFile(const std::string &filename);
  void importFile(const std::string &filename);
  void exportSMEFile(const std::string &filename);
  QString getXml();
  void clear();

  [[nodiscard]] SpeciesGeometry
  getSpeciesGeometry(const QString &speciesID) const;

  [[nodiscard]] std::string inlineExpr(const std::string &mathExpression) const;

  [[nodiscard]] DisplayOptions getDisplayOptions() const;
  void setDisplayOptions(const DisplayOptions &displayOptions);
};

} // namespace sme::model
