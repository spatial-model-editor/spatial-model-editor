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
  REQUIRE(ini[10] == "[model]");
  REQUIRE(ini[14] == "[model.compartements]");
  REQUIRE(ini[15] == "comp = 0");
  REQUIRE(ini[16] == "");
  REQUIRE(ini[17] == "[model.comp]");
  REQUIRE(ini[18] == "name = comp");
  REQUIRE(ini[20] == "[model.comp.initial]");
  REQUIRE(ini[21] == "A = 1");
  REQUIRE(ini[22] == "B = 1");
  REQUIRE(ini[23] == "C = 0");
  REQUIRE(ini[25] == "[model.comp.reaction]");
  REQUIRE(ini[26] == "A = -0.1*A*B");
  REQUIRE(ini[27] == "B = -0.1*A*B");
  REQUIRE(ini[28] == "C = 0.1*A*B");
  REQUIRE(ini[30] == "[model.comp.reaction.jacobian]");
  REQUIRE(ini[31] == "d(A)/d(A) = -0.1*B");
  REQUIRE(ini[32] == "d(A)/d(B) = -0.1*A");
  REQUIRE(ini[33] == "d(A)/d(C) = 0");
  REQUIRE(ini[34] == "d(B)/d(A) = -0.1*B");
  REQUIRE(ini[35] == "d(B)/d(B) = -0.1*A");
  REQUIRE(ini[36] == "d(B)/d(C) = 0");
  REQUIRE(ini[37] == "d(C)/d(A) = 0.1*B");
  REQUIRE(ini[38] == "d(C)/d(B) = 0.1*A");
  REQUIRE(ini[39] == "d(C)/d(C) = 0");
  REQUIRE(ini[41] == "[model.comp.diffusion]");
  REQUIRE(ini[42] == "A = 0.4");
  REQUIRE(ini[43] == "B = 0.4");
  REQUIRE(ini[44] == "C = 25");
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
  REQUIRE(*line++ == "d(A_c3)/d(A_c3) = -0.3");
  REQUIRE(*line++ == "d(A_c3)/d(B_c3) = 0");
  REQUIRE(*line++ == "d(B_c3)/d(A_c3) = 0.3");
  REQUIRE(*line++ == "d(B_c3)/d(B_c3) = 0");
}
