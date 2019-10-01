#include <QFile>

#include "catch.hpp"
#include "dune.hpp"
#include "logger.hpp"
#include "sbml_test_data/invalid_dune_names.hpp"

TEST_CASE("iniFile class", "[dune][ini][non-gui]") {
  dune::iniFile ini;
  ini.addSection("t1");
  QString correct("[t1]\n");
  REQUIRE(ini.getText() == correct);

  ini.addValue("x", "a");
  correct.append("x = a\n");
  REQUIRE(ini.getText() == correct);

  ini.addValue("y", 3);
  correct.append("y = 3\n");
  REQUIRE(ini.getText() == correct);

  ini.addValue("z", 3.14159, 10);
  correct.append("z = 3.14159\n");
  REQUIRE(ini.getText() == correct);

  ini.addValue("dblExp", 2.7e-10, 10);
  correct.append("dblExp = 2.7e-10\n");
  REQUIRE(ini.getText() == correct);

  ini.addSection("t1", "t2");
  correct.append("\n[t1.t2]\n");
  REQUIRE(ini.getText() == correct);

  ini.addSection("t1", "t2", "3_3_3");
  correct.append("\n[t1.t2.3_3_3]\n");
  REQUIRE(ini.getText() == correct);

  ini.clear();
  REQUIRE(ini.getText() == "");

  ini.addSection("a", "b", "c");
  correct = "[a.b.c]\n";
  REQUIRE(ini.getText() == correct);
}

TEST_CASE("DUNE ini file for ABtoC model", "[dune][ini][non-gui]") {
  sbml::SbmlDocWrapper s;
  QFile f(":/models/ABtoC.xml");
  f.open(QIODevice::ReadOnly);
  s.importSBMLString(f.readAll().toStdString());

  dune::DuneConverter dc(s, 14);
  QStringList ini = dc.getIniFile().split("\n");
  auto line = ini.cbegin();
  while (line != ini.cend() && *line != "[model.compartments]") {
    ++line;
  }
  REQUIRE(*line++ == "[model.compartments]");
  REQUIRE(*line++ == "comp = 0");
  REQUIRE(*line++ == "");
  REQUIRE(*line++ == "[model.comp]");
  REQUIRE((*line++).left(10) == "begin_time");
  REQUIRE((*line++).left(8) == "end_time");
  REQUIRE((*line++).left(9) == "time_step");
  REQUIRE(*line++ == "");
  REQUIRE(*line++ == "[model.comp.operator]");
  REQUIRE(*line++ == "A = 0");
  REQUIRE(*line++ == "B = 0");
  REQUIRE(*line++ == "C = 0");
  REQUIRE(*line++ == "");
  REQUIRE(*line++ == "[model.comp.initial]");
  REQUIRE(*line++ == "A = 1*A_initialConcentration(x,y)");
  REQUIRE(*line++ == "B = 1*B_initialConcentration(x,y)");
  REQUIRE(*line++ == "C = 0");
  REQUIRE(*line++ == "");
  REQUIRE(*line++ == "[model.data]");
  REQUIRE(*line++ == "A_initialConcentration = A_initialConcentration.tif");
  REQUIRE(*line++ == "B_initialConcentration = B_initialConcentration.tif");
  REQUIRE(*line++ == "");
  REQUIRE(*line++ == "[model.comp.reaction]");
  REQUIRE(*line++ == "A = -0.1*A*B");
  REQUIRE(*line++ == "B = -0.1*A*B");
  REQUIRE(*line++ == "C = 0.1*A*B");
  REQUIRE(*line++ == "");
  REQUIRE(*line++ == "[model.comp.reaction.jacobian]");
  REQUIRE(*line++ == "dA__dA = -0.1*B");
  REQUIRE(*line++ == "dA__dB = -0.1*A");
  REQUIRE(*line++ == "dA__dC = 0");
  REQUIRE(*line++ == "dB__dA = -0.1*B");
  REQUIRE(*line++ == "dB__dB = -0.1*A");
  REQUIRE(*line++ == "dB__dC = 0");
  REQUIRE(*line++ == "dC__dA = 0.1*B");
  REQUIRE(*line++ == "dC__dB = 0.1*A");
  REQUIRE(*line++ == "dC__dC = 0");
  REQUIRE(*line++ == "");
  REQUIRE(*line++ == "[model.comp.diffusion]");
  REQUIRE(*line++ == "A = 0.4");
  REQUIRE(*line++ == "B = 0.4");
  REQUIRE(*line++ == "C = 25");
  REQUIRE(*line++ == "");
  REQUIRE(*line++ == "[model.comp.writer]");
  REQUIRE((*line++).left(9) == "file_name");
}

TEST_CASE("DUNE ini file for brusselator model", "[dune][ini][non-gui]") {
  sbml::SbmlDocWrapper s;
  QFile f(":/models/brusselator-model.xml");
  f.open(QIODevice::ReadOnly);
  s.importSBMLString(f.readAll().toStdString());

  dune::DuneConverter dc(s);
  QStringList ini = dc.getIniFile().split("\n");
  auto line = ini.cbegin();
  while (line != ini.cend() && *line != "[model.compartment.reaction]") {
    ++line;
  }
  REQUIRE(*line++ == "[model.compartment.reaction]");
  REQUIRE(*line++ == "X = 0.5 - 4.0*X + 1.0*X^2*Y");
  REQUIRE(*line++ == "Y = 3.0*X - 1.0*X^2*Y");
  // the rest of the species are constant,
  // so they don't exist from DUNE's point of view:
  REQUIRE(*line++ == "");
}

TEST_CASE("DUNE ini file for very simple model", "[dune][ini][non-gui]") {
  sbml::SbmlDocWrapper s;
  QFile f(":/models/very-simple-model.xml");
  f.open(QIODevice::ReadOnly);
  s.importSBMLString(f.readAll().toStdString());

  dune::DuneConverter dc(s);
  QStringList ini = dc.getIniFile().split("\n");
  auto line = ini.cbegin();
  while (line != ini.cend() && *line != "[model.c2.reaction]") {
    ++line;
  }
  REQUIRE(*line++ == "[model.c2.reaction]");
  REQUIRE(*line++ == "A_c2 = 0.0");
  REQUIRE(*line++ == "B_c2 = 0.0");
  while (line != ini.cend() && *line != "[model.c3.reaction]") {
    ++line;
  }
  REQUIRE(*line++ == "[model.c3.reaction]");
  REQUIRE(*line++ == "A_c3 = -0.3*A_c3");
  REQUIRE(*line++ == "B_c3 = 0.3*A_c3");
  REQUIRE(*line++ == "");
  REQUIRE(*line++ == "[model.c3.reaction.jacobian]");
  REQUIRE(*line++ == "dA_c3__dA_c3 = -0.3");
  REQUIRE(*line++ == "dA_c3__dB_c3 = 0");
  REQUIRE(*line++ == "dB_c3__dA_c3 = 0.3");
  REQUIRE(*line++ == "dB_c3__dB_c3 = 0");
}

TEST_CASE("DUNE simulation of ABtoC model", "[dune][simulate][non-gui]") {
  sbml::SbmlDocWrapper s;
  QFile f(":/models/ABtoC.xml");
  f.open(QIODevice::ReadOnly);
  s.importSBMLString(f.readAll().toStdString());

  // set spatially constant initial conditions
  s.setInitialConcentration("A", 1.0);
  s.setInitialConcentration("B", 1.0);
  s.setInitialConcentration("C", 0.0);

  dune::DuneSimulation duneSim(s, QSize(200, 200));
  REQUIRE(duneSim.getAverageConcentration("A") == dbl_approx(1.0));
  REQUIRE(duneSim.getAverageConcentration("B") == dbl_approx(1.0));
  REQUIRE(duneSim.getAverageConcentration("C") == dbl_approx(0.0));
  duneSim.doTimestep(0.05);
  auto imgConcFull = duneSim.getConcImage();
  auto imgConcLinear = duneSim.getConcImage(true);
  REQUIRE(std::abs(duneSim.getAverageConcentration("A") - 0.995) < 5e-5);
  REQUIRE(std::abs(duneSim.getAverageConcentration("B") - 0.995) < 5e-5);
  REQUIRE(std::abs(duneSim.getAverageConcentration("C") - 0.005) < 5e-5);
  REQUIRE(imgConcFull.size() == QSize(200, 200));
  REQUIRE(imgConcLinear.size() == QSize(200, 200));
  std::vector<QPoint> points{
      QPoint(0, 0),    QPoint(12, 54),   QPoint(33, 31),
      QPoint(66, 3),   QPoint(88, 44),   QPoint(144, 189),
      QPoint(170, 14), QPoint(199, 199), QPoint(175, 77)};
  for (const auto& point : points) {
    REQUIRE(imgConcFull.pixel(point) == imgConcLinear.pixel(point));
  }
}

TEST_CASE("DUNE visualization self-consistency: 500x300",
          "[dune][visualization][non-gui]") {
  sbml::SbmlDocWrapper s;
  QFile f(":/models/ABtoC.xml");
  f.open(QIODevice::ReadOnly);
  s.importSBMLString(f.readAll().toStdString());

  QSize imgSize(500, 300);
  dune::DuneSimulation duneSim(s, imgSize);
  auto imgConcFull = duneSim.getConcImage();
  auto imgConcLinear = duneSim.getConcImage(true);
  REQUIRE(imgConcFull.size() == imgSize);
  REQUIRE(imgConcLinear.size() == imgSize);
  // 1st order FEM: linear interpolation should be equivalent
  std::vector<QPoint> points{
      QPoint(0, 0),    QPoint(12, 54),   QPoint(33, 31),
      QPoint(66, 3),   QPoint(88, 44),   QPoint(144, 189),
      QPoint(170, 14), QPoint(199, 199), QPoint(175, 77)};
  for (const auto& point : points) {
    REQUIRE(imgConcFull.pixel(point) == imgConcLinear.pixel(point));
  }
}

TEST_CASE("DUNE visualization analytic", "[dune][visualization][non-gui]") {
  sbml::SbmlDocWrapper s;
  QFile f(":/models/ABtoC.xml");
  f.open(QIODevice::ReadOnly);
  s.importSBMLString(f.readAll().toStdString());

  s.setAnalyticConcentration("A", "x");
  s.setAnalyticConcentration("B", "y");
  s.setAnalyticConcentration("C", "0");

  QSize imgSize(200, 200);
  dune::DuneSimulation duneSim(s, imgSize);
  auto imgConc = duneSim.getConcImage();
  imgConc.save("img.png");
  QPoint p(50, 130);
  auto oldCol = imgConc.pixel(p);
  for (int i = 0; i < 5; ++i) {
    p += QPoint(+10, -10);
    auto newCol = imgConc.pixel(p);
    CAPTURE(p);
    REQUIRE(newCol > oldCol);
    oldCol = newCol;
  }
}

TEST_CASE("Species names that are invalid dune variables",
          "[dune][ini][non-gui]") {
  std::unique_ptr<libsbml::SBMLDocument> doc(
      libsbml::readSBMLFromString(sbml_test_data::invalid_dune_names().xml));
  libsbml::SBMLWriter().writeSBML(doc.get(), "tmp.xml");
  sbml::SbmlDocWrapper s;
  s.importSBMLFile("tmp.xml");
  dune::DuneConverter dc(s);
  QStringList ini = dc.getIniFile().split("\n");
  auto line = ini.cbegin();
  while (line != ini.cend() && *line != "[model.comp.reaction]") {
    ++line;
  }
  REQUIRE(*line++ == "[model.comp.reaction]");
  REQUIRE(*line++ == "dim_ = -0.1*x__*dim_");
  REQUIRE(*line++ == "x__ = -0.1*x__*dim_");
  REQUIRE(*line++ == "x_ = 0.1*x__*dim_");
  REQUIRE(*line++ == "");
  REQUIRE(*line++ == "[model.comp.reaction.jacobian]");
  REQUIRE(*line++ == "ddim___ddim_ = -0.1*x__");
  REQUIRE(*line++ == "ddim___dx__ = -0.1*dim_");
  REQUIRE(*line++ == "ddim___dx_ = 0");
}
