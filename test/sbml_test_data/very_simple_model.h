#pragma once

#include <string>

namespace sbml_test_data {

struct very_simple_model {
  std::string xml() { return text; }
  const char* text =
      R"=====(<?xml version="1.0" encoding="UTF-8"?>
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
          <compartment metaid="COPASI1" id="c1" name="c1" spatialDimensions="3" size="10" constant="true">
          </compartment>
          <compartment metaid="COPASI2" id="c2" name="c2" spatialDimensions="3" size="1" constant="true">
          </compartment>
          <compartment metaid="COPASI3" id="c3" name="c3" spatialDimensions="3" size="0.2" constant="true">
          </compartment>
        </listOfCompartments>
        <listOfSpecies>
          <species metaid="COPASI4" id="A_c1" name="A" compartment="c1" initialConcentration="1" boundaryCondition="true" constant="true">
          </species>
          <species metaid="COPASI5" id="A_c2" name="A" compartment="c2" initialConcentration="0" boundaryCondition="false" constant="false">
          </species>
          <species metaid="COPASI6" id="A_c3" name="A" compartment="c3" initialConcentration="0" boundaryCondition="false" constant="false">
          </species>
          <species metaid="COPASI7" id="B_c3" name="B" compartment="c3" initialConcentration="0" boundaryCondition="false" constant="false">
          </species>
          <species metaid="COPASI8" id="B_c2" name="B" compartment="c2" initialConcentration="0" boundaryCondition="false" constant="false">
          </species>
          <species metaid="COPASI9" id="B_c1" name="B" compartment="c1" initialConcentration="0" boundaryCondition="false" constant="false">
          </species>
        </listOfSpecies>
        <listOfReactions>
          <reaction metaid="COPASI10" id="A_uptake" name="A_uptake" reversible="false">
            <listOfReactants>
              <speciesReference species="A_c1" stoichiometry="1"/>
            </listOfReactants>
            <listOfProducts>
              <speciesReference species="A_c2" stoichiometry="1"/>
            </listOfProducts>
            <kineticLaw>
              <math xmlns="http://www.w3.org/1998/Math/MathML">
                <apply>
                  <times/>
                  <ci> k1 </ci>
                  <ci> A_c1 </ci>
                </apply>
              </math>
              <listOfParameters>
                <parameter id="k1" name="k1" value="0.1"/>
              </listOfParameters>
            </kineticLaw>
          </reaction>
          <reaction metaid="COPASI11" id="A_transport" name="A_transport" reversible="true">
            <listOfReactants>
              <speciesReference species="A_c2" stoichiometry="1"/>
            </listOfReactants>
            <listOfProducts>
              <speciesReference species="A_c3" stoichiometry="1"/>
            </listOfProducts>
            <kineticLaw>
              <math xmlns="http://www.w3.org/1998/Math/MathML">
                <apply>
                  <minus/>
                  <apply>
                    <times/>
                    <ci> k1 </ci>
                    <ci> A_c2 </ci>
                  </apply>
                  <apply>
                    <times/>
                    <ci> k2 </ci>
                    <ci> A_c3 </ci>
                  </apply>
                </apply>
              </math>
              <listOfParameters>
                <parameter id="k1" name="k1" value="0.1"/>
                <parameter id="k2" name="k2" value="0.1"/>
              </listOfParameters>
            </kineticLaw>
          </reaction>
          <reaction metaid="COPASI12" id="A_B_conversion" name="A_B_conversion" reversible="false">
            <listOfReactants>
              <speciesReference species="A_c3" stoichiometry="1"/>
            </listOfReactants>
            <listOfProducts>
              <speciesReference species="B_c3" stoichiometry="1"/>
            </listOfProducts>
            <kineticLaw>
              <math xmlns="http://www.w3.org/1998/Math/MathML">
                <apply>
                  <times/>
                  <ci> c3 </ci>
                  <ci> k1 </ci>
                  <ci> A_c3 </ci>
                </apply>
              </math>
              <listOfParameters>
                <parameter id="k1" name="k1" value="0.3"/>
              </listOfParameters>
            </kineticLaw>
          </reaction>
          <reaction metaid="COPASI13" id="B_transport" name="B_transport" reversible="false">
            <listOfReactants>
              <speciesReference species="B_c3" stoichiometry="1"/>
            </listOfReactants>
            <listOfProducts>
              <speciesReference species="B_c2" stoichiometry="1"/>
            </listOfProducts>
            <kineticLaw>
              <math xmlns="http://www.w3.org/1998/Math/MathML">
                <apply>
                  <times/>
                  <ci> k1 </ci>
                  <ci> B_c3 </ci>
                </apply>
              </math>
              <listOfParameters>
                <parameter id="k1" name="k1" value="0.1"/>
              </listOfParameters>
            </kineticLaw>
          </reaction>
          <reaction metaid="COPASI14" id="B_excretion" name="B_excretion" reversible="false">
            <listOfReactants>
              <speciesReference species="B_c2" stoichiometry="1"/>
            </listOfReactants>
            <listOfProducts>
              <speciesReference species="B_c1" stoichiometry="1"/>
            </listOfProducts>
            <kineticLaw>
              <math xmlns="http://www.w3.org/1998/Math/MathML">
                <apply>
                  <times/>
                  <ci> k1 </ci>
                  <ci> B_c2 </ci>
                </apply>
              </math>
              <listOfParameters>
                <parameter id="k1" name="k1" value="0.2"/>
              </listOfParameters>
            </kineticLaw>
          </reaction>
        </listOfReactions>
      </model>
    </sbml>)=====";
};

}  // namespace sbml_test_data
