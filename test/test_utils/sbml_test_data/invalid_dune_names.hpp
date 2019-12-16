#pragma once

namespace sbml_test_data {

struct invalid_dune_names {
  const char* xml = R"=====(<?xml version="1.0" encoding="UTF-8"?>
<sbml xmlns="http://www.sbml.org/sbml/level3/version2/core" xmlns:spatial="http://www.sbml.org/sbml/level3/version1/spatial/version1" level="3" version="2" spatial:required="true">
  <model metaid="COPASI0" id="New_Model" name="New Model" substanceUnits="substance" timeUnits="second" volumeUnits="volume" areaUnits="area" lengthUnits="metre" extentUnits="substance">
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
      <unitDefinition id="area">
        <listOfUnits>
          <unit kind="metre" exponent="2" scale="0" multiplier="1"/>
        </listOfUnits>
      </unitDefinition>
    </listOfUnitDefinitions>
    <listOfCompartments>
      <compartment metaid="COPASI1" id="comp" name="comp" spatialDimensions="3" size="1" units="volume" constant="true">
        <spatial:compartmentMapping spatial:id="comp_compartmentMapping" spatial:domainType="comp_domainType" spatial:unitSize="1"/>
      </compartment>
    </listOfCompartments>
    <listOfSpecies>
      <species metaid="COPASI2" id="dim" name="A" compartment="comp" initialConcentration="1" substanceUnits="substance" hasOnlySubstanceUnits="false" boundaryCondition="false" constant="false" spatial:isSpatial="true"/>
      <species metaid="COPASI3" id="x" name="B" compartment="comp" initialConcentration="1" substanceUnits="substance" hasOnlySubstanceUnits="false" boundaryCondition="false" constant="false" spatial:isSpatial="true"/>
      <species metaid="COPASI4" id="x_" name="C" compartment="comp" initialConcentration="0" substanceUnits="substance" hasOnlySubstanceUnits="false" boundaryCondition="false" constant="false" spatial:isSpatial="true"/>
    </listOfSpecies>
    <listOfParameters>
      <parameter id="dim_diffusionConstant" value="1" constant="true">
        <spatial:diffusionCoefficient spatial:variable="dim" spatial:type="isotropic"/>
      </parameter>
      <parameter id="x_diffusionConstant" value="1" constant="true">
        <spatial:diffusionCoefficient spatial:variable="x" spatial:type="isotropic"/>
      </parameter>
      <parameter id="x__diffusionConstant" value="1" constant="true">
        <spatial:diffusionCoefficient spatial:variable="x_" spatial:type="isotropic"/>
      </parameter>
    </listOfParameters>
    <listOfReactions>
      <reaction metaid="COPASI5" id="r1" name="r1" reversible="false" compartment="comp" spatial:isLocal="true">
        <listOfReactants>
          <speciesReference species="dim" stoichiometry="1" constant="true"/>
          <speciesReference species="x" stoichiometry="1" constant="true"/>
        </listOfReactants>
        <listOfProducts>
          <speciesReference species="x_" stoichiometry="1" constant="true"/>
        </listOfProducts>
        <kineticLaw>
          <math xmlns="http://www.w3.org/1998/Math/MathML">
            <apply>
              <times/>
              <ci> comp </ci>
              <ci> k1 </ci>
              <ci> dim </ci>
              <ci> x </ci>
            </apply>
          </math>
          <listOfLocalParameters>
            <localParameter id="k1" name="k1" value="0.1"/>
          </listOfLocalParameters>
        </kineticLaw>
      </reaction>
    </listOfReactions>
    <spatial:geometry spatial:coordinateSystem="cartesian">
      <spatial:listOfCoordinateComponents>
        <spatial:coordinateComponent spatial:id="xCoord" spatial:type="cartesianX">
          <spatial:boundaryMin spatial:id="xBoundaryMin" spatial:value="0"/>
          <spatial:boundaryMax spatial:id="xBoundaryMax" spatial:value="0"/>
        </spatial:coordinateComponent>
        <spatial:coordinateComponent spatial:id="yCoord" spatial:type="cartesianY">
          <spatial:boundaryMin spatial:id="yBoundaryMin" spatial:value="0"/>
          <spatial:boundaryMax spatial:id="yBoundaryMax" spatial:value="0"/>
        </spatial:coordinateComponent>
      </spatial:listOfCoordinateComponents>
      <spatial:listOfDomainTypes>
        <spatial:domainType spatial:id="comp_domainType" spatial:spatialDimensions="2"/>
      </spatial:listOfDomainTypes>
      <spatial:listOfDomains>
        <spatial:domain spatial:id="comp_domain" spatial:domainType="comp_domainType">
        </spatial:domain>
      </spatial:listOfDomains>
      <spatial:listOfGeometryDefinitions>
        <spatial:sampledFieldGeometry spatial:id="sampledFieldGeometry" spatial:isActive="true" spatial:sampledField="geometryImage">
          <spatial:listOfSampledVolumes>
            <spatial:sampledVolume spatial:id="comp_sampledVolume" spatial:domainType="comp_domainType" spatial:sampledValue="4294967295"/>
          </spatial:listOfSampledVolumes>
        </spatial:sampledFieldGeometry>
      </spatial:listOfGeometryDefinitions>
      <spatial:listOfSampledFields>
        <spatial:sampledField spatial:id="geometryImage" spatial:dataType="uint32" spatial:numSamples1="3" spatial:numSamples2="1" spatial:interpolationType="nearestNeighbor" spatial:compression="uncompressed" spatial:samplesLength="3">4294967295 4289374890 4283585106</spatial:sampledField>
      </spatial:listOfSampledFields>
    </spatial:geometry>
  </model>
</sbml>)=====";
};

}  // namespace sbml_test_data
