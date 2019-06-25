#pragma once

namespace sbml_test_data {

extern const char* very_simple_model;
const char* very_simple_model = R"=====(<?xml version="1.0" encoding="UTF-8"?>
<!-- Created by COPASI version 4.12.65 (Source) on 2019-06-18 13:33 with libSBML version 5.18.0. -->
<sbml xmlns="http://www.sbml.org/sbml/level2/version4" level="2" version="4">
  <model metaid="COPASI0" id="New_Model" name="New Model">
    <annotation>
      <COPASI xmlns="http://www.copasi.org/static/sbml">
        <rdf:RDF xmlns:dcterms="http://purl.org/dc/terms/" xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#">
          <rdf:Description rdf:about="#COPASI0">
            <dcterms:created>
              <rdf:Description>
                <dcterms:W3CDTF>2019-06-17T12:50:04Z</dcterms:W3CDTF>
              </rdf:Description>
            </dcterms:created>
          </rdf:Description>
        </rdf:RDF>
      </COPASI>
    </annotation>
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
        <annotation>
          <COPASI xmlns="http://www.copasi.org/static/sbml">
            <rdf:RDF xmlns:dcterms="http://purl.org/dc/terms/" xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#">
              <rdf:Description rdf:about="#COPASI1">
                <dcterms:created>
                  <rdf:Description>
                    <dcterms:W3CDTF>2019-06-17T12:50:31Z</dcterms:W3CDTF>
                  </rdf:Description>
                </dcterms:created>
              </rdf:Description>
            </rdf:RDF>
          </COPASI>
        </annotation>
      </compartment>
      <compartment metaid="COPASI2" id="c2" name="c2" spatialDimensions="3" size="1" constant="true">
        <annotation>
          <COPASI xmlns="http://www.copasi.org/static/sbml">
            <rdf:RDF xmlns:dcterms="http://purl.org/dc/terms/" xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#">
              <rdf:Description rdf:about="#COPASI2">
                <dcterms:created>
                  <rdf:Description>
                    <dcterms:W3CDTF>2019-06-17T12:50:36Z</dcterms:W3CDTF>
                  </rdf:Description>
                </dcterms:created>
              </rdf:Description>
            </rdf:RDF>
          </COPASI>
        </annotation>
      </compartment>
      <compartment metaid="COPASI3" id="c3" name="c3" spatialDimensions="3" size="0.2" constant="true">
        <annotation>
          <COPASI xmlns="http://www.copasi.org/static/sbml">
            <rdf:RDF xmlns:dcterms="http://purl.org/dc/terms/" xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#">
              <rdf:Description rdf:about="#COPASI3">
                <dcterms:created>
                  <rdf:Description>
                    <dcterms:W3CDTF>2019-06-17T12:50:39Z</dcterms:W3CDTF>
                  </rdf:Description>
                </dcterms:created>
              </rdf:Description>
            </rdf:RDF>
          </COPASI>
        </annotation>
      </compartment>
    </listOfCompartments>
    <listOfSpecies>
      <species metaid="COPASI4" id="A_c1" name="A" compartment="c1" initialConcentration="1" boundaryCondition="true" constant="true">
        <annotation>
          <COPASI xmlns="http://www.copasi.org/static/sbml">
            <rdf:RDF xmlns:dcterms="http://purl.org/dc/terms/" xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#">
              <rdf:Description rdf:about="#COPASI4">
                <dcterms:created>
                  <rdf:Description>
                    <dcterms:W3CDTF>2019-06-17T12:51:47Z</dcterms:W3CDTF>
                  </rdf:Description>
                </dcterms:created>
              </rdf:Description>
            </rdf:RDF>
          </COPASI>
        </annotation>
      </species>
      <species metaid="COPASI5" id="A_c2" name="A" compartment="c2" initialConcentration="0" boundaryCondition="false" constant="false">
        <annotation>
          <COPASI xmlns="http://www.copasi.org/static/sbml">
            <rdf:RDF xmlns:dcterms="http://purl.org/dc/terms/" xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#">
              <rdf:Description rdf:about="#COPASI5">
                <dcterms:created>
                  <rdf:Description>
                    <dcterms:W3CDTF>2019-06-17T12:51:47Z</dcterms:W3CDTF>
                  </rdf:Description>
                </dcterms:created>
              </rdf:Description>
            </rdf:RDF>
          </COPASI>
        </annotation>
      </species>
      <species metaid="COPASI6" id="A_c3" name="A" compartment="c3" initialConcentration="0" boundaryCondition="false" constant="false">
        <annotation>
          <COPASI xmlns="http://www.copasi.org/static/sbml">
            <rdf:RDF xmlns:dcterms="http://purl.org/dc/terms/" xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#">
              <rdf:Description rdf:about="#COPASI6">
                <dcterms:created>
                  <rdf:Description>
                    <dcterms:W3CDTF>2019-06-17T12:52:07Z</dcterms:W3CDTF>
                  </rdf:Description>
                </dcterms:created>
              </rdf:Description>
            </rdf:RDF>
          </COPASI>
        </annotation>
      </species>
      <species metaid="COPASI7" id="B_c3" name="B" compartment="c3" initialConcentration="0" boundaryCondition="false" constant="false">
        <annotation>
          <COPASI xmlns="http://www.copasi.org/static/sbml">
            <rdf:RDF xmlns:dcterms="http://purl.org/dc/terms/" xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#">
              <rdf:Description rdf:about="#COPASI7">
                <dcterms:created>
                  <rdf:Description>
                    <dcterms:W3CDTF>2019-06-17T12:52:31Z</dcterms:W3CDTF>
                  </rdf:Description>
                </dcterms:created>
              </rdf:Description>
            </rdf:RDF>
          </COPASI>
        </annotation>
      </species>
      <species metaid="COPASI8" id="B_c2" name="B" compartment="c2" initialConcentration="0" boundaryCondition="false" constant="false">
        <annotation>
          <COPASI xmlns="http://www.copasi.org/static/sbml">
            <rdf:RDF xmlns:dcterms="http://purl.org/dc/terms/" xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#">
              <rdf:Description rdf:about="#COPASI8">
                <dcterms:created>
                  <rdf:Description>
                    <dcterms:W3CDTF>2019-06-17T12:53:54Z</dcterms:W3CDTF>
                  </rdf:Description>
                </dcterms:created>
              </rdf:Description>
            </rdf:RDF>
          </COPASI>
        </annotation>
      </species>
      <species metaid="COPASI9" id="B_c1" name="B" compartment="c1" initialConcentration="0" boundaryCondition="false" constant="false">
        <annotation>
          <COPASI xmlns="http://www.copasi.org/static/sbml">
            <rdf:RDF xmlns:dcterms="http://purl.org/dc/terms/" xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#">
              <rdf:Description rdf:about="#COPASI9">
                <dcterms:created>
                  <rdf:Description>
                    <dcterms:W3CDTF>2019-06-17T12:54:25Z</dcterms:W3CDTF>
                  </rdf:Description>
                </dcterms:created>
              </rdf:Description>
            </rdf:RDF>
          </COPASI>
        </annotation>
      </species>
    </listOfSpecies>
    <listOfReactions>
      <reaction metaid="COPASI10" id="A_uptake" name="A_uptake" reversible="false">
        <annotation>
          <COPASI xmlns="http://www.copasi.org/static/sbml">
            <rdf:RDF xmlns:dcterms="http://purl.org/dc/terms/" xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#">
              <rdf:Description rdf:about="#COPASI10">
                <dcterms:created>
                  <rdf:Description>
                    <dcterms:W3CDTF>2019-06-17T12:51:47Z</dcterms:W3CDTF>
                  </rdf:Description>
                </dcterms:created>
              </rdf:Description>
            </rdf:RDF>
          </COPASI>
        </annotation>
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
        <annotation>
          <COPASI xmlns="http://www.copasi.org/static/sbml">
            <rdf:RDF xmlns:dcterms="http://purl.org/dc/terms/" xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#">
              <rdf:Description rdf:about="#COPASI11">
                <dcterms:created>
                  <rdf:Description>
                    <dcterms:W3CDTF>2019-06-17T12:52:07Z</dcterms:W3CDTF>
                  </rdf:Description>
                </dcterms:created>
              </rdf:Description>
            </rdf:RDF>
          </COPASI>
        </annotation>
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
        <annotation>
          <COPASI xmlns="http://www.copasi.org/static/sbml">
            <rdf:RDF xmlns:dcterms="http://purl.org/dc/terms/" xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#">
              <rdf:Description rdf:about="#COPASI12">
                <dcterms:created>
                  <rdf:Description>
                    <dcterms:W3CDTF>2019-06-17T12:52:31Z</dcterms:W3CDTF>
                  </rdf:Description>
                </dcterms:created>
              </rdf:Description>
            </rdf:RDF>
          </COPASI>
        </annotation>
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
        <annotation>
          <COPASI xmlns="http://www.copasi.org/static/sbml">
            <rdf:RDF xmlns:dcterms="http://purl.org/dc/terms/" xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#">
              <rdf:Description rdf:about="#COPASI13">
                <dcterms:created>
                  <rdf:Description>
                    <dcterms:W3CDTF>2019-06-17T12:53:54Z</dcterms:W3CDTF>
                  </rdf:Description>
                </dcterms:created>
              </rdf:Description>
            </rdf:RDF>
          </COPASI>
        </annotation>
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
        <annotation>
          <COPASI xmlns="http://www.copasi.org/static/sbml">
            <rdf:RDF xmlns:dcterms="http://purl.org/dc/terms/" xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#">
              <rdf:Description rdf:about="#COPASI14">
                <dcterms:created>
                  <rdf:Description>
                    <dcterms:W3CDTF>2019-06-17T12:54:25Z</dcterms:W3CDTF>
                  </rdf:Description>
                </dcterms:created>
              </rdf:Description>
            </rdf:RDF>
          </COPASI>
        </annotation>
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

}  // namespace sbml_test_data
