#pragma once

namespace sbml_test_data {

struct ABtoC {
  const char* xml = R"=====(<?xml version="1.0" encoding="UTF-8"?>
<sbml xmlns="http://www.sbml.org/sbml/level2/version4" level="2" version="4">
  <model metaid="COPASI0" id="New_Model" name="New Model">
    <listOfUnitDefinitions>
      <unitDefinition id="volume" name="volume">
        <listOfUnits>
          <unit kind="litre" exponent="1" scale="-3" multiplier="1"/>
        </listOfUnits>
      </unitDefinition>
      <unitDefinition id="substance" name="substance">
        <listOfUnits>
          <unit kind="mole" exponent="1" scale="-3" multiplier="1"/>
        </listOfUnits>
      </unitDefinition>
    </listOfUnitDefinitions>
    <listOfCompartments>
      <compartment metaid="COPASI1" id="comp" name="comp" spatialDimensions="3" size="1" constant="true">
      </compartment>
    </listOfCompartments>
    <listOfSpecies>
      <species metaid="COPASI2" id="A" name="A" compartment="comp" initialConcentration="1" boundaryCondition="false" constant="false">
      </species>
      <species metaid="COPASI3" id="B" name="B" compartment="comp" initialConcentration="1" boundaryCondition="false" constant="false">
      </species>
      <species metaid="COPASI4" id="C" name="C" compartment="comp" initialConcentration="0" boundaryCondition="false" constant="false">
      </species>
    </listOfSpecies>
    <listOfReactions>
      <reaction metaid="COPASI5" id="r1" name="r1" reversible="false">
        <listOfReactants>
          <speciesReference species="A" stoichiometry="1"/>
          <speciesReference species="B" stoichiometry="1"/>
        </listOfReactants>
        <listOfProducts>
          <speciesReference species="C" stoichiometry="1"/>
        </listOfProducts>
        <kineticLaw>
          <math xmlns="http://www.w3.org/1998/Math/MathML">
            <apply>
              <times/>
              <ci> comp </ci>
              <ci> k1 </ci>
              <ci> A </ci>
              <ci> B </ci>
            </apply>
          </math>
          <listOfParameters>
            <parameter id="k1" name="k1" value="0.1"/>
          </listOfParameters>
        </kineticLaw>
      </reaction>
    </listOfReactions>
  </model>
</sbml>)=====";
};

}  // namespace sbml_test_data
