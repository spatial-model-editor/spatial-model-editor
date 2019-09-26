#include "dune.hpp"

#include <QFile>
#include <QImage>
#include <QPainter>

#include "logger.hpp"
#include "pde.hpp"
#include "reactions.hpp"
#include "symbolic.hpp"
#include "utils.hpp"

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

DuneConverter::DuneConverter(const sbml::SbmlDocWrapper &SbmlDoc,
                             int doublePrecision)
    : doc(SbmlDoc) {
  double begin_time = 0.0;
  double end_time = 0.02;
  double time_step = 0.01;

  // grid
  ini.addSection("grid");
  ini.addValue("file", "grid.msh");
  ini.addValue("initial_level", 0);

  // simulation
  ini.addSection("model");
  ini.addValue("begin_time", begin_time, doublePrecision);
  ini.addValue("end_time", end_time, doublePrecision);
  ini.addValue("time_step", time_step, doublePrecision);

  // list of compartments with corresponding gmsh surface index - 1
  ini.addSection("model.compartments");
  for (int i = 0; i < doc.compartments.size(); ++i) {
    ini.addValue(doc.compartments[i], i);
  }

  // for each compartment
  for (const auto &compartmentID : doc.compartments) {
    const auto &speciesList = doc.species.at(compartmentID);

    ini.addSection("model", compartmentID);
    ini.addValue("begin_time", begin_time, doublePrecision);
    ini.addValue("end_time", end_time, doublePrecision);
    ini.addValue("time_step", time_step, doublePrecision);

    // remove any constant species from the list of species
    std::vector<std::string> nonConstantSpecies;
    for (const auto &s : speciesList) {
      if (!doc.getIsSpeciesConstant(s.toStdString())) {
        nonConstantSpecies.push_back(s.toStdString());
      }
    }

    // operator splitting indexing: all set to zero for now...
    ini.addSection("model", compartmentID, "operator");
    for (const auto &speciesID : nonConstantSpecies) {
      ini.addValue(speciesID.c_str(), 0);
    }

    // initial concentrations
    ini.addSection("model", compartmentID, "initial");
    std::size_t i_species = 0;
    for (const auto &speciesID : nonConstantSpecies) {
      ini.addValue(nonConstantSpecies.at(i_species).c_str(),
                   doc.getInitialConcentration(speciesID.c_str()),
                   doublePrecision);
      ++i_species;
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
    pde::PDE pde(&doc, nonConstantSpecies, reacs);
    for (std::size_t i = 0; i < nSpecies; ++i) {
      ini.addValue(nonConstantSpecies.at(i).c_str(),
                   pde.getRHS().at(i).c_str());
    }

    // reaction term jacobian
    ini.addSection("model", compartmentID, "reaction.jacobian");
    for (std::size_t i = 0; i < nSpecies; ++i) {
      for (std::size_t j = 0; j < nSpecies; ++j) {
        QString lhs = QString("d%1__d%2")
                          .arg(nonConstantSpecies.at(i).c_str(),
                               nonConstantSpecies.at(j).c_str());
        QString rhs = pde.getJacobian().at(i).at(j).c_str();
        ini.addValue(lhs, rhs);
      }
    }

    // diffusion coefficients
    ini.addSection("model", compartmentID, "diffusion");
    i_species = 0;
    for (const auto &speciesID : nonConstantSpecies) {
      ini.addValue(
          nonConstantSpecies.at(i_species).c_str(),
          doc.mapSpeciesIdToField.at(speciesID.c_str()).diffusionConstant,
          doublePrecision);
      ++i_species;
    }

    // output file
    ini.addSection("model", compartmentID, "writer");
    ini.addValue("file_name", compartmentID);
  }

  // for each membrane
  for (const auto &membrane : doc.membraneVec) {
    ini.addSection("model", membrane.membraneID.c_str());
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

    // operator splitting indexing: all set to zero for now...
    ini.addSection("model", membrane.membraneID.c_str(), "operator");
    for (const auto &speciesID : nonConstantSpecies) {
      ini.addValue(speciesID.c_str(), 0);
    }

    // initial concentrations
    ini.addSection("model", membrane.membraneID.c_str(), "initial");
    std::size_t i_species = 0;
    for (const auto &speciesID : nonConstantSpecies) {
      ini.addValue(nonConstantSpecies.at(i_species).c_str(),
                   doc.getInitialConcentration(speciesID.c_str()),
                   doublePrecision);
      ++i_species;
    }

    // reactions
    ini.addSection("model", membrane.membraneID.c_str(), "reaction");
    std::size_t nSpecies = static_cast<std::size_t>(nonConstantSpecies.size());

    std::vector<std::string> reacs;
    if (doc.reactions.find(membrane.membraneID.c_str()) !=
        doc.reactions.cend()) {
      for (const auto &r : doc.reactions.at(membrane.membraneID.c_str())) {
        reacs.push_back(r.toStdString());
      }
    }
    pde::PDE pde(&doc, nonConstantSpecies, reacs);
    for (std::size_t i = 0; i < nSpecies; ++i) {
      ini.addValue(nonConstantSpecies.at(i).c_str(),
                   pde.getRHS().at(i).c_str());
    }

    // reaction term jacobian
    ini.addSection("model", membrane.membraneID.c_str(), "reaction.jacobian");
    for (std::size_t i = 0; i < nSpecies; ++i) {
      for (std::size_t j = 0; j < nSpecies; ++j) {
        QString lhs = QString("d%1__d%2")
                          .arg(nonConstantSpecies.at(i).c_str(),
                               nonConstantSpecies.at(j).c_str());
        QString rhs = pde.getJacobian().at(i).at(j).c_str();
        ini.addValue(lhs, rhs);
      }
    }

    // diffusion coefficients
    ini.addSection("model", membrane.membraneID.c_str(), "diffusion");
    i_species = 0;
    for (const auto &speciesID : nonConstantSpecies) {
      ini.addValue(
          nonConstantSpecies.at(i_species).c_str(),
          doc.mapSpeciesIdToField.at(speciesID.c_str()).diffusionConstant,
          doublePrecision);
      ++i_species;
    }

    // output file
    ini.addSection("model", membrane.membraneID.c_str(), "writer");
    ini.addValue("file_name", membrane.membraneID.c_str());
  }

  // logger settings
  ini.addSection("logging");
  ini.addValue("default.level", "off");

  //  ini.addSection("logging.backend.model");
  //  ini.addValue("level", "debug");
  //  ini.addValue("indent", 2);

  //  ini.addSection("logging.backend.solver");
  //  ini.addValue("level", "debug");
  //  ini.addValue("indent", 4);
}

QString DuneConverter::getIniFile() const { return ini.getText(); }

void DuneSimulation::initDuneModel(const sbml::SbmlDocWrapper &sbmlDoc) {
  geometrySize = sbmlDoc.getCompartmentImage().size() * sbmlDoc.getPixelWidth();
  // export gmsh file `grid.msh` in the same dir
  QFile f2("grid.msh");
  if (f2.open(QIODevice::ReadWrite | QIODevice::Text)) {
    f2.write(sbmlDoc.mesh.getGMSH().toUtf8());
  }
  f2.close();

  // pass dune ini file directly as istream
  std::stringstream ssIni(
      dune::DuneConverter(sbmlDoc).getIniFile().toStdString());
  Dune::ParameterTreeParser::readINITree(ssIni, config);

  // init Dune logging if not already done & mute it
  if (!Dune::Logging::Logging::initialized()) {
    auto &mpi_helper = Dune::MPIHelper::instance(0, nullptr);
    auto comm = mpi_helper.getCollectiveCommunication();
    Dune::Logging::Logging::init(comm, config.sub("logging"));
    Dune::Logging::Logging::mute();
  }

  // NB: msh file needs to be file for gmshreader
  std::tie(grid_ptr, host_grid_ptr) =
      Dune::Copasi::GmshReader<dune::Grid>::read("grid.msh", config);

  // initialize model
  model = std::make_unique<
      Dune::Copasi::ModelMultiDomainDiffusionReaction<dune::Grid>>(
      grid_ptr, config.sub("model"));
}

void DuneSimulation::updateCompartmentNames() {
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

void DuneSimulation::updateSpeciesNames() {
  speciesNames.clear();
  for (std::size_t iDomain = 0; iDomain < compartmentNames.size(); ++iDomain) {
    SPDLOG_DEBUG("compartment[{}]: {}", iDomain, compartmentNames.at(iDomain));
    // NB: species index is position in *sorted* list of species names
    // so make copy of list of names from ini file and sort it
    auto names =
        config.sub("model." + compartmentNames.at(iDomain) + ".initial")
            .getValueKeys();
    std::sort(names.begin(), names.end());
    SPDLOG_DEBUG("  - species: {}", names);
    speciesNames.push_back(std::move(names));
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

DuneSimulation::DuneSimulation(const sbml::SbmlDocWrapper &sbmlDoc) {
  initDuneModel(sbmlDoc);
  updateCompartmentNames();
  updateSpeciesNames();
  updateTriangles();
  // temp call to construct map to av concentrations as side effect
  // todo: remove this
  QImage img = getConcImage();
}

void DuneSimulation::doTimestep(double dt) {
  model->end_time() = model->current_time() + dt;
  model->run();
}

QImage DuneSimulation::getConcImage(const QSize &imageSize) {
  // resize to imageSize but maintain aspect ratio
  double scaleFactor = std::min(imageSize.width() / geometrySize.width(),
                                imageSize.height() / geometrySize.height());
  QImage img(imageSize, QImage::Format_ARGB32_Premultiplied);
  img.fill(0);
  QPainter p(&img);
  p.setRenderHint(QPainter::Antialiasing);
  p.setPen(QPen(Qt::black, 1));
  QBrush fillBrush(QColor(0, 0, 0));
  for (std::size_t iDomain = 0; iDomain < compartmentNames.size(); ++iDomain) {
    SPDLOG_TRACE("compartment[{}]: {}", iDomain, compartmentNames.at(iDomain));
    const auto &gridview =
        grid_ptr->subDomain(static_cast<int>(iDomain)).leafGridView();

    // get average conc for each triangle & species
    // also keep track of max conc for each species
    // todo: ensure species colours match GUI simulator colours
    // todo: do linear interpolation between vertices instead of av over
    // triangle (i.e. 1st order instead of 0th order)
    std::vector<std::vector<double>> conc;
    std::vector<double> concMax;
    const auto &species = speciesNames.at(iDomain);
    const auto &compTriangles = triangles.at(iDomain);
    for (std::size_t iSpecies = 0; iSpecies < species.size(); ++iSpecies) {
      auto gf = model->get_grid_function(model->states(), iDomain, iSpecies);
      using GF = decltype(gf);
      using Range = typename GF::Traits::RangeType;
      using Domain = typename GF::Traits::DomainType;
      Range result;
      double m = 0;
      conc.emplace_back();
      conc.back().reserve(compTriangles.size());
      for (const auto e : elements(gridview)) {
        double av = 0;
        for (const auto &dom : {Domain{0, 0}, Domain{1, 0}, Domain{0, 1}}) {
          gf.evaluate(e, dom, result);
          av += result / 3.0;
        }
        m = std::max(m, av);
        conc.back().push_back(av);
      }
      concMax.push_back(m < 1e-15 ? 1.0 : m);
      mapSpeciesIDToAvConc[species.at(iSpecies)] =
          std::accumulate(conc.back().begin(), conc.back().end(), 0.0) /
          static_cast<double>(conc.back().size());
      SPDLOG_TRACE("  - species[{}]: {} - max = {}", iSpecies,
                   species.at(iSpecies), concMax.back());
    }

    // equal contribution from each field
    double alpha = 1.0 / static_cast<double>(species.size());
    for (std::size_t i = 0; i < compTriangles.size(); ++i) {
      int r = 0;
      int g = 0;
      int b = 0;
      for (std::size_t i_f = 0; i_f < species.size(); ++i_f) {
        double c = alpha * conc[i_f][i] / concMax[i_f];
        QColor col = utils::indexedColours()[i_f];
        r += static_cast<int>(col.red() * c);
        g += static_cast<int>(col.green() * c);
        b += static_cast<int>(col.blue() * c);
      }
      const auto &t = compTriangles[i];
      QPainterPath path(t[0] * scaleFactor);
      path.lineTo(t[1] * scaleFactor);
      path.lineTo(t[2] * scaleFactor);
      path.lineTo(t[0] * scaleFactor);
      fillBrush.setColor(qRgb(r, g, b));
      p.setBrush(fillBrush);
      p.drawPath(path);
    }
  }
  p.end();
  return img.mirrored(false, true);
}

double DuneSimulation::getAverageConcentration(
    const std::string &speciesID) const {
  return mapSpeciesIDToAvConc.at(speciesID);
}

}  // namespace dune
