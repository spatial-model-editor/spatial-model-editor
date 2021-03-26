#include "model_compartments.hpp"
#include "geometry_sampled_field.hpp"
#include "id.hpp"
#include "logger.hpp"
#include "model_geometry.hpp"
#include "model_membranes.hpp"
#include "model_reactions.hpp"
#include "model_species.hpp"
#include "sbml_utils.hpp"
#include "simulate_data.hpp"
#include <optional>
#include <sbml/SBMLTypes.h>
#include <sbml/extension/SBMLDocumentPlugin.h>
#include <sbml/packages/spatial/common/SpatialExtensionTypes.h>
#include <sbml/packages/spatial/extension/SpatialExtension.h>
#include <string>

namespace sme {

namespace model {

static QStringList importIds(const libsbml::Model *model) {
  QStringList ids;
  unsigned int numCompartments = model->getNumCompartments();
  ids.reserve(static_cast<int>(numCompartments));
  auto nDim = getNumSpatialDimensions(model);
  for (unsigned int i = 0; i < numCompartments; ++i) {
    const auto *comp = model->getCompartment(i);
    if (!(comp->isSetSpatialDimensions() &&
          comp->getSpatialDimensions() + 1 == nDim)) {
      SPDLOG_INFO("Importing Compartment '{}'", comp->getId());
      ids.push_back(QString::fromStdString(comp->getId()));
    }
  }
  return ids;
}

static QStringList importNamesAndMakeUnique(libsbml::Model *model,
                                            const QStringList &ids) {
  QStringList names;
  names.reserve(ids.size());
  for (const auto &id : ids) {
    auto sId = id.toStdString();
    auto *comp = model->getCompartment(sId);
    if (comp->getName().empty()) {
      SPDLOG_INFO("Compartment '{0}' has no Name, using '{0}'", sId);
      comp->setName(sId);
    }
    std::string name = comp->getName();
    while (names.contains(name.c_str())) {
      name.append("_");
      SPDLOG_INFO("Changing Compartment '{}' name to '{}' to make it unique",
                  sId, name);
    }
    comp->setName(name);
    names.push_back(QString::fromStdString(name));
  }
  return names;
}

static void makeSizesValid(libsbml::Model *model) {
  constexpr double defaultCompartmentSize = 1.0;
  for (unsigned int i = 0; i < model->getNumCompartments(); ++i) {
    if (auto *comp = model->getCompartment(i); !comp->isSetSize()) {
      SPDLOG_WARN("Compartment '{}' has no size, assigning default value: {}",
                  comp->getId(), defaultCompartmentSize);
      comp->setSize(defaultCompartmentSize);
    }
  }
}

ModelCompartments::ModelCompartments() = default;

ModelCompartments::ModelCompartments(libsbml::Model *model,
                                     ModelGeometry *geometry,
                                     ModelMembranes *membranes,
                                     ModelSpecies *species,
                                     ModelReactions *reactions,
                                     simulate::SimulationData *data)
    : ids{importIds(model)}, names{importNamesAndMakeUnique(model, ids)},
      sbmlModel{model}, modelGeometry{geometry}, modelMembranes{membranes},
      modelSpecies{species}, modelReactions{reactions}, simulationData{data} {
  makeSizesValid(model);
  colours = QVector<QRgb>(ids.size(), 0);
  compartments.reserve(static_cast<std::size_t>(ids.size()));
  createDefaultCompartmentGeometryIfMissing(model);
  for (const auto &id : ids) {
    compartments.push_back(
        std::make_unique<geometry::Compartment>(id.toStdString(), QImage{}, 0));
  }
}

const QStringList &ModelCompartments::getIds() const { return ids; }

const QStringList &ModelCompartments::getNames() const { return names; }

const QVector<QRgb> &ModelCompartments::getColours() const { return colours; }

QString ModelCompartments::add(const QString &name) {
  SPDLOG_INFO("Adding new compartment");
  QString newName = name;
  while (names.contains(newName)) {
    newName.append("_");
  }
  auto *comp = sbmlModel->createCompartment();
  SPDLOG_INFO("  - name: {}", newName.toStdString());
  comp->setName(newName.toStdString());
  auto id = nameToUniqueSId(newName, sbmlModel);
  SPDLOG_INFO("  - id: {}", id.toStdString());
  comp->setId(id.toStdString());
  comp->setConstant(true);
  comp->setSpatialDimensions(
      static_cast<unsigned int>(modelGeometry->getNumDimensions()));
  ids.push_back(id);
  names.push_back(newName);
  colours.push_back(0);
  compartments.push_back(std::make_unique<geometry::Compartment>());
  createDefaultCompartmentGeometryIfMissing(sbmlModel);
  modelGeometry->updateMesh();
  modelMembranes->updateCompartments(compartments);
  modelMembranes->updateCompartmentNames(names, sbmlModel);
  hasUnsavedChanges = true;
  SPDLOG_INFO("Clearing simulation data");
  simulationData->clear();
  return newName; // should be id?
}

static void removeCompartmentFromSBML(libsbml::Model *model,
                                      const std::string &sId) {
  auto *comp = model->getCompartment(sId);
  // find and remove any spatial SBML stuff related to compartment
  const auto *scp = static_cast<const libsbml::SpatialCompartmentPlugin *>(
      comp->getPlugin("spatial"));
  if (scp->isSetCompartmentMapping()) {
    std::string domainTypeId = scp->getCompartmentMapping()->getDomainType();
    auto *geom = getOrCreateGeometry(model);
    if (const auto *dom = geom->getDomainByDomainType(domainTypeId);
        dom != nullptr) {
      if (std::unique_ptr<libsbml::Domain> rmdom(
              geom->removeDomain(dom->getId()));
          rmdom != nullptr) {
        SPDLOG_INFO("  - removed Domain {}", rmdom->getId());
      } else {
        SPDLOG_WARN("Failed to remove Domain for compartment {}", sId);
      }
    }
    auto *sfgeom = getSampledFieldGeometry(geom);
    if (const auto *sfvol = sfgeom->getSampledVolumeByDomainType(domainTypeId);
        sfvol != nullptr) {
      if (std::unique_ptr<libsbml::SampledVolume> rmsfvol(
              sfgeom->removeSampledVolume(sfvol->getId()));
          sfvol != nullptr) {
        SPDLOG_INFO("  - removed SampledVolume {}", rmsfvol->getId());
      } else {
        SPDLOG_WARN("Failed to remove SampledVolume for compartment {}", sId);
      }
    }
    if (std::unique_ptr<libsbml::DomainType> rmdt(
            geom->removeDomainType(domainTypeId));
        rmdt != nullptr) {
      SPDLOG_INFO("  - removed DomainType {}", rmdt->getId());
    } else {
      SPDLOG_WARN("Failed to remove DomainType for compartment {}", sId);
    }
  }
  if (std::unique_ptr<libsbml::Compartment> c(model->removeCompartment(sId));
      c != nullptr) {
    SPDLOG_INFO("  - removed Compartment {}", c->getId());
  } else {
    SPDLOG_WARN("Failed to remove Compartment {}", sId);
  }
}

bool ModelCompartments::remove(const QString &id) {
  SPDLOG_INFO("Removing compartment '{}'", id.toStdString());
  auto i = ids.indexOf(id);
  if (i < 0) {
    SPDLOG_WARN("Compartment '{}' not found", id.toStdString());
    return false;
  }
  hasUnsavedChanges = true;
  SPDLOG_INFO("Clearing simulation data");
  simulationData->clear();
  ids.removeAt(i);
  names.removeAt(i);
  colours.removeAt(i);
  using diffType = decltype(compartments)::difference_type;
  compartments.erase(compartments.begin() + static_cast<diffType>(i));
  removeCompartmentFromSBML(sbmlModel, id.toStdString());
  for (const auto &s : modelSpecies->getIds(id)) {
    modelSpecies->remove(s);
  }
  modelMembranes->updateCompartments(compartments);
  modelMembranes->updateCompartmentNames(names, sbmlModel);
  modelGeometry->updateMesh();
  return true;
}

QString ModelCompartments::getName(const QString &id) const {
  auto i = ids.indexOf(id);
  if (i < 0) {
    return {};
  }
  return names[i];
}

QString ModelCompartments::setName(const QString &id, const QString &name) {
  auto i = ids.indexOf(id);
  if (i < 0) {
    return {};
  }
  hasUnsavedChanges = true;
  QString uniqueName = name;
  while (names.contains(uniqueName)) {
    uniqueName.append("_");
  }
  names[i] = uniqueName;
  std::string sId{id.toStdString()};
  std::string sName{uniqueName.toStdString()};
  auto *comp = sbmlModel->getCompartment(sId);
  SPDLOG_INFO("sId '{}' : name -> '{}'", sId, sName);
  comp->setName(sName);
  modelMembranes->updateCompartmentNames(names, sbmlModel);
  return uniqueName;
}

std::optional<std::vector<QPointF>>
ModelCompartments::getInteriorPoints(const QString &id) const {
  SPDLOG_INFO("compartmentID: {}", id.toStdString());
  const auto *comp = sbmlModel->getCompartment(id.toStdString());
  const auto *scp = static_cast<const libsbml::SpatialCompartmentPlugin *>(
      comp->getPlugin("spatial"));
  const std::string &domainType = scp->getCompartmentMapping()->getDomainType();
  SPDLOG_INFO("  - domainType: {}", domainType);
  const auto *geom = getOrCreateGeometry(sbmlModel);
  const auto *domain = geom->getDomainByDomainType(domainType);
  if (domain == nullptr) {
    SPDLOG_INFO("  - no Domain found");
    return {};
  }
  SPDLOG_INFO("  - domain: {}", domain->getId());
  SPDLOG_INFO("  - numInteriorPoints: {}", domain->getNumInteriorPoints());
  if (domain->getNumInteriorPoints() == 0) {
    SPDLOG_INFO("  - no interior point found");
    return {};
  }
  std::vector<QPointF> points;
  for (unsigned i = 0; i < domain->getNumInteriorPoints(); ++i) {
    const auto *interiorPoint = domain->getInteriorPoint(i);
    points.emplace_back(interiorPoint->getCoord1(), interiorPoint->getCoord2());
    SPDLOG_INFO("  - interior point ({},{})", points.back().x(),
                points.back().y());
  }
  return points;
}

void ModelCompartments::setInteriorPoints(const QString &id,
                                          const std::vector<QPointF> &points) {
  hasUnsavedChanges = true;
  SPDLOG_INFO("compartmentID: {}", id.toStdString());
  auto *comp = sbmlModel->getCompartment(id.toStdString());
  auto *scp = static_cast<libsbml::SpatialCompartmentPlugin *>(
      comp->getPlugin("spatial"));
  const std::string &domainType = scp->getCompartmentMapping()->getDomainType();
  SPDLOG_INFO("  - domainType: {}", domainType);
  auto *geom = getOrCreateGeometry(sbmlModel);
  auto *domain = geom->getDomainByDomainType(domainType);
  SPDLOG_INFO("  - domain: {}", domain->getId());
  while (domain->getNumInteriorPoints() > 0) {
    std::unique_ptr<libsbml::InteriorPoint> ip{domain->removeInteriorPoint(0)};
    SPDLOG_INFO("  - removing interior point ({},{})", ip->getCoord1(),
                ip->getCoord2());
  }
  const auto &origin{modelGeometry->getPhysicalOrigin()};
  auto pixelWidth{modelGeometry->getPixelWidth()};
  for (const auto &point : points) {
    SPDLOG_INFO("  - creating new interior point");
    SPDLOG_INFO("    - pixel point: ({},{})", point.x(), point.y());
    auto *ip = domain->createInteriorPoint();
    // convert to physical units with pixelWidth and origin
    ip->setCoord1(origin.x() + pixelWidth * point.x());
    ip->setCoord2(origin.y() + pixelWidth * point.y());
    SPDLOG_INFO("    - physical point: ({},{})", ip->getCoord1(),
                ip->getCoord2());
  }
}

void ModelCompartments::setColour(const QString &id, QRgb colour) {
  auto i = ids.indexOf(id);
  if (i < 0) {
    return;
  }
  hasUnsavedChanges = true;
  SPDLOG_INFO("Clearing simulation data");
  simulationData->clear();
  std::string sId{id.toStdString()};
  SPDLOG_INFO("assigning colour {:x} to compartment {}", colour, sId);
  if (auto oldId = getIdFromColour(colour); colour != 0 && !oldId.isEmpty()) {
    SPDLOG_INFO("removing colour {:x} from compartment {}", colour,
                oldId.toStdString());
    setColour(oldId, 0);
  }
  colours[i] = colour;
  compartments[static_cast<std::size_t>(i)] =
      std::make_unique<geometry::Compartment>(sId, modelGeometry->getImage(),
                                              colour);
  // set SampledValue (aka colour) of SampledFieldVolume
  auto *scp = static_cast<libsbml::SpatialCompartmentPlugin *>(
      sbmlModel->getCompartment(sId)->getPlugin("spatial"));
  const std::string &domainType = scp->getCompartmentMapping()->getDomainType();
  SPDLOG_INFO("  - domainType '{}'", domainType);
  auto *geom = getOrCreateGeometry(sbmlModel);
  auto *sfgeom = getOrCreateSampledFieldGeometry(geom);
  auto *sfvol = sfgeom->getSampledVolumeByDomainType(domainType);
  if (sfvol == nullptr) {
    sfvol = sfgeom->createSampledVolume();
    sfvol->setId(sId + "_sampledVolume");
    sfvol->setDomainType(domainType);
  }
  geom->getDomainType(domainType)
      ->setSpatialDimensions(
          static_cast<int>(geom->getNumCoordinateComponents()));
  SPDLOG_INFO("  - sampledVolume '{}'", sfvol->getId());
  if (colour == 0 && sfvol->isSetSampledValue()) {
    sfvol->unsetSampledValue();
  } else {
    sfvol->setSampledValue(static_cast<double>(colour));
  }
  modelSpecies->updateCompartmentGeometry(id);
  modelMembranes->updateCompartments(compartments);
  modelMembranes->updateCompartmentNames(names, sbmlModel);
  modelGeometry->updateMesh();
  modelReactions->makeReactionsSpatial(modelMembranes->getMembranes());
}

QRgb ModelCompartments::getColour(const QString &id) const {
  auto i = ids.indexOf(id);
  if (i < 0) {
    return 0;
  }
  return colours[i];
}

QString ModelCompartments::getIdFromColour(QRgb colour) const {
  auto i = colours.indexOf(colour);
  if (i < 0) {
    return {};
  }
  return ids[i];
}

const std::vector<std::unique_ptr<geometry::Compartment>> &
ModelCompartments::getCompartments() const {
  return compartments;
}

geometry::Compartment *ModelCompartments::getCompartment(const QString &id) {
  auto i = ids.indexOf(id);
  if (i < 0) {
    return nullptr;
  }
  return compartments[static_cast<std::size_t>(i)].get();
}

const geometry::Compartment *
ModelCompartments::getCompartment(const QString &id) const {
  auto i = ids.indexOf(id);
  if (i < 0) {
    return nullptr;
  }
  return compartments[static_cast<std::size_t>(i)].get();
}

void ModelCompartments::clear() {
  ids.clear();
  names.clear();
  colours.clear();
  compartments.clear();
  hasUnsavedChanges = true;
  simulationData->clear();
}

bool ModelCompartments::getHasUnsavedChanges() const {
  return hasUnsavedChanges;
}

void ModelCompartments::setHasUnsavedChanges(bool unsavedChanges) {
  hasUnsavedChanges = unsavedChanges;
}

} // namespace model

} // namespace sme
