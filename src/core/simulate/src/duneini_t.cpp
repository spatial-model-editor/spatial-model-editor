#include "catch_wrapper.hpp"
#include "duneini.hpp"

SCENARIO("DUNE: ini files",
         "[core/simulate/duneini][core/simulate][core][duneini]") {
  GIVEN("IniFile class") {
    simulate::IniFile ini;
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
}
