#include "dune.hpp"

#include <QFile>
#include <QImage>
#include <QPainter>
#include <algorithm>

#include "logger.hpp"
#include "pde.hpp"
#include "reactions.hpp"
#include "symbolic.hpp"
#include "tiff.hpp"
#include "utils.hpp"

static std::vector<std::string> makeValidDuneSpeciesNames(
    const std::vector<std::string> &names) {
  std::vector<std::string> duneNames = names;
  // muparser reserved words, taken from:
  // https://beltoforion.de/article.php?a=muparser&p=features
  std::vector<std::string> reservedNames{
      {"sin",  "cos",  "tan",   "asin",  "acos",  "atan", "sinh",
       "cosh", "tanh", "asinh", "acosh", "atanh", "log2", "log10",
       "log",  "ln",   "exp",   "sqrt",  "sign",  "rint", "abs",
       "min",  "max",  "sum",   "avg"}};
  // dune-copasi reserved words:
  reservedNames.insert(reservedNames.end(), {"x", "y", "t", "pi", "dim"});
  for (auto &name : duneNames) {
    SPDLOG_TRACE("name {}", name);
    std::string duneName = name;
    name = "";
    // if species name clashes with a reserved name, append an underscore
    if (std::find(reservedNames.cbegin(), reservedNames.cend(), duneName) !=
        reservedNames.cend()) {
      duneName.append("_");
    }
    // if species name clashes with another species name,
    // append another underscore
    while (std::find(duneNames.cbegin(), duneNames.cend(), duneName) !=
           duneNames.cend()) {
      duneName.append("_");
    }
    name = duneName;
    SPDLOG_TRACE("  -> {}", name);
  }
  return duneNames;
}

namespace dune {

const QString &iniFile::getText() const { return text; }

void iniFile::addSection(const QString &str) {
  if (!text.isEmpty()) {
    text.append("\n");
  }
  text.append(QString("[%1]\n").arg(str));
}

void iniFile::addSection(const QString &str1, const QString &str2) {
  addSection(QString("%1.%2").arg(str1, str2));
}

void iniFile::addSection(const QString &str1, const QString &str2,
                         const QString &str3) {
  addSection(QString("%1.%2.%3").arg(str1, str2, str3));
}

void iniFile::addValue(const QString &var, const QString &value) {
  text.append(QString("%1 = %2\n").arg(var, value));
}

void iniFile::addValue(const QString &var, int value) {
  addValue(var, QString::number(value));
}

void iniFile::addValue(const QString &var, double value, int precision) {
  addValue(var, QString::number(value, 'g', precision));
}

void iniFile::clear() { text.clear(); }

DuneConverter::DuneConverter(const sbml::SbmlDocWrapper &SbmlDoc, double dt,
                             int doublePrecision)
    : doc(SbmlDoc) {
  double begin_time = 0.0;
  double end_time = 0.02;
  double time_step = dt;

  mapDuneNameToColour.clear();

  // grid
  ini.addSection("grid");
  ini.addValue("file", "grid.msh");
  ini.addValue("initial_level", 0);

  // simulation
  ini.addSection("model");
  ini.addValue("begin_time", begin_time, doublePrecision);
  ini.addValue("end_time", end_time, doublePrecision);
  ini.addValue("time_step", time_step, doublePrecision);
  ini.addValue("order", 1);

  // list of compartments with corresponding gmsh surface index - 1
  ini.addSection("model.compartments");
  int compMeshIndex = 0;
  for (const auto &comp : doc.compartments) {
    ini.addValue(comp, compMeshIndex);
    ++compMeshIndex;
  }
  for (const auto &mem : doc.membranes) {
    ini.addValue(mem, compMeshIndex);
    ++compMeshIndex;
  }

  // for each compartment
  for (const auto &compartmentID : doc.compartments) {
    ini.addSection("model", compartmentID);
    ini.addValue("begin_time", begin_time, doublePrecision);
    ini.addValue("end_time", end_time, doublePrecision);
    ini.addValue("time_step", time_step, doublePrecision);

    // remove any constant species from the list of species
    std::vector<std::string> nonConstantSpecies;
    for (const auto &s : doc.species.at(compartmentID)) {
      if (!doc.getIsSpeciesConstant(s.toStdString())) {
        nonConstantSpecies.push_back(s.toStdString());
      }
    }
    auto duneSpeciesNames = makeValidDuneSpeciesNames(nonConstantSpecies);
    for (std::size_t is = 0; is < duneSpeciesNames.size(); ++is) {
      mapDuneNameToColour[duneSpeciesNames.at(is)] =
          doc.getSpeciesColour(nonConstantSpecies.at(is).c_str());
    }

    // operator splitting indexing: all set to zero for now...
    ini.addSection("model", compartmentID, "operator");
    for (const auto &speciesID : duneSpeciesNames) {
      ini.addValue(speciesID.c_str(), 0);
    }

    // initial concentrations
    std::vector<QString> tiffs;
    ini.addSection("model", compartmentID, "initial");
    for (std::size_t i = 0; i < nonConstantSpecies.size(); ++i) {
      QString name = nonConstantSpecies.at(i).c_str();
      QString duneName = duneSpeciesNames.at(i).c_str();
      double initConc = doc.getInitialConcentration(name);
      QString expr = doc.getAnalyticConcentration(name);
      QString sampledField =
          doc.getSpeciesSampledFieldInitialAssignment(name.toStdString())
              .c_str();
      if (!sampledField.isEmpty()) {
        // if there is a sampledField then make a TIFF
        auto sampledFieldFile = QString("%1.tif").arg(sampledField);
        double max = utils::writeTIFF(sampledFieldFile.toStdString(),
                                      doc.mapSpeciesIdToField.at(name),
                                      doc.getPixelWidth());
        tiffs.push_back(sampledField);
        ini.addValue(duneName,
                     QString("%2*%3(x,y)").arg(max).arg(sampledField));
      } else if (!expr.isEmpty()) {
        // otherwise, initialAssignments take precedence:
        std::string e = doc.inlineExpr(expr.toStdString());
        symbolic::Symbolic sym(e, {"x", "y"}, doc.getGlobalConstants());
        ini.addValue(duneName, sym.simplify().c_str());
      } else {
        // otherwise just use initialConcentration value
        ini.addValue(duneName, initConc, doublePrecision);
      }
    }
    if (!tiffs.empty()) {
      ini.addSection("model", "data");
      for (const auto &tiff : tiffs) {
        ini.addValue(tiff, tiff + QString(".tif"));
      }
    }

    // reactions
    ini.addSection("model", compartmentID, "reaction");
    std::size_t nSpecies = static_cast<std::size_t>(nonConstantSpecies.size());

    std::vector<std::string> reacs;
    if (doc.reactions.find(compartmentID) != doc.reactions.cend()) {
      for (const auto &r : doc.reactions.at(compartmentID)) {
        reacs.push_back(r.toStdString());
      }
    }
    pde::PDE pde(&doc, nonConstantSpecies, reacs, duneSpeciesNames);
    for (std::size_t i = 0; i < nSpecies; ++i) {
      ini.addValue(duneSpeciesNames.at(i).c_str(), pde.getRHS().at(i).c_str());
    }

    // reaction term jacobian
    ini.addSection("model", compartmentID, "reaction.jacobian");
    for (std::size_t i = 0; i < nSpecies; ++i) {
      for (std::size_t j = 0; j < nSpecies; ++j) {
        QString lhs = QString("d%1__d%2")
                          .arg(duneSpeciesNames.at(i).c_str(),
                               duneSpeciesNames.at(j).c_str());
        QString rhs = pde.getJacobian().at(i).at(j).c_str();
        ini.addValue(lhs, rhs);
      }
    }

    // diffusion coefficients
    ini.addSection("model", compartmentID, "diffusion");
    for (std::size_t i = 0; i < nSpecies; ++i) {
      ini.addValue(duneSpeciesNames.at(i).c_str(),
                   doc.getDiffusionConstant(nonConstantSpecies.at(i).c_str()),
                   doublePrecision);
    }

    // output file
    ini.addSection("model", compartmentID, "writer");
    ini.addValue("file_name", compartmentID);
  }

  // for each membrane do the same
  for (const auto &membrane : doc.membraneVec) {
    QString membraneID = membrane.membraneID.c_str();
    ini.addSection("model", membraneID);
    ini.addValue("begin_time", begin_time, doublePrecision);
    ini.addValue("end_time", end_time, doublePrecision);
    ini.addValue("time_step", time_step, doublePrecision);

    // remove any constant species from the list of species
    std::vector<std::string> nonConstantSpecies;
    QString compA = membrane.compA->compartmentID.c_str();
    QString compB = membrane.compB->compartmentID.c_str();
    for (const auto &s : doc.species.at(compA)) {
      if (!doc.getIsSpeciesConstant(s.toStdString())) {
        nonConstantSpecies.push_back(s.toStdString());
      }
    }
    for (const auto &s : doc.species.at(compB)) {
      if (!doc.getIsSpeciesConstant(s.toStdString())) {
        nonConstantSpecies.push_back(s.toStdString());
      }
    }
    auto duneSpeciesNames = makeValidDuneSpeciesNames(nonConstantSpecies);
    for (std::size_t is = 0; is < duneSpeciesNames.size(); ++is) {
      mapDuneNameToColour[duneSpeciesNames.at(is)] =
          doc.getSpeciesColour(nonConstantSpecies.at(is).c_str());
    }

    // operator splitting indexing: all set to zero for now...
    ini.addSection("model", membraneID, "operator");
    for (const auto &speciesID : nonConstantSpecies) {
      ini.addValue(speciesID.c_str(), 0);
    }

    // initial concentrations
    ini.addSection("model", membraneID, "initial");
    for (std::size_t i = 0; i < nonConstantSpecies.size(); ++i) {
      ini.addValue(
          duneSpeciesNames.at(i).c_str(),
          doc.getInitialConcentration(nonConstantSpecies.at(i).c_str()),
          doublePrecision);
    }

    // reactions: want reactions for both neighbouring compartments
    // as well as membrane reactions (that involve species from both
    // compartments in the same reaction)
    ini.addSection("model", membraneID, "reaction");
    std::size_t nSpecies = static_cast<std::size_t>(nonConstantSpecies.size());

    std::vector<std::string> reacs;
    std::vector<std::string> reacScaleFactors;
    for (const auto &comp : {compA, compB}) {
      if (doc.reactions.find(comp) != doc.reactions.cend()) {
        for (const auto &r : doc.reactions.at(comp)) {
          reacs.push_back(r.toStdString());
          reacScaleFactors.push_back("1");
        }
      }
    }
    // divide membrane reaction rates by width of membrane
    if (doc.reactions.find(membraneID) != doc.reactions.cend()) {
      for (const auto &r : doc.reactions.at(membraneID)) {
        double width = doc.mesh.getMembraneWidth(membraneID.toStdString());
        reacScaleFactors.push_back(
            QString::number(width, 'g', 17).toStdString());
        SPDLOG_INFO("dividing membrane reaction by membrane width {}:", width);
        reacs.push_back(r.toStdString());
      }
    }
    pde::PDE pde(&doc, nonConstantSpecies, reacs, duneSpeciesNames,
                 reacScaleFactors);
    for (std::size_t i = 0; i < nSpecies; ++i) {
      ini.addValue(duneSpeciesNames.at(i).c_str(), pde.getRHS().at(i).c_str());
    }

    // reaction term jacobian
    ini.addSection("model", membrane.membraneID.c_str(), "reaction.jacobian");
    for (std::size_t i = 0; i < nSpecies; ++i) {
      for (std::size_t j = 0; j < nSpecies; ++j) {
        QString lhs = QString("d%1__d%2")
                          .arg(duneSpeciesNames.at(i).c_str(),
                               duneSpeciesNames.at(j).c_str());
        QString rhs = pde.getJacobian().at(i).at(j).c_str();
        ini.addValue(lhs, rhs);
      }
    }

    // diffusion coefficients
    ini.addSection("model", membrane.membraneID.c_str(), "diffusion");
    for (std::size_t i = 0; i < nSpecies; ++i) {
      ini.addValue(duneSpeciesNames.at(i).c_str(),
                   doc.getDiffusionConstant(nonConstantSpecies.at(i).c_str()),
                   doublePrecision);
    }

    // output file
    ini.addSection("model", membrane.membraneID.c_str(), "writer");
    ini.addValue("file_name", membrane.membraneID.c_str());
  }

  // logger settings
  ini.addSection("logging");
  if (SPDLOG_ACTIVE_LEVEL >= 2) {
    // for release builds disable DUNE logging
    ini.addValue("default.level", "off");
  } else {
    // for debug builds enable verbose DUNE logging
    ini.addValue("default.level", "trace");
    ini.addSection("logging.backend.model");
    ini.addValue("level", "trace");
    ini.addValue("indent", 2);

    ini.addSection("logging.backend.solver");
    ini.addValue("level", "trace");
    ini.addValue("indent", 4);
  }
}

QString DuneConverter::getIniFile() const { return ini.getText(); }

QColor DuneConverter::getSpeciesColour(const std::string &duneName) const {
  return mapDuneNameToColour.at(duneName);
}

void DuneSimulation::initDuneModel(const sbml::SbmlDocWrapper &sbmlDoc,
                                   double dt) {
  geometrySize = sbmlDoc.getCompartmentImage().size() * sbmlDoc.getPixelWidth();
  // export gmsh file `grid.msh` in the same dir
  QFile f2("grid.msh");
  if (f2.open(QIODevice::WriteOnly | QIODevice::Text)) {
    f2.write(sbmlDoc.mesh.getGMSH().toUtf8());
    f2.close();
  } else {
    SPDLOG_ERROR("Cannot write to file grid.msh");
  }

  // pass dune ini file directly as istream
  dune::DuneConverter dc(sbmlDoc, dt);
  std::stringstream ssIni(dc.getIniFile().toStdString());
  Dune::ParameterTreeParser::readINITree(ssIni, config);

  // init Dune logging if not already done
  if (!Dune::Logging::Logging::initialized()) {
    Dune::Logging::Logging::init(
        Dune::FakeMPIHelper::getCollectiveCommunication(),
        config.sub("logging"));
    if (SPDLOG_ACTIVE_LEVEL >= 2) {
      // for release builds disable DUNE logging
      Dune::Logging::Logging::mute();
    }
  }

  // NB: msh file needs to be file for gmshreader
  std::tie(grid_ptr, host_grid_ptr) =
      Dune::Copasi::GmshReader<dune::Grid>::read("grid.msh", config);

  // initialize model
  model = std::make_unique<
      Dune::Copasi::ModelMultiDomainDiffusionReaction<ModelTraits>>(
      grid_ptr, config.sub("model"));

  initCompartmentNames();
  initSpeciesNames(dc);
}

void DuneSimulation::initCompartmentNames() {
  const auto &compartments = config.sub("model.compartments").getValueKeys();
  compartmentNames.resize(compartments.size());
  for (const auto &compartment : compartments) {
    std::size_t iDomain =
        config.sub("model.compartments").get<std::size_t>(compartment);
    SPDLOG_DEBUG("compartment[{}]: {}", iDomain, compartment);
    if (iDomain >= compartmentNames.size()) {
      SPDLOG_ERROR(
          "found compartment index {}, but there are only {} compartments",
          iDomain, compartmentNames.size());
    }
    compartmentNames[iDomain] = compartment;
  }
}

void DuneSimulation::initSpeciesNames(const DuneConverter &dc) {
  speciesNames.clear();
  speciesColours.clear();
  for (std::size_t iDomain = 0; iDomain < compartmentNames.size(); ++iDomain) {
    SPDLOG_DEBUG("compartment[{}]: {}", iDomain, compartmentNames.at(iDomain));
    // NB: species index is position in *sorted* list of species names
    // so make copy of list of names from ini file and sort it
    auto names =
        config.sub("model." + compartmentNames.at(iDomain) + ".initial")
            .getValueKeys();
    std::sort(names.begin(), names.end());
    speciesNames.push_back(std::move(names));
    auto &speciesColoursCompartment = speciesColours.emplace_back();
    for (const auto &name : speciesNames.back()) {
      const QColor &c = dc.getSpeciesColour(name);
      speciesColoursCompartment.push_back(c);
      SPDLOG_DEBUG("  - species: {} --> colour {}", names, c);
    }
  }
}

static std::pair<QPoint, QPoint> getBoundingBox(const QTriangleF &t,
                                                const QPointF &scale) {
  // get triangle bounding box in physical units
  QPointF fmin(t[0].x(), t[0].y());
  QPointF fmax = fmin;
  for (std::size_t i = 1; i < 3; ++i) {
    fmin.setX(std::min(fmin.x(), t[i].x()));
    fmax.setX(std::max(fmax.x(), t[i].x()));
    fmin.setY(std::min(fmin.y(), t[i].y()));
    fmax.setY(std::max(fmax.y(), t[i].y()));
  }
  // convert physical points to pixel locations
  return std::make_pair<QPoint, QPoint>(
      QPoint(static_cast<int>(fmin.x() * scale.x()),
             static_cast<int>(fmin.y() * scale.y())),
      QPoint(static_cast<int>(fmax.x() * scale.x()),
             static_cast<int>(fmax.y() * scale.y())));
}

void DuneSimulation::updatePixels() {
  pixels.clear();
  for (std::size_t iDomain = 0; iDomain < compartmentNames.size(); ++iDomain) {
    auto &pixelsComp = pixels.emplace_back();
    const auto &gridview =
        grid_ptr->subDomain(static_cast<int>(iDomain)).leafGridView();
    SPDLOG_TRACE("compartment[{}]: {}", iDomain, compartmentNames.at(iDomain));
    // get vertices of triangles:
    for (const auto e : elements(gridview)) {
      auto &pixelsTriangle = pixelsComp.emplace_back();
      const auto &geo = e.geometry();
      assert(geo.type().isTriangle());
      auto ref = Dune::referenceElement(geo);
      QPointF c0(geo.corner(0)[0], geo.corner(0)[1]);
      QPointF c1(geo.corner(1)[0], geo.corner(1)[1]);
      QPointF c2(geo.corner(2)[0], geo.corner(2)[1]);
      auto [pMin, pMax] = getBoundingBox({{c0, c1, c2}}, scaleFactor);
      SPDLOG_TRACE("  - bounding box {} - {}", pMin, pMax);
      for (int x = pMin.x(); x < pMax.x() + 1; ++x) {
        for (int y = pMin.y(); y < pMax.y() + 1; ++y) {
          auto localPoint =
              e.geometry().local({static_cast<double>(x) / scaleFactor.x(),
                                  static_cast<double>(y) / scaleFactor.y()});
          if (ref.checkInside(localPoint)) {
            pixelsTriangle.push_back(
                std::make_pair(QPoint(x, y), std::move(localPoint)));
          }
        }
      }
      SPDLOG_TRACE("    - found {} pixels", pixelsTriangle.size());
    }
  }
}

void DuneSimulation::updateTriangles() {
  triangles.clear();
  for (std::size_t iDomain = 0; iDomain < compartmentNames.size(); ++iDomain) {
    triangles.emplace_back();
    const auto &gridview =
        grid_ptr->subDomain(static_cast<int>(iDomain)).leafGridView();
    SPDLOG_TRACE("compartment[{}]: {}", iDomain, compartmentNames.at(iDomain));
    // get vertices of triangles:
    for (const auto e : elements(gridview)) {
      const auto &geo = e.geometry();
      assert(geo.type().isTriangle());
      QPointF c0(geo.corner(0)[0], geo.corner(0)[1]);
      QPointF c1(geo.corner(1)[0], geo.corner(1)[1]);
      QPointF c2(geo.corner(2)[0], geo.corner(2)[1]);
      triangles.back().push_back({{c0, c1, c2}});
    }
    SPDLOG_TRACE("  - found {} triangles", triangles.back().size());
  }
}

void DuneSimulation::updateBarycentricWeights() {
  SPDLOG_TRACE("geometry size: {}", geometrySize);
  SPDLOG_TRACE("image size: {}", imageSize);
  SPDLOG_TRACE("scaleFactor (where pixel = scaleFactor*physical): {}",
               scaleFactor);
  weights.clear();
  for (const auto &comp : triangles) {
    SPDLOG_TRACE("compartment with {} triangles:", comp.size());
    weights.emplace_back();
    for (const auto &t : comp) {
      auto &triangleWeights = weights.back().emplace_back();
      auto [pMin, pMax] = getBoundingBox(t, scaleFactor);
      SPDLOG_TRACE("  - bounding box {} - {}", pMin, pMax);
      // get weights for each point
      double denom = (t[1].y() - t[2].y()) * (t[0].x() - t[2].x()) +
                     (t[2].x() - t[1].x()) * (t[0].y() - t[2].y());
      QPointF w1((t[1].y() - t[2].y()) / denom, (t[2].x() - t[1].x()) / denom);
      QPointF w2((t[2].y() - t[0].y()) / denom, (t[0].x() - t[2].x()) / denom);
      for (int x = pMin.x(); x < pMax.x() + 1; ++x) {
        for (int y = pMin.y(); y < pMax.y() + 1; ++y) {
          double W1 =
              w1.x() * (static_cast<double>(x) / scaleFactor.x() - t[2].x()) +
              w1.y() * (static_cast<double>(y) / scaleFactor.y() - t[2].y());
          double W2 =
              w2.x() * (static_cast<double>(x) / scaleFactor.x() - t[2].x()) +
              w2.y() * (static_cast<double>(y) / scaleFactor.y() - t[2].y());
          double W3 = 1.0 - W1 - W2;
          if (W1 >= 0 && W2 >= 0 && W3 >= 0) {
            // if all weights positive: add point
            triangleWeights.push_back({QPoint(x, y), {W1, W2, W3}});
          }
        }
      }
      SPDLOG_TRACE("  - triangle with {} pixels", triangleWeights.size());
    }
  }
}

void DuneSimulation::updateSpeciesConcentrations() {
  concentrations.clear();
  maxConcs.clear();
  for (std::size_t iDomain = 0; iDomain < compartmentNames.size(); ++iDomain) {
    maxConcs.emplace_back();
    SPDLOG_TRACE("compartment[{}]: {}", iDomain, compartmentNames.at(iDomain));
    const auto &gridview =
        grid_ptr->subDomain(static_cast<int>(iDomain)).leafGridView();
    const auto &species = speciesNames.at(iDomain);
    const auto &compTriangles = triangles.at(iDomain);
    auto &comp = concentrations.emplace_back();
    for (std::size_t iSpecies = 0; iSpecies < species.size(); ++iSpecies) {
      auto &spec = comp.emplace_back();
      auto gf = model->get_grid_function(model->states(), iDomain, iSpecies);
      using GF = decltype(gf);
      using Range = typename GF::Traits::RangeType;
      using Domain = typename GF::Traits::DomainType;
      Range result;
      spec.reserve(compTriangles.size());
      double avC = 0;
      double maxC = 0;
      for (const auto e : elements(gridview)) {
        auto &corners = spec.emplace_back();
        corners.reserve(3);
        for (const auto &dom : {Domain{0, 0}, Domain{1, 0}, Domain{0, 1}}) {
          gf.evaluate(e, dom, result);
          corners.push_back(result[0]);
          avC += corners.back();
          maxC = std::max(maxC, corners.back());
        }
      }
      avC /= static_cast<double>(spec.size() * 3);
      mapSpeciesIDToAvConc[species.at(iSpecies)] = avC;
      maxConcs.back().push_back(maxC);
      SPDLOG_TRACE("  - species[{}]: {}", iSpecies, species.at(iSpecies));
      SPDLOG_TRACE("    - avg = {}", avC);
      SPDLOG_TRACE("    - max = {}", maxC);
    }
  }
}

DuneSimulation::DuneSimulation(const sbml::SbmlDocWrapper &sbmlDoc, double dt,
                               QSize imgSize) {
  initDuneModel(sbmlDoc, dt);
  setImageSize(imgSize);
  updateSpeciesConcentrations();
}

void DuneSimulation::doTimestep(double dt) {
  double endTime = model->current_time() + dt;
  while (model->current_time() < endTime) {
    model->step();
  }
  updateSpeciesConcentrations();
}

QRgb DuneSimulation::pixelColour(std::size_t iDomain,
                                 const std::vector<double> &concs) const {
  double alpha = 1.0 / static_cast<double>(concs.size());
  int r = 0;
  int g = 0;
  int b = 0;
  for (std::size_t iSpecies = 0; iSpecies < concs.size(); ++iSpecies) {
    const QColor &col = speciesColours.at(iDomain).at(iSpecies);
    double c = concs[iSpecies] * alpha;
    c /= maxConcs[iDomain][iSpecies];
    r += static_cast<int>(col.red() * c);
    g += static_cast<int>(col.green() * c);
    b += static_cast<int>(col.blue() * c);
  }
  return qRgb(r, g, b);
}

QImage DuneSimulation::getConcImage(bool linearInterpolationOnly) const {
  QImage img(imageSize, QImage::Format_ARGB32_Premultiplied);
  img.fill(0);
  using GF = decltype(model->get_grid_function(model->states(), 0, 0));
  for (std::size_t iDomain = 0; iDomain < compartmentNames.size(); ++iDomain) {
    std::size_t nSpecies = speciesNames.at(iDomain).size();
    // get grid function for each species in this compartment
    std::vector<GF> gridFunctions;
    gridFunctions.reserve(nSpecies);
    for (std::size_t iSpecies = 0; iSpecies < nSpecies; ++iSpecies) {
      gridFunctions.push_back(
          model->get_grid_function(model->states(), iDomain, iSpecies));
    }
    const auto &domainWeights = weights.at(iDomain);
    const auto &gridview =
        grid_ptr->subDomain(static_cast<int>(iDomain)).leafGridView();
    std::size_t iTriangle = 0;
    for (const auto e : elements(gridview)) {
      SPDLOG_TRACE("triangle {}", iTriangle);
      if (linearInterpolationOnly) {
        for (const auto &weight : domainWeights[iTriangle]) {
          auto [point, w] = weight;
          SPDLOG_TRACE("  - point {}", point);
          SPDLOG_TRACE("  - weights {}", w);
          std::vector<double> localConcs;
          localConcs.reserve(nSpecies);
          // interpolate linearly between corner values
          for (std::size_t iSpecies = 0; iSpecies < nSpecies; ++iSpecies) {
            const auto &conc = concentrations[iDomain][iSpecies][iTriangle];
            double c = w[0] * conc[0] + w[1] * conc[1] + w[2] * conc[2];
            // replace negative values with zero
            localConcs.push_back(c < 0 ? 0 : c);
          }
          img.setPixel(point, pixelColour(iDomain, localConcs));
        }
      } else {
        for (const auto &pair : pixels[iDomain][iTriangle]) {
          // evaluate DUNE grid function at this pixel location
          // convert pixel->global->local
          auto localPoint = pair.second;
          SPDLOG_TRACE("  - pixel {} -> -> local ({},{})", pair.first,
                       localPoint[0], localPoint[1]);
          GF::Traits::RangeType result;
          std::vector<double> localConcs;
          localConcs.reserve(nSpecies);
          for (std::size_t iSpecies = 0; iSpecies < nSpecies; ++iSpecies) {
            gridFunctions[iSpecies].evaluate(e, localPoint, result);
            SPDLOG_TRACE("    - species[{}] = {}", iSpecies, result[0]);
            // replace negative values with zero
            localConcs.push_back(result[0] < 0 ? 0 : result[0]);
          }
          img.setPixel(pair.first, pixelColour(iDomain, localConcs));
        }
      }
      ++iTriangle;
    }
  }
  // (0,0) pixel is bottom-left in the above, but (0,0) is top-left in QImage:
  return img.mirrored(false, true);
}

double DuneSimulation::getAverageConcentration(
    const std::string &speciesID) const {
  return mapSpeciesIDToAvConc.at(speciesID);
}

void DuneSimulation::setImageSize(const QSize &imgSize) {
  // maintain aspect-ratio
  double scale = std::min(imgSize.width() / geometrySize.width(),
                          imgSize.height() / geometrySize.height());
  imageSize.setWidth(static_cast<int>(scale * geometrySize.width()));
  imageSize.setHeight(static_cast<int>(scale * geometrySize.height()));
  scaleFactor.setX(scale);
  scaleFactor.setY(scale);
  updatePixels();
  updateTriangles();
  updateBarycentricWeights();
}

}  // namespace dune
