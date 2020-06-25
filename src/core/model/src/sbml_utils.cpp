#include "sbml_utils.hpp"

#include <sbml/SBMLTypes.h>
#include <sbml/extension/SBMLDocumentPlugin.h>
#include <sbml/packages/spatial/common/SpatialExtensionTypes.h>
#include <sbml/packages/spatial/extension/SpatialExtension.h>

#include "logger.hpp"

libsbml::SampledFieldGeometry *
getSampledFieldGeometry(libsbml::Geometry *geom) {
  for (unsigned i = 0; i < geom->getNumGeometryDefinitions(); ++i) {
    auto *def = geom->getGeometryDefinition(i);
    if (def->getIsActive() && def->isSampledFieldGeometry()) {
      return static_cast<libsbml::SampledFieldGeometry *>(def);
    }
  }
  return nullptr;
}

const libsbml::Geometry *getOrCreateGeometry(const libsbml::Model *model) {
  const auto *spatial = static_cast<const libsbml::SpatialModelPlugin *>(
      model->getPlugin("spatial"));
  if (spatial == nullptr) {
    return nullptr;
  }
  return spatial->getGeometry();
}

libsbml::Geometry *getOrCreateGeometry(libsbml::Model *model) {
  auto *spatial =
      static_cast<libsbml::SpatialModelPlugin *>(model->getPlugin("spatial"));
  if (spatial == nullptr) {
    return nullptr;
  }
  auto *geom = spatial->getGeometry();
  if (geom == nullptr) {
    geom = spatial->createGeometry();
  }
  return geom;
}

void createDefaultCompartmentGeometryIfMissing(libsbml::Model *model) {
  for (unsigned i = 0; i < model->getNumCompartments(); ++i) {
    auto *comp = model->getCompartment(i);
    if (comp == nullptr) {
      return;
    }
    std::string sId = comp->getId();
    SPDLOG_INFO("Compartment '{}'", sId);
    auto *geom = getOrCreateGeometry(model);
    auto *scp = static_cast<libsbml::SpatialCompartmentPlugin *>(
        comp->getPlugin("spatial"));
    libsbml::CompartmentMapping *cmap{nullptr};
    libsbml::DomainType *dt{nullptr};
    libsbml::Domain *dom{nullptr};
    if (scp->isSetCompartmentMapping()) {
      cmap = scp->getCompartmentMapping();
      dt = geom->getDomainType(cmap->getDomainType());
      if (dt != nullptr) {
        dom = geom->getDomainByDomainType(dt->getId());
      }
    }
    if (dt == nullptr) {
      dt = geom->createDomainType();
      dt->setId(sId + "_domainType");
    }
    if (dom == nullptr) {
      dom = geom->createDomain();
      dom->setId(sId + "_domain");
      dom->setDomainType(dt->getId());
    }
    if (cmap == nullptr) {
      cmap = scp->createCompartmentMapping();
      cmap->setId(sId + "_compartmentMapping");
      cmap->setDomainType(dt->getId());
    }
    cmap->setUnitSize(1.0);
    SPDLOG_INFO("  - CompartmentMapping '{}'", cmap->getId());
    SPDLOG_INFO("  - DomainType '{}'", dt->getId());
    SPDLOG_INFO("  - Domain '{}'", dom->getId());
  }
}

unsigned int getNumSpatialDimensions(const libsbml::Model *model) {
  const auto *geom = getOrCreateGeometry(model);
  if (geom == nullptr) {
    return 0;
  }
  unsigned int numCoords = geom->getNumCoordinateComponents();
  SPDLOG_INFO(" number of coordinate components: {}", numCoords);
  unsigned int maxCompDim = 0;
  for (unsigned int i = 0; i < model->getNumCompartments(); ++i) {
    const auto *comp = model->getCompartment(i);
    if (comp->isSetSpatialDimensions()) {
      maxCompDim = std::max(maxCompDim, comp->getSpatialDimensions());
    }
  }
  SPDLOG_INFO(" max dimensions of any compartment: {}", maxCompDim);
  if (maxCompDim > numCoords) {
    SPDLOG_WARN(
        "compartment exists with more dimensions than number of coordinates");
  }
  return maxCompDim;
}

static std::string getCompartmentIdFromDomainId(const libsbml::Model *model,
                                                const std::string &domainId) {
  const auto *geom = getOrCreateGeometry(model);
  const auto *domain = geom->getDomain(domainId);
  if (domain == nullptr) {
    return {};
  }
  const auto &domainTypeId = domain->getDomainType();
  for (unsigned i = 0; i < model->getNumCompartments(); ++i) {
    const auto *comp = model->getCompartment(i);
    if (const auto *scp =
            static_cast<const libsbml::SpatialCompartmentPlugin *>(
                comp->getPlugin("spatial"));
        scp != nullptr && scp->isSetCompartmentMapping() &&
        scp->getCompartmentMapping()->getDomainType() == domainTypeId) {
      return comp->getId();
    }
  }
  return {};
}

std::string getDomainIdFromCompartmentId(const libsbml::Model *model,
                                         const std::string &compartmentId) {
  const auto *geom = getOrCreateGeometry(model);
  if (geom == nullptr) {
    return {};
  }
  const auto *scp = static_cast<const libsbml::SpatialCompartmentPlugin *>(
      model->getCompartment(compartmentId)->getPlugin("spatial"));
  if (scp == nullptr || !scp->isSetCompartmentMapping()) {
    return {};
  }
  const std::string &domainType = scp->getCompartmentMapping()->getDomainType();
  const auto *domain = geom->getDomainByDomainType(domainType);
  if (domain == nullptr) {
    return {};
  }
  return domain->getId();
}

std::optional<std::pair<std::string, std::string>>
getAdjacentCompartments(const libsbml::Model *model,
                        const std::string &compartmentId) {
  auto domainId = getDomainIdFromCompartmentId(model, compartmentId);
  const auto *geom = getOrCreateGeometry(model);
  std::vector<std::string> adjacentDomains;
  adjacentDomains.reserve(2);
  for (unsigned int i = 0; i < geom->getNumAdjacentDomains(); ++i) {
    const auto *adjDom = geom->getAdjacentDomains(i);
    if (adjDom->isSetDomain1() && adjDom->getDomain1() == domainId) {
      adjacentDomains.push_back(adjDom->getDomain2());
    }
    if (adjDom->isSetDomain2() && adjDom->getDomain2() == domainId) {
      adjacentDomains.push_back(adjDom->getDomain1());
    }
  }
  if (adjacentDomains.size() < 2) {
    return {};
  }
  return {{getCompartmentIdFromDomainId(model, adjacentDomains[0]),
           getCompartmentIdFromDomainId(model, adjacentDomains[1])}};
}

bool getIsSpeciesConstant(const libsbml::Species *spec) {
  if (spec->isSetConstant() && spec->getConstant()) {
    // `Constant` species is a constant:
    //  - cannot be altered by Reactions or by RateRules
    return true;
  }
  if ((spec->isSetBoundaryCondition() && spec->getBoundaryCondition()) &&
      spec->getModel()->getRateRule(spec->getId()) == nullptr) {
    // `BoundaryCondition` species is a constant:
    //   - unless it is altered by a RateRule
    return true;
  }
  return false;
}
