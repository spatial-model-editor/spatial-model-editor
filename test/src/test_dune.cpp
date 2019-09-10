#include "dune.hpp"

#include "catch.hpp"

#include "logger.hpp"

#include <QFile>

TEST_CASE("iniFile class", "[dune][ini]") {
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

TEST_CASE("DUNE ini file for ABtoC model", "[dune][ini]") {
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
  REQUIRE(*line++ == "A = 1");
  REQUIRE(*line++ == "B = 1");
  REQUIRE(*line++ == "C = 0");
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

TEST_CASE("DUNE ini file for brusselator model", "[dune][ini]") {
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

TEST_CASE("DUNE ini file for very simple model", "[dune][ini]") {
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
