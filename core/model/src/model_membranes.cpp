#include "sme/model_membranes.hpp"
#include "sbml_utils.hpp"
#include "sme/logger.hpp"
#include "sme/model_compartments.hpp"
#include "sme/model_geometry.hpp"
#include "sme/model_membranes_util.hpp"
#include <QImage>
#include <QPoint>
#include <QString>
#include <fmt/core.h>
#include <sbml/SBMLTypes.h>
#include <sbml/extension/SBMLDocumentPlugin.h>
#include <sbml/packages/spatial/common/SpatialExtensionTypes.h>
#include <sbml/packages/spatial/extension/SpatialExtension.h>
#include <stdexcept>

namespace sme::model {

const QStringList &ModelMembranes::getIds() const { return ids; }

const QStringList &ModelMembranes::getNames() const { return names; }

QString ModelMembranes::setName(const QString &id, const QString &name) {
  auto i = ids.indexOf(id);
  if (i < 0) {
    return {};
  }
  if (names[i] == name) {
    // no-op: setting name to the same value as it already had
    return name;
  }
  hasUnsavedChanges = true;
  names[i] = name;
  return name;
}

QString ModelMembranes::getName(const QString &id) const {
  auto i = ids.indexOf(id);
  if (i < 0) {
    return {};
  }
  return names[i];
}

const std::vector<geometry::Membrane> &ModelMembranes::getMembranes() const {
  return membranes;
}

const geometry::Membrane *ModelMembranes::getMembrane(const QString &id) const {
  auto i = ids.indexOf(id);
  if (i < 0) {
    return nullptr;
  }
  return &membranes[static_cast<std::size_t>(i)];
}

const std::vector<std::pair<std::string, std::pair<QRgb, QRgb>>> &
ModelMembranes::getIdColourPairs() const {
  return idColourPairs;
}

double ModelMembranes::getSize(const QString &id) const {
  const auto *compartment{sbmlModel->getCompartment(id.toStdString())};
  if (compartment == nullptr || !compartment->isSetSize()) {
    return 0.0;
  }
  return compartment->getSize();
}

void ModelMembranes::updateCompartmentNames(
    const QStringList &compartmentNames) {
  names.clear();
  names.reserve(compartmentNames.size());
  hasUnsavedChanges = true;
  for (const auto &m : membranes) {
    SPDLOG_TRACE("compA: {}", m.getCompartmentA()->getId());
    SPDLOG_TRACE("compB: {}", m.getCompartmentB()->getId());
    auto iCompA = compIds.indexOf(m.getCompartmentA()->getId().c_str());
    auto iCompB = compIds.indexOf(m.getCompartmentB()->getId().c_str());
    auto id = QString("%1 <-> %2")
                  .arg(compartmentNames[iCompA])
                  .arg(compartmentNames[iCompB]);
    SPDLOG_TRACE("  -> {}", id.toStdString());
    names.push_back(id);
  }
  importMembraneIdsAndNames();
}

void ModelMembranes::updateCompartments(
    const std::vector<std::unique_ptr<geometry::Compartment>> &compartments) {
  hasUnsavedChanges = true;
  compIds.clear();
  compIds.reserve(static_cast<int>(compartments.size()));
  for (const auto &compartment : compartments) {
    compIds.push_back(compartment->getId().c_str());
    SPDLOG_TRACE("  - {}", compartment->getId());
    SPDLOG_TRACE("    - {:x}", compartment->getColour());
  }
  membranes.clear();
  idColourPairs.clear();
  ids.clear();
  if (membranePixels == nullptr) {
    return;
  }
  for (std::size_t j = 1; j < compartments.size(); ++j) {
    for (std::size_t i = 0; i < j; ++i) {
      const auto *compA = compartments[i].get();
      const auto *compB = compartments[j].get();
      auto colourA = compA->getColour();
      auto colourB = compB->getColour();
      if (colourA == 0 || colourB == 0) {
        break;
      }
      auto colourIndexA = membranePixels->getColourIndex(colourA);
      auto colourIndexB = membranePixels->getColourIndex(colourB);
      if (colourIndexB < colourIndexA) {
        std::swap(compA, compB);
        std::swap(colourA, colourB);
        std::swap(colourIndexA, colourIndexB);
      }
      SPDLOG_TRACE("-compA '{}': colour [{}] = {:x}", compA->getId(),
                   colourIndexA, colourA);
      SPDLOG_TRACE("-compB '{}': colour [{}] = {:x}", compB->getId(),
                   colourIndexB, colourB);
      if (const auto *pixelPairs =
              membranePixels->getPoints(colourIndexA, colourIndexB);
          pixelPairs != nullptr) {
        SPDLOG_TRACE("  -> {} point membrane", pixelPairs->size());
        std::string mId{compA->getId()};
        mId.append("_").append(compB->getId()).append("_membrane");
        membranes.emplace_back(mId, compA, compB, pixelPairs);
        ids.push_back(QString(mId.c_str()));
        idColourPairs.push_back({mId, {colourA, colourB}});
      }
    }
  }
}

void ModelMembranes::updateCompartmentImage(const std::vector<QImage> &imgs) {
  membranePixels = std::make_unique<ImageMembranePixels>(imgs);
  SPDLOG_TRACE("{} colour image:", img.colorCount());
}

void ModelMembranes::importMembraneIdsAndNames() {
  if (sbmlModel == nullptr) {
    return;
  }
  auto nDim = getNumSpatialDimensions(sbmlModel);
  for (unsigned int i = 0; i < sbmlModel->getNumCompartments(); ++i) {
    const auto *comp = sbmlModel->getCompartment(i);
    if (comp->isSetSpatialDimensions() &&
        comp->getSpatialDimensions() + 1 == nDim) {
      const auto &mId = comp->getId();
      auto adjacentCompartments = getAdjacentCompartments(sbmlModel, mId);
      if (adjacentCompartments.has_value()) {
        const auto &[c1, c2] = adjacentCompartments.value();
        for (auto &m : membranes) {
          const auto &compA = m.getCompartmentA()->getId();
          const auto &compB = m.getCompartmentB()->getId();
          if ((compA == c1 && compB == c2) || (compA == c2 && compB == c1)) {
            auto mIndex = ids.indexOf(m.getId().c_str());
            SPDLOG_TRACE("Found membrane[{}] '{}' in SBML document", mIndex,
                         mId);
            m.setId(mId);
            SPDLOG_TRACE("  id -> '{}'", mId);
            ids[mIndex] = QString::fromStdString(m.getId());
            if (!comp->getName().empty()) {
              SPDLOG_TRACE("  name -> '{}'", comp->getName());
              names[mIndex] = QString::fromStdString(comp->getName());
            }
          }
        }
      }
    }
  }
}

void ModelMembranes::exportToSBML(double pixelArea) {
  if (sbmlModel == nullptr) {
    SPDLOG_WARN("no sbml model to export to - ignoring");
  }
  // ensure all membranes have a corresponding n-1 dim compartment in SBML
  auto *geom{getOrCreateGeometry(sbmlModel)};
  auto nDimMinusOne{geom->getNumCoordinateComponents() - 1};
  for (int i = 0; i < ids.size(); ++i) {
    std::string sId{ids[i].toStdString()};
    SPDLOG_INFO("Membrane id: '{}'", sId);
    auto *comp{sbmlModel->getCompartment(sId)};
    if (comp == nullptr) {
      SPDLOG_INFO("  - creating Membrane compartment in SBML");
      comp = sbmlModel->createCompartment();
      comp->setId(sId);
    }
    comp->setName(names[i].toStdString());
    SPDLOG_INFO("  - name: {}", comp->getName());
    comp->setConstant(true);
    comp->setSpatialDimensions(nDimMinusOne);
    if (comp->isSetUnits()) {
      // we set the model units, compartment units are then inferred from that
      comp->unsetUnits();
    }
    auto nPixels{membranes[static_cast<std::size_t>(i)].getIndexPairs().size()};
    double area{static_cast<double>(nPixels) * pixelArea};
    SPDLOG_INFO("  - size: {}", area);
    comp->setSize(area);
    auto *scp = dynamic_cast<libsbml::SpatialCompartmentPlugin *>(
        comp->getPlugin("spatial"));
    libsbml::CompartmentMapping *cmap{nullptr};
    libsbml::DomainType *dt{nullptr};
    libsbml::Domain *dom{nullptr};
    if (scp->isSetCompartmentMapping()) {
      cmap = scp->getCompartmentMapping();
      dt = geom->getDomainType(cmap->getDomainType());
      dom = geom->getDomainByDomainType(dt->getId());
    }
    if (dt == nullptr) {
      SPDLOG_INFO("  - creating DomainType");
      dt = geom->createDomainType();
      dt->setId(sId + "_domainType");
    }
    dt->setSpatialDimensions(static_cast<int>(nDimMinusOne));
    if (dom == nullptr) {
      SPDLOG_INFO("  - creating Domain");
      dom = geom->createDomain();
      dom->setId(sId + "_domain");
      dom->setDomainType(dt->getId());
    }
    if (cmap == nullptr) {
      SPDLOG_INFO("  - creating CompartmentMapping");
      cmap = scp->createCompartmentMapping();
      cmap->setId(sId + "_compartmentMapping");
      cmap->setDomainType(dt->getId());
    }
    cmap->setUnitSize(1.0);
    SPDLOG_INFO("  - CompartmentMapping '{}'", cmap->getId());
    SPDLOG_INFO("  - DomainType '{}'", dt->getId());
    SPDLOG_INFO("  - Domain '{}'", dom->getId());
  }
  // update all adjacentDomains in SBML
  geom->getListOfAdjacentDomains()->clear();
  for (const auto &membrane : membranes) {
    std::string adjId = membrane.getId();
    adjId.append("_adjacentDomain");
    auto domId = getDomainIdFromCompartmentId(sbmlModel, membrane.getId());
    auto *adjDomA = geom->createAdjacentDomains();
    auto adjIdA = adjId + "A";
    auto domIdA = getDomainIdFromCompartmentId(
        sbmlModel, membrane.getCompartmentA()->getId());
    adjDomA->setId(adjIdA);
    adjDomA->setDomain1(domId);
    adjDomA->setDomain2(domIdA);
    auto *adjDomB = geom->createAdjacentDomains();
    auto adjIdB = adjId + "B";
    auto domIdB = getDomainIdFromCompartmentId(
        sbmlModel, membrane.getCompartmentB()->getId());
    adjDomB->setId(adjIdB);
    adjDomB->setDomain1(domId);
    adjDomB->setDomain2(domIdB);
  }
}

ModelMembranes::ModelMembranes(libsbml::Model *model) : sbmlModel{model} {}

ModelMembranes::ModelMembranes(ModelMembranes &&) noexcept = default;

ModelMembranes &ModelMembranes::operator=(ModelMembranes &&) noexcept = default;

ModelMembranes::~ModelMembranes() = default;

bool ModelMembranes::getHasUnsavedChanges() const { return hasUnsavedChanges; }

void ModelMembranes::setHasUnsavedChanges(bool unsavedChanges) {
  hasUnsavedChanges = unsavedChanges;
}

} // namespace sme::model
