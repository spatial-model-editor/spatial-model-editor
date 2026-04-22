// SBML document wrapper
// - uses libSBML to read/write SBML document
// - provides the contents in Qt containers for display
// - augments the model with spatial information
// - keeps track of geometry, membranes, mesh, colors, etc

#pragma once

#include "sme/image_stack.hpp"
#include "sme/model_compartments.hpp"
#include "sme/model_events.hpp"
#include "sme/model_features.hpp"
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

/**
 * @brief Geometry context needed for species analytic/image evaluation.
 */
struct SpeciesGeometry {
  const common::Volume &compartmentImageSize;
  const std::vector<common::Voxel> &compartmentVoxels;
  const common::VoxelF &physicalOrigin;
  const common::VolumeF &voxelSize;
  const ModelUnits &modelUnits;
};

/**
 * @brief High-level wrapper around SBML model, geometry, and simulation data.
 */
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
  std::unique_ptr<ModelFeatures> modelFeatures;

  void initModelData(bool emptySpatialModel = false);
  void setHasUnsavedChanges(bool unsavedChanges);
  void updateSBMLDoc();

public:
  /**
   * @brief Returns ``true`` if model loaded/constructed successfully.
   * @returns ``true`` if model state is valid.
   */
  [[nodiscard]] bool getIsValid() const;
  /**
   * @brief Error message from last load/parse failure.
   * @returns Error message string.
   */
  [[nodiscard]] const QString &getErrorMessage() const;
  /**
   * @brief Returns ``true`` if any contained model data is unsaved.
   * @returns ``true`` if model has unsaved changes.
   */
  [[nodiscard]] bool getHasUnsavedChanges() const;
  /**
   * @brief Current file path associated with model.
   * @returns Current filename/path.
   */
  [[nodiscard]] const QString &getCurrentFilename() const;

  /**
   * @brief Set model name.
   * @param name New model name.
   */
  void setName(const QString &name);
  /**
   * @brief Get model name.
   * @returns Model name.
   */
  [[nodiscard]] QString getName() const;

  /**
   * @brief Mutable access to compartment manager.
   */
  ModelCompartments &getCompartments();
  /**
   * @brief Immutable access to compartment manager.
   */
  [[nodiscard]] const ModelCompartments &getCompartments() const;
  /**
   * @brief Mutable access to geometry manager.
   */
  ModelGeometry &getGeometry();
  /**
   * @brief Immutable access to geometry manager.
   */
  [[nodiscard]] const ModelGeometry &getGeometry() const;
  /**
   * @brief Mutable access to membrane manager.
   */
  ModelMembranes &getMembranes();
  /**
   * @brief Immutable access to membrane manager.
   */
  [[nodiscard]] const ModelMembranes &getMembranes() const;
  /**
   * @brief Mutable access to species manager.
   */
  ModelSpecies &getSpecies();
  /**
   * @brief Immutable access to species manager.
   */
  [[nodiscard]] const ModelSpecies &getSpecies() const;
  /**
   * @brief Mutable access to reaction manager.
   */
  ModelReactions &getReactions();
  /**
   * @brief Immutable access to reaction manager.
   */
  [[nodiscard]] const ModelReactions &getReactions() const;
  /**
   * @brief Mutable access to function-definition manager.
   */
  ModelFunctions &getFunctions();
  /**
   * @brief Immutable access to function-definition manager.
   */
  [[nodiscard]] const ModelFunctions &getFunctions() const;
  /**
   * @brief Mutable access to parameter manager.
   */
  ModelParameters &getParameters();
  /**
   * @brief Immutable access to parameter manager.
   */
  [[nodiscard]] const ModelParameters &getParameters() const;
  /**
   * @brief Mutable access to event manager.
   */
  ModelEvents &getEvents();
  /**
   * @brief Immutable access to event manager.
   */
  [[nodiscard]] const ModelEvents &getEvents() const;
  /**
   * @brief Mutable access to units manager.
   */
  ModelUnits &getUnits();
  /**
   * @brief Immutable access to units manager.
   */
  [[nodiscard]] const ModelUnits &getUnits() const;
  /**
   * @brief Mutable access to math parser/evaluator.
   */
  ModelMath &getMath();
  /**
   * @brief Immutable access to math parser/evaluator.
   */
  [[nodiscard]] const ModelMath &getMath() const;
  /**
   * @brief Mutable simulation results storage.
   */
  simulate::SimulationData &getSimulationData();
  /**
   * @brief Immutable simulation results storage.
   */
  [[nodiscard]] const simulate::SimulationData &getSimulationData() const;
  /**
   * @brief Mutable simulation settings.
   */
  SimulationSettings &getSimulationSettings();
  /**
   * @brief Immutable simulation settings.
   */
  [[nodiscard]] const SimulationSettings &getSimulationSettings() const;
  /**
   * @brief Mutable mesh settings.
   */
  MeshParameters &getMeshParameters();
  /**
   * @brief Immutable mesh settings.
   */
  [[nodiscard]] const MeshParameters &getMeshParameters() const;
  /**
   * @brief Mutable optimization settings.
   */
  simulate::OptimizeOptions &getOptimizeOptions();
  /**
   * @brief Immutable optimization settings.
   */
  [[nodiscard]] const simulate::OptimizeOptions &getOptimizeOptions() const;
  /**
   * @brief Imported sampled-field colors.
   * @returns Vector of sampled-field colors.
   */
  [[nodiscard]] const std::vector<QRgb> &getSampledFieldColors() const;
  /**
   * @brief Mutable access to feature manager.
   */
  ModelFeatures &getFeatures();
  /**
   * @brief Immutable access to feature manager.
   */
  [[nodiscard]] const ModelFeatures &getFeatures() const;

  /**
   * @brief Construct empty model wrapper.
   */
  explicit Model();
  Model(Model &&) noexcept = default;
  Model &operator=(Model &&) noexcept = default;
  Model &operator=(const Model &) = delete;
  Model(const Model &) = delete;
  /**
   * @brief Destructor.
   */
  ~Model();

  /**
   * @brief Create a new spatial SBML model.
   * @param name Model name.
   */
  void createSBMLFile(const std::string &name);
  /**
   * @brief Import SBML from file.
   * @param filename Path to SBML file.
   */
  void importSBMLFile(const std::string &filename);
  /**
   * @brief Import SBML from XML string.
   * @param xml SBML XML text.
   * @param filename Optional source filename.
   */
  void importSBMLString(const std::string &xml,
                        const std::string &filename = {});
  /**
   * @brief Export SBML to file.
   * @param filename Output SBML filename.
   */
  void exportSBMLFile(const std::string &filename);
  /**
   * @brief Import SBML/SME file by extension/probing.
   * @param filename Input filename.
   */
  void importFile(const std::string &filename);
  /**
   * @brief Export SME project file.
   * @param filename Output SME filename.
   */
  void exportSMEFile(const std::string &filename);
  /**
   * @brief Serialize current SBML XML.
   * @returns SBML XML as text.
   */
  QString getXml();
  /**
   * @brief Clear model state.
   */
  void clear();

  /**
   * @brief Geometry context for given species id.
   * @param speciesID Species id.
   * @returns Geometry context for species.
   */
  [[nodiscard]] SpeciesGeometry
  getSpeciesGeometry(const QString &speciesID) const;

  /**
   * @brief Inline functions/assignments in a math expression.
   * @param mathExpression Expression to inline.
   * @returns Inlined expression string.
   */
  [[nodiscard]] std::string inlineExpr(const std::string &mathExpression) const;

  /**
   * @brief Current display options.
   * @returns Display options.
   */
  [[nodiscard]] DisplayOptions getDisplayOptions() const;
  /**
   * @brief Set display options.
   * @param displayOptions New display options.
   */
  void setDisplayOptions(const DisplayOptions &displayOptions);
};

} // namespace sme::model
