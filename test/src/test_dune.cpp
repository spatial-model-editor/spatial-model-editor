#include "catch.hpp"
#include "dune.hpp"

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

  ini.addValue("z", 3.14159);
  correct.append("z = 3.14159\n");
  REQUIRE(ini.getText() == correct);

  ini.addValue("dblExp", 2.7e-10);
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

  dune::DuneConverter dc(s);
  QStringList ini = dc.getIniFile().split("\n");
  REQUIRE(ini[10] == "[model]");
  REQUIRE(ini[14] == "[model.compartements]");
  REQUIRE(ini[15] == "comp = 0");
  REQUIRE(ini[16] == "");
  REQUIRE(ini[17] == "[model.comp]");
  REQUIRE(ini[18] == "name = comp");
  REQUIRE(ini[22] == "[model.comp.initial]");
  REQUIRE(ini[23] == "u_0 = 1");
  REQUIRE(ini[24] == "u_1 = 1");
  REQUIRE(ini[25] == "u_2 = 0");
  REQUIRE(ini[27] == "[model.comp.reaction]");
  REQUIRE(ini[28] == "u_0 = -0.1*u_1*u_0");
  REQUIRE(ini[29] == "u_1 = -0.1*u_1*u_0");
  REQUIRE(ini[30] == "u_2 = 0.1*u_1*u_0");
  REQUIRE(ini[32] == "[model.comp.reaction.jacobian]");
  REQUIRE(ini[33] == "d(u_0)/d(u_0) = -0.1*u_1");
  REQUIRE(ini[34] == "d(u_0)/d(u_1) = -0.1*u_0");
  REQUIRE(ini[35] == "d(u_0)/d(u_2) = 0");
  REQUIRE(ini[36] == "d(u_1)/d(u_0) = -0.1*u_1");
  REQUIRE(ini[37] == "d(u_1)/d(u_1) = -0.1*u_0");
  REQUIRE(ini[38] == "d(u_1)/d(u_2) = 0");
  REQUIRE(ini[39] == "d(u_2)/d(u_0) = 0.1*u_1");
  REQUIRE(ini[40] == "d(u_2)/d(u_1) = 0.1*u_0");
  REQUIRE(ini[41] == "d(u_2)/d(u_2) = 0");
  REQUIRE(ini[43] == "[model.comp.diffusion]");
  REQUIRE(ini[44] == "u_0 = 0.4");
  REQUIRE(ini[45] == "u_1 = 0.4");
  REQUIRE(ini[46] == "u_2 = 25");
}
