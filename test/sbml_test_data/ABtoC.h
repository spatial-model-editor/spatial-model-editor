#pragma once

namespace sbml_test_data {

extern const char* ABtoC;
const char* ABtoC = R"=====(<?xml version="1.0" encoding="UTF-8"?>
<!-- Created by COPASI version 4.12.65 (Source) on 2019-06-06 10:03 with libSBML version 5.18.0. -->
<sbml xmlns="http://www.sbml.org/sbml/level2/version4" level="2" version="4">
  <model metaid="COPASI0" id="New_Model" name="New Model">
    <annotation>
      <COPASI xmlns="http://www.copasi.org/static/sbml">
        <rdf:RDF xmlns:dcterms="http://purl.org/dc/terms/" xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#">
          <rdf:Description rdf:about="#COPASI0">
            <dcterms:created>
              <rdf:Description>
                <dcterms:W3CDTF>2019-06-06T08:00:18Z</dcterms:W3CDTF>
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
      <compartment metaid="COPASI1" id="comp" name="comp" spatialDimensions="3" size="1" constant="true">
        <annotation>
          <COPASI xmlns="http://www.copasi.org/static/sbml">
            <rdf:RDF xmlns:dcterms="http://purl.org/dc/terms/" xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#">
              <rdf:Description rdf:about="#COPASI1">
                <dcterms:created>
                  <rdf:Description>
                    <dcterms:W3CDTF>2019-06-06T08:00:22Z</dcterms:W3CDTF>
                  </rdf:Description>
                </dcterms:created>
              </rdf:Description>
            </rdf:RDF>
          </COPASI>
        </annotation>
      </compartment>
    </listOfCompartments>
    <listOfSpecies>
      <species metaid="COPASI2" id="A" name="A" compartment="comp" initialConcentration="1" boundaryCondition="false" constant="false">
        <annotation>
          <COPASI xmlns="http://www.copasi.org/static/sbml">
            <rdf:RDF xmlns:dcterms="http://purl.org/dc/terms/" xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#">
              <rdf:Description rdf:about="#COPASI2">
                <dcterms:created>
                  <rdf:Description>
                    <dcterms:W3CDTF>2019-06-06T08:00:25Z</dcterms:W3CDTF>
                  </rdf:Description>
                </dcterms:created>
              </rdf:Description>
            </rdf:RDF>
          </COPASI>
        </annotation>
      </species>
      <species metaid="COPASI3" id="B" name="B" compartment="comp" initialConcentration="1" boundaryCondition="false" constant="false">
        <annotation>
          <COPASI xmlns="http://www.copasi.org/static/sbml">
            <rdf:RDF xmlns:dcterms="http://purl.org/dc/terms/" xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#">
              <rdf:Description rdf:about="#COPASI3">
                <dcterms:created>
                  <rdf:Description>
                    <dcterms:W3CDTF>2019-06-06T08:00:26Z</dcterms:W3CDTF>
                  </rdf:Description>
                </dcterms:created>
              </rdf:Description>
            </rdf:RDF>
          </COPASI>
        </annotation>
      </species>
      <species metaid="COPASI4" id="C" name="C" compartment="comp" initialConcentration="0" boundaryCondition="false" constant="false">
        <annotation>
          <COPASI xmlns="http://www.copasi.org/static/sbml">
            <rdf:RDF xmlns:dcterms="http://purl.org/dc/terms/" xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#">
              <rdf:Description rdf:about="#COPASI4">
                <dcterms:created>
                  <rdf:Description>
                    <dcterms:W3CDTF>2019-06-06T08:00:27Z</dcterms:W3CDTF>
                  </rdf:Description>
                </dcterms:created>
              </rdf:Description>
            </rdf:RDF>
          </COPASI>
        </annotation>
      </species>
    </listOfSpecies>
    <listOfReactions>
      <reaction metaid="COPASI5" id="r1" name="r1" reversible="false">
        <annotation>
          <COPASI xmlns="http://www.copasi.org/static/sbml">
            <rdf:RDF xmlns:dcterms="http://purl.org/dc/terms/" xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#">
              <rdf:Description rdf:about="#COPASI5">
                <dcterms:created>
                  <rdf:Description>
                    <dcterms:W3CDTF>2019-06-06T08:00:37Z</dcterms:W3CDTF>
                  </rdf:Description>
                </dcterms:created>
              </rdf:Description>
            </rdf:RDF>
          </COPASI>
        </annotation>
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

}  // namespace sbml_test_data
