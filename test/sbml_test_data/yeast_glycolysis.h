﻿#pragma once

namespace sbml_test_data {

extern const char* yeast_glycolysis;
const char* yeast_glycolysis = R"=====(<?xml version="1.0" encoding="UTF-8"?>
<sbml xmlns="http://www.sbml.org/sbml/level2/version4" level="2" version="4">
<model metaid="COPASI0" id="Yeast_glycolysis_model_of_Pritchard_and_Kell" name="Yeast glycolysis model of Pritchard and Kell">
<listOfFunctionDefinitions>
<functionDefinition metaid="COPASI46" id="HK_kinetics" name="HK kinetics">
<math xmlns="http://www.w3.org/1998/Math/MathML">
<lambda>
<bvar>
<ci> A </ci>
</bvar>
<bvar>
<ci> B </ci>
</bvar>
<bvar>
<ci> P </ci>
</bvar>
<bvar>
<ci> Q </ci>
</bvar>
<bvar>
<ci> Vmax </ci>
</bvar>
<bvar>
<ci> Kglc </ci>
</bvar>
<bvar>
<ci> Katp </ci>
</bvar>
<bvar>
<ci> Keq </ci>
</bvar>
<bvar>
<ci> Kg6p </ci>
</bvar>
<bvar>
<ci> Kadp </ci>
</bvar>
<apply>
<divide/>
<apply>
<times/>
<ci> Vmax </ci>
<apply>
<minus/>
<apply>
<divide/>
<apply>
  <times/>
  <ci> A </ci>
  <ci> B </ci>
</apply>
<apply>
  <times/>
  <ci> Kglc </ci>
  <ci> Katp </ci>
</apply>
</apply>
<apply>
<divide/>
<apply>
  <times/>
  <ci> P </ci>
  <ci> Q </ci>
</apply>
<apply>
  <times/>
  <ci> Kglc </ci>
  <ci> Katp </ci>
  <ci> Keq </ci>
</apply>
</apply>
</apply>
</apply>
<apply>
<times/>
<apply>
<plus/>
<cn> 1 </cn>
<apply>
<divide/>
<ci> A </ci>
<ci> Kglc </ci>
</apply>
<apply>
<divide/>
<ci> P </ci>
<ci> Kg6p </ci>
</apply>
</apply>
<apply>
<plus/>
<cn> 1 </cn>
<apply>
<divide/>
<ci> B </ci>
<ci> Katp </ci>
</apply>
<apply>
<divide/>
<ci> Q </ci>
<ci> Kadp </ci>
</apply>
</apply>
</apply>
</apply>
</lambda>
</math>
</functionDefinition>
<functionDefinition metaid="COPASI47" id="PGI_kinetics" name="PGI kinetics">
<math xmlns="http://www.w3.org/1998/Math/MathML">
<lambda>
<bvar>
<ci> A </ci>
</bvar>
<bvar>
<ci> P </ci>
</bvar>
<bvar>
<ci> Vmax </ci>
</bvar>
<bvar>
<ci> Kg6p </ci>
</bvar>
<bvar>
<ci> Keq </ci>
</bvar>
<bvar>
<ci> Kf6p </ci>
</bvar>
<apply>
<divide/>
<apply>
<times/>
<ci> Vmax </ci>
<apply>
<minus/>
<apply>
<divide/>
<ci> A </ci>
<ci> Kg6p </ci>
</apply>
<apply>
<divide/>
<ci> P </ci>
<apply>
  <times/>
  <ci> Kg6p </ci>
  <ci> Keq </ci>
</apply>
</apply>
</apply>
</apply>
<apply>
<plus/>
<cn> 1 </cn>
<apply>
<divide/>
<ci> A </ci>
<ci> Kg6p </ci>
</apply>
<apply>
<divide/>
<ci> P </ci>
<ci> Kf6p </ci>
</apply>
</apply>
</apply>
</lambda>
</math>
</functionDefinition>
<functionDefinition metaid="COPASI48" id="ALD_kinetics" name="ALD kinetics">
<math xmlns="http://www.w3.org/1998/Math/MathML">
<lambda>
<bvar>
<ci> A </ci>
</bvar>
<bvar>
<ci> P </ci>
</bvar>
<bvar>
<ci> Q </ci>
</bvar>
<bvar>
<ci> Vmax </ci>
</bvar>
<bvar>
<ci> Kf16bp </ci>
</bvar>
<bvar>
<ci> Keq </ci>
</bvar>
<bvar>
<ci> Kdhap </ci>
</bvar>
<bvar>
<ci> Kgap </ci>
</bvar>
<bvar>
<ci> Kigap </ci>
</bvar>
<apply>
<divide/>
<apply>
<times/>
<ci> Vmax </ci>
<apply>
<minus/>
<apply>
<divide/>
<ci> A </ci>
<ci> Kf16bp </ci>
</apply>
<apply>
<divide/>
<apply>
  <times/>
  <ci> P </ci>
  <ci> Q </ci>
</apply>
<apply>
  <times/>
  <ci> Kf16bp </ci>
  <ci> Keq </ci>
</apply>
</apply>
</apply>
</apply>
<apply>
<plus/>
<cn> 1 </cn>
<apply>
<divide/>
<ci> A </ci>
<ci> Kf16bp </ci>
</apply>
<apply>
<divide/>
<ci> P </ci>
<ci> Kdhap </ci>
</apply>
<apply>
<divide/>
<ci> Q </ci>
<ci> Kgap </ci>
</apply>
<apply>
<divide/>
<apply>
<times/>
<ci> A </ci>
<ci> Q </ci>
</apply>
<apply>
<times/>
<ci> Kf16bp </ci>
<ci> Kigap </ci>
</apply>
</apply>
<apply>
<divide/>
<apply>
<times/>
<ci> P </ci>
<ci> Q </ci>
</apply>
<apply>
<times/>
<ci> Kdhap </ci>
<ci> Kgap </ci>
</apply>
</apply>
</apply>
</apply>
</lambda>
</math>
</functionDefinition>
<functionDefinition metaid="COPASI49" id="PFK_kinetics" name="PFK kinetics">
<math xmlns="http://www.w3.org/1998/Math/MathML">
<lambda>
<bvar>
<ci> F6P </ci>
</bvar>
<bvar>
<ci> ATP </ci>
</bvar>
<bvar>
<ci> F16P </ci>
</bvar>
<bvar>
<ci> AMP </ci>
</bvar>
<bvar>
<ci> F26BP </ci>
</bvar>
<bvar>
<ci> Vmax </ci>
</bvar>
<bvar>
<ci> gR </ci>
</bvar>
<bvar>
<ci> Kf6p </ci>
</bvar>
<bvar>
<ci> Katp </ci>
</bvar>
<bvar>
<ci> L0 </ci>
</bvar>
<bvar>
<ci> Ciatp </ci>
</bvar>
<bvar>
<ci> Kiatp </ci>
</bvar>
<bvar>
<ci> Camp </ci>
</bvar>
<bvar>
<ci> Kamp </ci>
</bvar>
<bvar>
<ci> Cf26 </ci>
</bvar>
<bvar>
<ci> Kf26 </ci>
</bvar>
<bvar>
<ci> Cf16 </ci>
</bvar>
<bvar>
<ci> Kf16 </ci>
</bvar>
<bvar>
<ci> Catp </ci>
</bvar>
<apply>
<times/>
<ci> Vmax </ci>
<apply>
<divide/>
<apply>
<times/>
<ci> gR </ci>
<apply>
<divide/>
<ci> F6P </ci>
<ci> Kf6p </ci>
</apply>
<apply>
<divide/>
<ci> ATP </ci>
<ci> Katp </ci>
</apply>
<apply>
<plus/>
<cn> 1 </cn>
<apply>
  <divide/>
  <ci> F6P </ci>
  <ci> Kf6p </ci>
</apply>
<apply>
  <divide/>
  <ci> ATP </ci>
  <ci> Katp </ci>
</apply>
<apply>
  <divide/>
  <apply>
    <times/>
    <apply>
      <divide/>
      <apply>
        <times/>
        <ci> gR </ci>
        <ci> F6P </ci>
      </apply>
      <ci> Kf6p </ci>
    </apply>
    <ci> ATP </ci>
  </apply>
  <ci> Katp </ci>
</apply>
</apply>
</apply>
<apply>
<plus/>
<apply>
<power/>
<apply>
  <plus/>
  <cn> 1 </cn>
  <apply>
    <divide/>
    <ci> F6P </ci>
    <ci> Kf6p </ci>
  </apply>
  <apply>
    <divide/>
    <ci> ATP </ci>
    <ci> Katp </ci>
  </apply>
  <apply>
    <divide/>
    <apply>
      <times/>
      <apply>
        <divide/>
        <apply>
          <times/>
          <ci> gR </ci>
          <ci> F6P </ci>
        </apply>
        <ci> Kf6p </ci>
      </apply>
      <ci> ATP </ci>
    </apply>
    <ci> Katp </ci>
  </apply>
</apply>
<cn> 2 </cn>
</apply>
<apply>
<times/>
<ci> L0 </ci>
<apply>
  <power/>
  <apply>
    <divide/>
    <apply>
      <plus/>
      <cn> 1 </cn>
      <apply>
        <divide/>
        <apply>
          <times/>
          <ci> Ciatp </ci>
          <ci> ATP </ci>
        </apply>
        <ci> Kiatp </ci>
      </apply>
    </apply>
    <apply>
      <plus/>
      <cn> 1 </cn>
      <apply>
        <divide/>
        <ci> ATP </ci>
        <ci> Kiatp </ci>
      </apply>
    </apply>
  </apply>
  <cn> 2 </cn>
</apply>
<apply>
  <power/>
  <apply>
    <divide/>
    <apply>
      <plus/>
      <cn> 1 </cn>
      <apply>
        <divide/>
        <apply>
          <times/>
          <ci> Camp </ci>
          <ci> AMP </ci>
        </apply>
        <ci> Kamp </ci>
      </apply>
    </apply>
    <apply>
      <plus/>
      <cn> 1 </cn>
      <apply>
        <divide/>
        <ci> AMP </ci>
        <ci> Kamp </ci>
      </apply>
    </apply>
  </apply>
  <cn> 2 </cn>
</apply>
<apply>
  <power/>
  <apply>
    <divide/>
    <apply>
      <plus/>
      <cn> 1 </cn>
      <apply>
        <divide/>
        <apply>
          <times/>
          <ci> Cf26 </ci>
          <ci> F26BP </ci>
        </apply>
        <ci> Kf26 </ci>
      </apply>
      <apply>
        <divide/>
        <apply>
          <times/>
          <ci> Cf16 </ci>
          <ci> F16P </ci>
        </apply>
        <ci> Kf16 </ci>
      </apply>
    </apply>
    <apply>
      <plus/>
      <cn> 1 </cn>
      <apply>
        <divide/>
        <ci> F26BP </ci>
        <ci> Kf26 </ci>
      </apply>
      <apply>
        <divide/>
        <ci> F16P </ci>
        <ci> Kf16 </ci>
      </apply>
    </apply>
  </apply>
  <cn> 2 </cn>
</apply>
<apply>
  <power/>
  <apply>
    <plus/>
    <cn> 1 </cn>
    <apply>
      <divide/>
      <apply>
        <times/>
        <ci> Catp </ci>
        <ci> ATP </ci>
      </apply>
      <ci> Katp </ci>
    </apply>
  </apply>
  <cn> 2 </cn>
</apply>
</apply>
</apply>
</apply>
</apply>
</lambda>
</math>
</functionDefinition>
<functionDefinition metaid="COPASI50" id="GAPDH_kinetics" name="GAPDH kinetics">
<math xmlns="http://www.w3.org/1998/Math/MathML">
<lambda>
<bvar>
<ci> A </ci>
</bvar>
<bvar>
<ci> B </ci>
</bvar>
<bvar>
<ci> P </ci>
</bvar>
<bvar>
<ci> Q </ci>
</bvar>
<bvar>
<ci> C </ci>
</bvar>
<bvar>
<ci> Vmaxf </ci>
</bvar>
<bvar>
<ci> Kgap </ci>
</bvar>
<bvar>
<ci> Knad </ci>
</bvar>
<bvar>
<ci> Vmaxr </ci>
</bvar>
<bvar>
<ci> Kbpg </ci>
</bvar>
<bvar>
<ci> Knadh </ci>
</bvar>
<apply>
<divide/>
<apply>
<times/>
<ci> C </ci>
<apply>
<minus/>
<apply>
<divide/>
<apply>
  <times/>
  <ci> Vmaxf </ci>
  <ci> A </ci>
  <ci> B </ci>
</apply>
<apply>
  <times/>
  <ci> Kgap </ci>
  <ci> Knad </ci>
</apply>
</apply>
<apply>
<divide/>
<apply>
  <times/>
  <ci> Vmaxr </ci>
  <ci> P </ci>
  <ci> Q </ci>
</apply>
<apply>
  <times/>
  <ci> Kbpg </ci>
  <ci> Knadh </ci>
</apply>
</apply>
</apply>
</apply>
<apply>
<times/>
<apply>
<plus/>
<cn> 1 </cn>
<apply>
<divide/>
<ci> A </ci>
<ci> Kgap </ci>
</apply>
<apply>
<divide/>
<ci> P </ci>
<ci> Kbpg </ci>
</apply>
</apply>
<apply>
<plus/>
<cn> 1 </cn>
<apply>
<divide/>
<ci> B </ci>
<ci> Knad </ci>
</apply>
<apply>
<divide/>
<ci> Q </ci>
<ci> Knadh </ci>
</apply>
</apply>
</apply>
</apply>
</lambda>
</math>
</functionDefinition>
<functionDefinition metaid="COPASI51" id="PGK_kinetics" name="PGK kinetics">
<math xmlns="http://www.w3.org/1998/Math/MathML">
<lambda>
<bvar>
<ci> A </ci>
</bvar>
<bvar>
<ci> B </ci>
</bvar>
<bvar>
<ci> P </ci>
</bvar>
<bvar>
<ci> Q </ci>
</bvar>
<bvar>
<ci> Vmax </ci>
</bvar>
<bvar>
<ci> Keq </ci>
</bvar>
<bvar>
<ci> Kp3g </ci>
</bvar>
<bvar>
<ci> Katp </ci>
</bvar>
<bvar>
<ci> Kbpg </ci>
</bvar>
<bvar>
<ci> Kadp </ci>
</bvar>
<apply>
<divide/>
<apply>
<times/>
<ci> Vmax </ci>
<apply>
<divide/>
<apply>
<minus/>
<apply>
  <times/>
  <ci> Keq </ci>
  <ci> A </ci>
  <ci> B </ci>
</apply>
<apply>
  <times/>
  <ci> P </ci>
  <ci> Q </ci>
</apply>
</apply>
<apply>
<times/>
<ci> Kp3g </ci>
<ci> Katp </ci>
</apply>
</apply>
</apply>
<apply>
<times/>
<apply>
<plus/>
<cn> 1 </cn>
<apply>
<divide/>
<ci> A </ci>
<ci> Kbpg </ci>
</apply>
<apply>
<divide/>
<ci> P </ci>
<ci> Kp3g </ci>
</apply>
</apply>
<apply>
<plus/>
<cn> 1 </cn>
<apply>
<divide/>
<ci> B </ci>
<ci> Kadp </ci>
</apply>
<apply>
<divide/>
<ci> Q </ci>
<ci> Katp </ci>
</apply>
</apply>
</apply>
</apply>
</lambda>
</math>
</functionDefinition>
<functionDefinition metaid="COPASI52" id="PGM_kinetics" name="PGM kinetics">
<math xmlns="http://www.w3.org/1998/Math/MathML">
<lambda>
<bvar>
<ci> A </ci>
</bvar>
<bvar>
<ci> P </ci>
</bvar>
<bvar>
<ci> Vmax </ci>
</bvar>
<bvar>
<ci> Kp3g </ci>
</bvar>
<bvar>
<ci> Keq </ci>
</bvar>
<bvar>
<ci> Kp2g </ci>
</bvar>
<apply>
<divide/>
<apply>
<times/>
<ci> Vmax </ci>
<apply>
<minus/>
<apply>
<divide/>
<ci> A </ci>
<ci> Kp3g </ci>
</apply>
<apply>
<divide/>
<ci> P </ci>
<apply>
  <times/>
  <ci> Kp3g </ci>
  <ci> Keq </ci>
</apply>
</apply>
</apply>
</apply>
<apply>
<plus/>
<cn> 1 </cn>
<apply>
<divide/>
<ci> A </ci>
<ci> Kp3g </ci>
</apply>
<apply>
<divide/>
<ci> P </ci>
<ci> Kp2g </ci>
</apply>
</apply>
</apply>
</lambda>
</math>
</functionDefinition>
<functionDefinition metaid="COPASI53" id="ENO_kinetics" name="ENO kinetics">
<math xmlns="http://www.w3.org/1998/Math/MathML">
<lambda>
<bvar>
<ci> A </ci>
</bvar>
<bvar>
<ci> P </ci>
</bvar>
<bvar>
<ci> Vmax </ci>
</bvar>
<bvar>
<ci> Kp2g </ci>
</bvar>
<bvar>
<ci> Keq </ci>
</bvar>
<bvar>
<ci> Kpep </ci>
</bvar>
<apply>
<divide/>
<apply>
<times/>
<ci> Vmax </ci>
<apply>
<minus/>
<apply>
<divide/>
<ci> A </ci>
<ci> Kp2g </ci>
</apply>
<apply>
<divide/>
<ci> P </ci>
<apply>
  <times/>
  <ci> Kp2g </ci>
  <ci> Keq </ci>
</apply>
</apply>
</apply>
</apply>
<apply>
<plus/>
<cn> 1 </cn>
<apply>
<divide/>
<ci> A </ci>
<ci> Kp2g </ci>
</apply>
<apply>
<divide/>
<ci> P </ci>
<ci> Kpep </ci>
</apply>
</apply>
</apply>
</lambda>
</math>
</functionDefinition>
<functionDefinition metaid="COPASI54" id="PDC_kinetics" name="PDC kinetics">
<math xmlns="http://www.w3.org/1998/Math/MathML">
<lambda>
<bvar>
<ci> A </ci>
</bvar>
<bvar>
<ci> Vmax </ci>
</bvar>
<bvar>
<ci> Kpyr </ci>
</bvar>
<bvar>
<ci> nH </ci>
</bvar>
<apply>
<divide/>
<apply>
<times/>
<ci> Vmax </ci>
<apply>
<power/>
<apply>
<divide/>
<ci> A </ci>
<ci> Kpyr </ci>
</apply>
<ci> nH </ci>
</apply>
</apply>
<apply>
<plus/>
<cn> 1 </cn>
<apply>
<power/>
<apply>
<divide/>
<ci> A </ci>
<ci> Kpyr </ci>
</apply>
<ci> nH </ci>
</apply>
</apply>
</apply>
</lambda>
</math>
</functionDefinition>
<functionDefinition metaid="COPASI55" id="PYK_kinetics" name="PYK kinetics">
<math xmlns="http://www.w3.org/1998/Math/MathML">
<lambda>
<bvar>
<ci> A </ci>
</bvar>
<bvar>
<ci> B </ci>
</bvar>
<bvar>
<ci> P </ci>
</bvar>
<bvar>
<ci> Q </ci>
</bvar>
<bvar>
<ci> Vmax </ci>
</bvar>
<bvar>
<ci> Kpep </ci>
</bvar>
<bvar>
<ci> Kadp </ci>
</bvar>
<bvar>
<ci> Keq </ci>
</bvar>
<bvar>
<ci> Kpyr </ci>
</bvar>
<bvar>
<ci> Katp </ci>
</bvar>
<apply>
<divide/>
<apply>
<times/>
<ci> Vmax </ci>
<apply>
<minus/>
<apply>
<divide/>
<apply>
  <times/>
  <ci> A </ci>
  <ci> B </ci>
</apply>
<apply>
  <times/>
  <ci> Kpep </ci>
  <ci> Kadp </ci>
</apply>
</apply>
<apply>
<divide/>
<apply>
  <times/>
  <ci> P </ci>
  <ci> Q </ci>
</apply>
<apply>
  <times/>
  <ci> Kpep </ci>
  <ci> Kadp </ci>
  <ci> Keq </ci>
</apply>
</apply>
</apply>
</apply>
<apply>
<times/>
<apply>
<plus/>
<cn> 1 </cn>
<apply>
<divide/>
<ci> A </ci>
<ci> Kpep </ci>
</apply>
<apply>
<divide/>
<ci> P </ci>
<ci> Kpyr </ci>
</apply>
</apply>
<apply>
<plus/>
<cn> 1 </cn>
<apply>
<divide/>
<ci> B </ci>
<ci> Kadp </ci>
</apply>
<apply>
<divide/>
<ci> Q </ci>
<ci> Katp </ci>
</apply>
</apply>
</apply>
</apply>
</lambda>
</math>
</functionDefinition>
<functionDefinition metaid="COPASI56" id="ATPase_0" name="ATPase">
<math xmlns="http://www.w3.org/1998/Math/MathML">
<lambda>
<bvar>
<ci> ATP </ci>
</bvar>
<bvar>
<ci> Katpase </ci>
</bvar>
<apply>
<times/>
<ci> Katpase </ci>
<ci> ATP </ci>
</apply>
</lambda>
</math>
</functionDefinition>
<functionDefinition metaid="COPASI57" id="HXT_kinetics" name="HXT kinetics">
<math xmlns="http://www.w3.org/1998/Math/MathML">
<lambda>
<bvar>
<ci> A </ci>
</bvar>
<bvar>
<ci> B </ci>
</bvar>
<bvar>
<ci> Vmax </ci>
</bvar>
<bvar>
<ci> Kglc </ci>
</bvar>
<bvar>
<ci> Ki </ci>
</bvar>
<apply>
<divide/>
<apply>
<divide/>
<apply>
<times/>
<ci> Vmax </ci>
<apply>
<minus/>
<ci> A </ci>
<ci> B </ci>
</apply>
</apply>
<ci> Kglc </ci>
</apply>
<apply>
<plus/>
<cn> 1 </cn>
<apply>
<divide/>
<apply>
<plus/>
<ci> A </ci>
<ci> B </ci>
</apply>
<ci> Kglc </ci>
</apply>
<apply>
<divide/>
<apply>
<times/>
<ci> Ki </ci>
<ci> A </ci>
<ci> B </ci>
</apply>
<apply>
<power/>
<ci> Kglc </ci>
<cn> 2 </cn>
</apply>
</apply>
</apply>
</apply>
</lambda>
</math>
</functionDefinition>
<functionDefinition metaid="COPASI58" id="ADH_kinetics" name="ADH kinetics">
<math xmlns="http://www.w3.org/1998/Math/MathML">
<lambda>
<bvar>
<ci> A </ci>
</bvar>
<bvar>
<ci> B </ci>
</bvar>
<bvar>
<ci> P </ci>
</bvar>
<bvar>
<ci> Q </ci>
</bvar>
<bvar>
<ci> Vmax </ci>
</bvar>
<bvar>
<ci> Ketoh </ci>
</bvar>
<bvar>
<ci> Kinad </ci>
</bvar>
<bvar>
<ci> Keq </ci>
</bvar>
<bvar>
<ci> Knad </ci>
</bvar>
<bvar>
<ci> Knadh </ci>
</bvar>
<bvar>
<ci> Kinadh </ci>
</bvar>
<bvar>
<ci> Kacald </ci>
</bvar>
<bvar>
<ci> Kiacald </ci>
</bvar>
<bvar>
<ci> Kietoh </ci>
</bvar>
<apply>
<divide/>
<apply>
<times/>
<ci> Vmax </ci>
<apply>
<minus/>
<apply>
<divide/>
<apply>
  <times/>
  <ci> A </ci>
  <ci> B </ci>
</apply>
<apply>
  <times/>
  <ci> Ketoh </ci>
  <ci> Kinad </ci>
</apply>
</apply>
<apply>
<divide/>
<apply>
  <times/>
  <ci> P </ci>
  <ci> Q </ci>
</apply>
<apply>
  <times/>
  <ci> Ketoh </ci>
  <ci> Kinad </ci>
  <ci> Keq </ci>
</apply>
</apply>
</apply>
</apply>
<apply>
<plus/>
<cn> 1 </cn>
<apply>
<divide/>
<ci> B </ci>
<ci> Kinad </ci>
</apply>
<apply>
<divide/>
<apply>
<times/>
<ci> A </ci>
<ci> Knad </ci>
</apply>
<apply>
<times/>
<ci> Kinad </ci>
<ci> Ketoh </ci>
</apply>
</apply>
<apply>
<divide/>
<apply>
<times/>
<ci> P </ci>
<ci> Knadh </ci>
</apply>
<apply>
<times/>
<ci> Kinadh </ci>
<ci> Kacald </ci>
</apply>
</apply>
<apply>
<divide/>
<ci> Q </ci>
<ci> Kinadh </ci>
</apply>
<apply>
<divide/>
<apply>
<times/>
<ci> A </ci>
<ci> B </ci>
</apply>
<apply>
<times/>
<ci> Kinad </ci>
<ci> Ketoh </ci>
</apply>
</apply>
<apply>
<divide/>
<apply>
<times/>
<ci> B </ci>
<ci> P </ci>
<ci> Knadh </ci>
</apply>
<apply>
<times/>
<ci> Kinad </ci>
<ci> Kinadh </ci>
<ci> Kacald </ci>
</apply>
</apply>
<apply>
<divide/>
<apply>
<times/>
<ci> A </ci>
<ci> Q </ci>
<ci> Knad </ci>
</apply>
<apply>
<times/>
<ci> Kinad </ci>
<ci> Kinadh </ci>
<ci> Ketoh </ci>
</apply>
</apply>
<apply>
<divide/>
<apply>
<times/>
<ci> P </ci>
<ci> Q </ci>
</apply>
<apply>
<times/>
<ci> Kacald </ci>
<ci> Kinadh </ci>
</apply>
</apply>
<apply>
<divide/>
<apply>
<times/>
<ci> A </ci>
<ci> B </ci>
<ci> P </ci>
</apply>
<apply>
<times/>
<ci> Kinad </ci>
<ci> Kiacald </ci>
<ci> Ketoh </ci>
</apply>
</apply>
<apply>
<divide/>
<apply>
<times/>
<ci> A </ci>
<ci> P </ci>
<ci> Q </ci>
</apply>
<apply>
<times/>
<ci> Kietoh </ci>
<ci> Kinadh </ci>
<ci> Kacald </ci>
</apply>
</apply>
</apply>
</apply>
</lambda>
</math>
</functionDefinition>
<functionDefinition metaid="COPASI59" id="G3PDH_kinetics" name="G3PDH kinetics">
<math xmlns="http://www.w3.org/1998/Math/MathML">
<lambda>
<bvar>
<ci> A </ci>
</bvar>
<bvar>
<ci> B </ci>
</bvar>
<bvar>
<ci> P </ci>
</bvar>
<bvar>
<ci> Q </ci>
</bvar>
<bvar>
<ci> Vmax </ci>
</bvar>
<bvar>
<ci> Kdhap </ci>
</bvar>
<bvar>
<ci> Knadh </ci>
</bvar>
<bvar>
<ci> Keq </ci>
</bvar>
<bvar>
<ci> Kglycerol </ci>
</bvar>
<bvar>
<ci> Knad </ci>
</bvar>
<apply>
<divide/>
<apply>
<times/>
<ci> Vmax </ci>
<apply>
<minus/>
<apply>
<times/>
<apply>
  <divide/>
  <ci> A </ci>
  <ci> Kdhap </ci>
</apply>
<apply>
  <divide/>
  <ci> B </ci>
  <ci> Knadh </ci>
</apply>
</apply>
<apply>
<times/>
<apply>
  <divide/>
  <ci> P </ci>
  <ci> Kdhap </ci>
</apply>
<apply>
  <divide/>
  <ci> Q </ci>
  <ci> Knadh </ci>
</apply>
<apply>
  <divide/>
  <cn> 1 </cn>
  <ci> Keq </ci>
</apply>
</apply>
</apply>
</apply>
<apply>
<times/>
<apply>
<plus/>
<cn> 1 </cn>
<apply>
<divide/>
<ci> A </ci>
<ci> Kdhap </ci>
</apply>
<apply>
<divide/>
<ci> P </ci>
<ci> Kglycerol </ci>
</apply>
</apply>
<apply>
<plus/>
<cn> 1 </cn>
<apply>
<divide/>
<ci> B </ci>
<ci> Knadh </ci>
</apply>
<apply>
<divide/>
<ci> Q </ci>
<ci> Knad </ci>
</apply>
</apply>
</apply>
</apply>
</lambda>
</math>
</functionDefinition>
<functionDefinition metaid="COPASI60" id="Glycogen_synthesis_kinetics" name="Glycogen synthesis kinetics">
<math xmlns="http://www.w3.org/1998/Math/MathML">
<lambda>
<bvar>
<ci> KGLYCOGEN </ci>
</bvar>
<ci> KGLYCOGEN </ci>
</lambda>
</math>
</functionDefinition>
<functionDefinition metaid="COPASI61" id="Trehalose_synthesis_kinetics" name="Trehalose synthesis kinetics">
<math xmlns="http://www.w3.org/1998/Math/MathML">
<lambda>
<bvar>
<ci> Ktrehalose </ci>
</bvar>
<ci> Ktrehalose </ci>
</lambda>
</math>
</functionDefinition>
<functionDefinition metaid="COPASI62" id="Succinate_kinetics" name="Succinate kinetics">
<math xmlns="http://www.w3.org/1998/Math/MathML">
<lambda>
<bvar>
<ci> A </ci>
</bvar>
<bvar>
<ci> k </ci>
</bvar>
<apply>
<times/>
<ci> k </ci>
<ci> A </ci>
</apply>
</lambda>
</math>
</functionDefinition>
</listOfFunctionDefinitions>
<listOfUnitDefinitions>
<unitDefinition id="volume" name="volume">
<listOfUnits>
<unit kind="litre" exponent="1" scale="-3" multiplier="1"/>
</listOfUnits>
</unitDefinition>
<unitDefinition id="time" name="time">
<listOfUnits>
<unit kind="second" exponent="1" scale="0" multiplier="60"/>
</listOfUnits>
</unitDefinition>
<unitDefinition id="substance" name="substance">
<listOfUnits>
<unit kind="mole" exponent="1" scale="-3" multiplier="1"/>
</listOfUnits>
</unitDefinition>
</listOfUnitDefinitions>
<listOfCompartments>
<compartment metaid="COPASI1" id="compartment" name="compartment" spatialDimensions="3" size="1" constant="true">
</compartment>
</listOfCompartments>
<listOfSpecies>
<species metaid="COPASI2" id="GLCo" name="GLCo" compartment="compartment" initialConcentration="2" boundaryCondition="true" constant="true">
</species>
<species metaid="COPASI3" id="GLCi" name="GLCi" compartment="compartment" initialConcentration="0.097652231064563" boundaryCondition="false" constant="false">
</species>
<species metaid="COPASI4" id="ATP" name="ATP" compartment="compartment" initialConcentration="2.52512746499271" boundaryCondition="false" constant="false">
</species>
<species metaid="COPASI5" id="G6P" name="G6P" compartment="compartment" initialConcentration="2.67504014044787" boundaryCondition="false" constant="false">
</species>
<species metaid="COPASI6" id="ADP" name="ADP" compartment="compartment" initialConcentration="1.28198768168719" boundaryCondition="false" constant="false">
</species>
<species metaid="COPASI7" id="F6P" name="F6P" compartment="compartment" initialConcentration="0.624976405532373" boundaryCondition="false" constant="false">
</species>
<species metaid="COPASI8" id="F16bP" name="F16bP" compartment="compartment" initialConcentration="6.22132076069411" boundaryCondition="false" constant="false">
</species>
<species metaid="COPASI9" id="AMP" name="AMP" compartment="compartment" initialConcentration="0.292884853320091" boundaryCondition="false" constant="false">
</species>
<species metaid="COPASI10" id="F26bP" name="F26bP" compartment="compartment" initialConcentration="0.02" boundaryCondition="true" constant="true">
</species>
<species metaid="COPASI11" id="DHAP" name="DHAP" compartment="compartment" initialConcentration="1.00415254899644" boundaryCondition="false" constant="false">
</species>
<species metaid="COPASI12" id="GAP" name="GAP" compartment="compartment" initialConcentration="0.0451809175780963" boundaryCondition="false" constant="false">
</species>
<species metaid="COPASI13" id="NAD" name="NAD" compartment="compartment" initialConcentration="1.50329030201531" boundaryCondition="false" constant="false">
</species>
<species metaid="COPASI14" id="BPG" name="BPG" compartment="compartment" initialConcentration="0.000736873499865602" boundaryCondition="false" constant="false">
</species>
<species metaid="COPASI15" id="NADH" name="NADH" compartment="compartment" initialConcentration="0.0867096979846952" boundaryCondition="false" constant="false">
</species>
<species metaid="COPASI16" id="P3G" name="P3G" compartment="compartment" initialConcentration="0.885688538360659" boundaryCondition="false" constant="false">
</species>
<species metaid="COPASI17" id="P2G" name="P2G" compartment="compartment" initialConcentration="0.127695817386632" boundaryCondition="false" constant="false">
</species>
<species metaid="COPASI18" id="PEP" name="PEP" compartment="compartment" initialConcentration="0.0632352144936527" boundaryCondition="false" constant="false">
</species>
<species metaid="COPASI19" id="PYR" name="PYR" compartment="compartment" initialConcentration="1.81531251192736" boundaryCondition="false" constant="false">
</species>
<species metaid="COPASI20" id="AcAld" name="AcAld" compartment="compartment" initialConcentration="0.178140579850657" boundaryCondition="false" constant="false">
</species>
<species metaid="COPASI21" id="CO2" name="CO2" compartment="compartment" initialConcentration="1" boundaryCondition="true" constant="true">
</species>
<species metaid="COPASI22" id="EtOH" name="EtOH" compartment="compartment" initialConcentration="50" boundaryCondition="true" constant="true">
</species>
<species metaid="COPASI23" id="Glycerol" name="Glycerol" compartment="compartment" initialConcentration="0.15" boundaryCondition="true" constant="true">
</species>
<species metaid="COPASI24" id="Glycogen" name="Glycogen" compartment="compartment" initialConcentration="0" boundaryCondition="true" constant="true">
</species>
<species metaid="COPASI25" id="Trehalose" name="Trehalose" compartment="compartment" initialConcentration="0" boundaryCondition="true" constant="true">
</species>
<species metaid="COPASI26" id="Succinate" name="Succinate" compartment="compartment" initialConcentration="0" boundaryCondition="true" constant="true">
</species>
</listOfSpecies>
<listOfReactions>
<reaction metaid="COPASI27" id="HXT" name="HXT" reversible="true">
<listOfReactants>
<speciesReference species="GLCo" stoichiometry="1"/>
</listOfReactants>
<listOfProducts>
<speciesReference species="GLCi" stoichiometry="1"/>
</listOfProducts>
<kineticLaw>
<math xmlns="http://www.w3.org/1998/Math/MathML">
<apply>
<times/>
<ci> compartment </ci>
<apply>
<ci> HXT_kinetics </ci>
<ci> GLCo </ci>
<ci> GLCi </ci>
<ci> Vmax </ci>
<ci> Kglc </ci>
<ci> Ki </ci>
</apply>
</apply>
</math>
<listOfParameters>
<parameter id="Vmax" name="Vmax" value="97.24"/>
<parameter id="Kglc" name="Kglc" value="1.1918"/>
<parameter id="Ki" name="Ki" value="0.91"/>
</listOfParameters>
</kineticLaw>
</reaction>
<reaction metaid="COPASI28" id="HK" name="HK" reversible="true">
<listOfReactants>
<speciesReference species="GLCi" stoichiometry="1"/>
<speciesReference species="ATP" stoichiometry="1"/>
</listOfReactants>
<listOfProducts>
<speciesReference species="G6P" stoichiometry="1"/>
<speciesReference species="ADP" stoichiometry="1"/>
</listOfProducts>
<kineticLaw>
<math xmlns="http://www.w3.org/1998/Math/MathML">
<apply>
<times/>
<ci> compartment </ci>
<apply>
<ci> HK_kinetics </ci>
<ci> GLCi </ci>
<ci> ATP </ci>
<ci> G6P </ci>
<ci> ADP </ci>
<ci> Vmax </ci>
<ci> Kglc </ci>
<ci> Katp </ci>
<ci> Keq </ci>
<ci> Kg6p </ci>
<ci> Kadp </ci>
</apply>
</apply>
</math>
<listOfParameters>
<parameter id="Vmax" name="Vmax" value="236.7"/>
<parameter id="Kglc" name="Kglc" value="0.08"/>
<parameter id="Katp" name="Katp" value="0.15"/>
<parameter id="Keq" name="Keq" value="2000"/>
<parameter id="Kg6p" name="Kg6p" value="30"/>
<parameter id="Kadp" name="Kadp" value="0.23"/>
</listOfParameters>
</kineticLaw>
</reaction>
<reaction metaid="COPASI29" id="PGI" name="PGI" reversible="true">
<listOfReactants>
<speciesReference species="G6P" stoichiometry="1"/>
</listOfReactants>
<listOfProducts>
<speciesReference species="F6P" stoichiometry="1"/>
</listOfProducts>
<kineticLaw>
<math xmlns="http://www.w3.org/1998/Math/MathML">
<apply>
<times/>
<ci> compartment </ci>
<apply>
<ci> PGI_kinetics </ci>
<ci> G6P </ci>
<ci> F6P </ci>
<ci> Vmax </ci>
<ci> Kg6p </ci>
<ci> Keq </ci>
<ci> Kf6p </ci>
</apply>
</apply>
</math>
<listOfParameters>
<parameter id="Vmax" name="Vmax" value="1056"/>
<parameter id="Kg6p" name="Kg6p" value="1.4"/>
<parameter id="Keq" name="Keq" value="0.29"/>
<parameter id="Kf6p" name="Kf6p" value="0.3"/>
</listOfParameters>
</kineticLaw>
</reaction>
<reaction metaid="COPASI30" id="PFK" name="PFK" reversible="false">
<listOfReactants>
<speciesReference species="F6P" stoichiometry="1"/>
<speciesReference species="ATP" stoichiometry="1"/>
</listOfReactants>
<listOfProducts>
<speciesReference species="F16bP" stoichiometry="1"/>
<speciesReference species="ADP" stoichiometry="1"/>
</listOfProducts>
<listOfModifiers>
<modifierSpeciesReference species="AMP"/>
<modifierSpeciesReference species="F26bP"/>
</listOfModifiers>
<kineticLaw>
<math xmlns="http://www.w3.org/1998/Math/MathML">
<apply>
<times/>
<ci> compartment </ci>
<apply>
<ci> PFK_kinetics </ci>
<ci> F6P </ci>
<ci> ATP </ci>
<ci> F16bP </ci>
<ci> AMP </ci>
<ci> F26bP </ci>
<ci> Vmax </ci>
<ci> gR </ci>
<ci> Kf6p </ci>
<ci> Katp </ci>
<ci> L0 </ci>
<ci> Ciatp </ci>
<ci> Kiatp </ci>
<ci> Camp </ci>
<ci> Kamp </ci>
<ci> Cf26 </ci>
<ci> Kf26 </ci>
<ci> Cf16 </ci>
<ci> Kf16 </ci>
<ci> Catp </ci>
</apply>
</apply>
</math>
<listOfParameters>
<parameter id="Vmax" name="Vmax" value="110"/>
<parameter id="gR" name="gR" value="5.12"/>
<parameter id="Kf6p" name="Kf6p" value="0.1"/>
<parameter id="Katp" name="Katp" value="0.71"/>
<parameter id="L0" name="L0" value="0.66"/>
<parameter id="Ciatp" name="Ciatp" value="100"/>
<parameter id="Kiatp" name="Kiatp" value="0.65"/>
<parameter id="Camp" name="Camp" value="0.0845"/>
<parameter id="Kamp" name="Kamp" value="0.0995"/>
<parameter id="Cf26" name="Cf26" value="0.0174"/>
<parameter id="Kf26" name="Kf26" value="0.000682"/>
<parameter id="Cf16" name="Cf16" value="0.397"/>
<parameter id="Kf16" name="Kf16" value="0.111"/>
<parameter id="Catp" name="Catp" value="3"/>
</listOfParameters>
</kineticLaw>
</reaction>
<reaction metaid="COPASI31" id="ALD" name="ALD" reversible="true">
<listOfReactants>
<speciesReference species="F16bP" stoichiometry="1"/>
</listOfReactants>
<listOfProducts>
<speciesReference species="DHAP" stoichiometry="1"/>
<speciesReference species="GAP" stoichiometry="1"/>
</listOfProducts>
<kineticLaw>
<math xmlns="http://www.w3.org/1998/Math/MathML">
<apply>
<times/>
<ci> compartment </ci>
<apply>
<ci> ALD_kinetics </ci>
<ci> F16bP </ci>
<ci> DHAP </ci>
<ci> GAP </ci>
<ci> Vmax </ci>
<ci> Kf16bp </ci>
<ci> Keq </ci>
<ci> Kdhap </ci>
<ci> Kgap </ci>
<ci> Kigap </ci>
</apply>
</apply>
</math>
<listOfParameters>
<parameter id="Vmax" name="Vmax" value="94.69"/>
<parameter id="Kf16bp" name="Kf16bp" value="0.3"/>
<parameter id="Keq" name="Keq" value="0.069"/>
<parameter id="Kdhap" name="Kdhap" value="2"/>
<parameter id="Kgap" name="Kgap" value="2.4"/>
<parameter id="Kigap" name="Kigap" value="10"/>
</listOfParameters>
</kineticLaw>
</reaction>
<reaction metaid="COPASI32" id="TPI" name="TPI" reversible="true">
<listOfReactants>
<speciesReference species="DHAP" stoichiometry="1"/>
</listOfReactants>
<listOfProducts>
<speciesReference species="GAP" stoichiometry="1"/>
</listOfProducts>
<kineticLaw>
<math xmlns="http://www.w3.org/1998/Math/MathML">
<apply>
<times/>
<ci> compartment </ci>
<apply>
<minus/>
<apply>
 <times/>
 <ci> k1 </ci>
 <ci> DHAP </ci>
</apply>
<apply>
 <times/>
 <ci> k2 </ci>
 <ci> GAP </ci>
</apply>
</apply>
</apply>
</math>
<listOfParameters>
<parameter id="k1" name="k1" value="450000"/>
<parameter id="k2" name="k2" value="10000000"/>
</listOfParameters>
</kineticLaw>
</reaction>
<reaction metaid="COPASI33" id="GAPDH" name="GAPDH" reversible="true">
<listOfReactants>
<speciesReference species="GAP" stoichiometry="1"/>
<speciesReference species="NAD" stoichiometry="1"/>
</listOfReactants>
<listOfProducts>
<speciesReference species="BPG" stoichiometry="1"/>
<speciesReference species="NADH" stoichiometry="1"/>
</listOfProducts>
<kineticLaw>
<math xmlns="http://www.w3.org/1998/Math/MathML">
<apply>
<times/>
<ci> compartment </ci>
<apply>
<ci> GAPDH_kinetics </ci>
<ci> GAP </ci>
<ci> NAD </ci>
<ci> BPG </ci>
<ci> NADH </ci>
<ci> C </ci>
<ci> Vmaxf </ci>
<ci> Kgap </ci>
<ci> Knad </ci>
<ci> Vmaxr </ci>
<ci> Kbpg </ci>
<ci> Knadh </ci>
</apply>
</apply>
</math>
<listOfParameters>
<parameter id="C" name="C" value="1"/>
<parameter id="Vmaxf" name="Vmaxf" value="1152"/>
<parameter id="Kgap" name="Kgap" value="0.21"/>
<parameter id="Knad" name="Knad" value="0.09"/>
<parameter id="Vmaxr" name="Vmaxr" value="6719"/>
<parameter id="Kbpg" name="Kbpg" value="0.0098"/>
<parameter id="Knadh" name="Knadh" value="0.06"/>
</listOfParameters>
</kineticLaw>
</reaction>
<reaction metaid="COPASI34" id="PGK" name="PGK" reversible="true">
<listOfReactants>
<speciesReference species="BPG" stoichiometry="1"/>
<speciesReference species="ADP" stoichiometry="1"/>
</listOfReactants>
<listOfProducts>
<speciesReference species="P3G" stoichiometry="1"/>
<speciesReference species="ATP" stoichiometry="1"/>
</listOfProducts>
<kineticLaw>
<math xmlns="http://www.w3.org/1998/Math/MathML">
<apply>
<times/>
<ci> compartment </ci>
<apply>
<ci> PGK_kinetics </ci>
<ci> BPG </ci>
<ci> ADP </ci>
<ci> P3G </ci>
<ci> ATP </ci>
<ci> Vmax </ci>
<ci> Keq </ci>
<ci> Kp3g </ci>
<ci> Katp </ci>
<ci> Kbpg </ci>
<ci> Kadp </ci>
</apply>
</apply>
</math>
<listOfParameters>
<parameter id="Vmax" name="Vmax" value="1288"/>
<parameter id="Keq" name="Keq" value="3200"/>
<parameter id="Kp3g" name="Kp3g" value="0.53"/>
<parameter id="Katp" name="Katp" value="0.3"/>
<parameter id="Kbpg" name="Kbpg" value="0.003"/>
<parameter id="Kadp" name="Kadp" value="0.2"/>
</listOfParameters>
</kineticLaw>
</reaction>
<reaction metaid="COPASI35" id="PGM" name="PGM" reversible="true">
<listOfReactants>
<speciesReference species="P3G" stoichiometry="1"/>
</listOfReactants>
<listOfProducts>
<speciesReference species="P2G" stoichiometry="1"/>
</listOfProducts>
<kineticLaw>
<math xmlns="http://www.w3.org/1998/Math/MathML">
<apply>
<times/>
<ci> compartment </ci>
<apply>
<ci> PGM_kinetics </ci>
<ci> P3G </ci>
<ci> P2G </ci>
<ci> Vmax </ci>
<ci> Kp3g </ci>
<ci> Keq </ci>
<ci> Kp2g </ci>
</apply>
</apply>
</math>
<listOfParameters>
<parameter id="Vmax" name="Vmax" value="2585"/>
<parameter id="Kp3g" name="Kp3g" value="1.2"/>
<parameter id="Keq" name="Keq" value="0.19"/>
<parameter id="Kp2g" name="Kp2g" value="0.08"/>
</listOfParameters>
</kineticLaw>
</reaction>
<reaction metaid="COPASI36" id="ENO" name="ENO" reversible="true">
<listOfReactants>
<speciesReference species="P2G" stoichiometry="1"/>
</listOfReactants>
<listOfProducts>
<speciesReference species="PEP" stoichiometry="1"/>
</listOfProducts>
<kineticLaw>
<math xmlns="http://www.w3.org/1998/Math/MathML">
<apply>
<times/>
<ci> compartment </ci>
<apply>
<ci> ENO_kinetics </ci>
<ci> P2G </ci>
<ci> PEP </ci>
<ci> Vmax </ci>
<ci> Kp2g </ci>
<ci> Keq </ci>
<ci> Kpep </ci>
</apply>
</apply>
</math>
<listOfParameters>
<parameter id="Vmax" name="Vmax" value="201.6"/>
<parameter id="Kp2g" name="Kp2g" value="0.04"/>
<parameter id="Keq" name="Keq" value="6.7"/>
<parameter id="Kpep" name="Kpep" value="0.5"/>
</listOfParameters>
</kineticLaw>
</reaction>
<reaction metaid="COPASI37" id="PYK" name="PYK" reversible="true">
<listOfReactants>
<speciesReference species="PEP" stoichiometry="1"/>
<speciesReference species="ADP" stoichiometry="1"/>
</listOfReactants>
<listOfProducts>
<speciesReference species="PYR" stoichiometry="1"/>
<speciesReference species="ATP" stoichiometry="1"/>
</listOfProducts>
<kineticLaw>
<math xmlns="http://www.w3.org/1998/Math/MathML">
<apply>
<times/>
<ci> compartment </ci>
<apply>
<ci> PYK_kinetics </ci>
<ci> PEP </ci>
<ci> ADP </ci>
<ci> PYR </ci>
<ci> ATP </ci>
<ci> Vmax </ci>
<ci> Kpep </ci>
<ci> Kadp </ci>
<ci> Keq </ci>
<ci> Kpyr </ci>
<ci> Katp </ci>
</apply>
</apply>
</math>
<listOfParameters>
<parameter id="Vmax" name="Vmax" value="1000"/>
<parameter id="Kpep" name="Kpep" value="0.14"/>
<parameter id="Kadp" name="Kadp" value="0.53"/>
<parameter id="Keq" name="Keq" value="6500"/>
<parameter id="Kpyr" name="Kpyr" value="21"/>
<parameter id="Katp" name="Katp" value="1.5"/>
</listOfParameters>
</kineticLaw>
</reaction>
<reaction metaid="COPASI38" id="PDC" name="PDC" reversible="false">
<listOfReactants>
<speciesReference species="PYR" stoichiometry="1"/>
</listOfReactants>
<listOfProducts>
<speciesReference species="AcAld" stoichiometry="1"/>
<speciesReference species="CO2" stoichiometry="1"/>
</listOfProducts>
<kineticLaw>
<math xmlns="http://www.w3.org/1998/Math/MathML">
<apply>
<times/>
<ci> compartment </ci>
<apply>
<ci> PDC_kinetics </ci>
<ci> PYR </ci>
<ci> Vmax </ci>
<ci> Kpyr </ci>
<ci> nH </ci>
</apply>
</apply>
</math>
<listOfParameters>
<parameter id="Vmax" name="Vmax" value="857.8"/>
<parameter id="Kpyr" name="Kpyr" value="4.33"/>
<parameter id="nH" name="nH" value="1.9"/>
</listOfParameters>
</kineticLaw>
</reaction>
<reaction metaid="COPASI39" id="ADH" name="ADH" reversible="true">
<listOfReactants>
<speciesReference species="EtOH" stoichiometry="1"/>
<speciesReference species="NAD" stoichiometry="1"/>
</listOfReactants>
<listOfProducts>
<speciesReference species="AcAld" stoichiometry="1"/>
<speciesReference species="NADH" stoichiometry="1"/>
</listOfProducts>
<kineticLaw>
<math xmlns="http://www.w3.org/1998/Math/MathML">
<apply>
<times/>
<ci> compartment </ci>
<apply>
<ci> ADH_kinetics </ci>
<ci> EtOH </ci>
<ci> NAD </ci>
<ci> AcAld </ci>
<ci> NADH </ci>
<ci> Vmax </ci>
<ci> Ketoh </ci>
<ci> Kinad </ci>
<ci> Keq </ci>
<ci> Knad </ci>
<ci> Knadh </ci>
<ci> Kinadh </ci>
<ci> Kacald </ci>
<ci> Kiacald </ci>
<ci> Kietoh </ci>
</apply>
</apply>
</math>
<listOfParameters>
<parameter id="Vmax" name="Vmax" value="209.5"/>
<parameter id="Ketoh" name="Ketoh" value="17"/>
<parameter id="Kinad" name="Kinad" value="0.92"/>
<parameter id="Keq" name="Keq" value="6.9e-05"/>
<parameter id="Knad" name="Knad" value="0.17"/>
<parameter id="Knadh" name="Knadh" value="0.11"/>
<parameter id="Kinadh" name="Kinadh" value="0.031"/>
<parameter id="Kacald" name="Kacald" value="1.11"/>
<parameter id="Kiacald" name="Kiacald" value="1.1"/>
<parameter id="Kietoh" name="Kietoh" value="90"/>
</listOfParameters>
</kineticLaw>
</reaction>
<reaction metaid="COPASI40" id="ATPase" name="ATPase" reversible="false">
<listOfReactants>
<speciesReference species="ATP" stoichiometry="1"/>
</listOfReactants>
<listOfProducts>
<speciesReference species="ADP" stoichiometry="1"/>
</listOfProducts>
<kineticLaw>
<math xmlns="http://www.w3.org/1998/Math/MathML">
<apply>
<times/>
<ci> compartment </ci>
<apply>
<ci> ATPase_0 </ci>
<ci> ATP </ci>
<ci> Katpase </ci>
</apply>
</apply>
</math>
<listOfParameters>
<parameter id="Katpase" name="Katpase" value="39.5"/>
</listOfParameters>
</kineticLaw>
</reaction>
<reaction metaid="COPASI41" id="AK" name="AK" reversible="true">
<listOfReactants>
<speciesReference species="ADP" stoichiometry="2"/>
</listOfReactants>
<listOfProducts>
<speciesReference species="ATP" stoichiometry="1"/>
<speciesReference species="AMP" stoichiometry="1"/>
</listOfProducts>
<kineticLaw>
<math xmlns="http://www.w3.org/1998/Math/MathML">
<apply>
<times/>
<ci> compartment </ci>
<apply>
<minus/>
<apply>
 <times/>
 <ci> k1 </ci>
 <apply>
   <power/>
   <ci> ADP </ci>
   <cn> 2 </cn>
 </apply>
</apply>
<apply>
 <times/>
 <ci> k2 </ci>
 <ci> ATP </ci>
 <ci> AMP </ci>
</apply>
</apply>
</apply>
</math>
<listOfParameters>
<parameter id="k1" name="k1" value="45"/>
<parameter id="k2" name="k2" value="100"/>
</listOfParameters>
</kineticLaw>
</reaction>
<reaction metaid="COPASI42" id="G3PDH" name="G3PDH" reversible="false">
<listOfReactants>
<speciesReference species="DHAP" stoichiometry="1"/>
<speciesReference species="NADH" stoichiometry="1"/>
</listOfReactants>
<listOfProducts>
<speciesReference species="Glycerol" stoichiometry="1"/>
<speciesReference species="NAD" stoichiometry="1"/>
</listOfProducts>
<kineticLaw>
<math xmlns="http://www.w3.org/1998/Math/MathML">
<apply>
<times/>
<ci> compartment </ci>
<apply>
<ci> G3PDH_kinetics </ci>
<ci> DHAP </ci>
<ci> NADH </ci>
<ci> Glycerol </ci>
<ci> NAD </ci>
<ci> Vmax </ci>
<ci> Kdhap </ci>
<ci> Knadh </ci>
<ci> Keq </ci>
<ci> Kglycerol </ci>
<ci> Knad </ci>
</apply>
</apply>
</math>
<listOfParameters>
<parameter id="Vmax" name="Vmax" value="47.11"/>
<parameter id="Kdhap" name="Kdhap" value="0.4"/>
<parameter id="Knadh" name="Knadh" value="0.023"/>
<parameter id="Keq" name="Keq" value="4300"/>
<parameter id="Kglycerol" name="Kglycerol" value="1"/>
<parameter id="Knad" name="Knad" value="0.93"/>
</listOfParameters>
</kineticLaw>
</reaction>
<reaction metaid="COPASI43" id="Glycogen_Branch" name="Glycogen Branch" reversible="false">
<listOfReactants>
<speciesReference species="G6P" stoichiometry="1"/>
<speciesReference species="ATP" stoichiometry="1"/>
</listOfReactants>
<listOfProducts>
<speciesReference species="ADP" stoichiometry="1"/>
<speciesReference species="Glycogen" stoichiometry="1"/>
</listOfProducts>
<kineticLaw>
<math xmlns="http://www.w3.org/1998/Math/MathML">
<apply>
<times/>
<ci> compartment </ci>
<apply>
<ci> Glycogen_synthesis_kinetics </ci>
<ci> KGLYCOGEN </ci>
</apply>
</apply>
</math>
<listOfParameters>
<parameter id="KGLYCOGEN" name="KGLYCOGEN" value="6"/>
</listOfParameters>
</kineticLaw>
</reaction>
<reaction metaid="COPASI44" id="Trehalose_Branch" name="Trehalose Branch" reversible="false">
<listOfReactants>
<speciesReference species="G6P" stoichiometry="2"/>
<speciesReference species="ATP" stoichiometry="1"/>
</listOfReactants>
<listOfProducts>
<speciesReference species="ADP" stoichiometry="1"/>
<speciesReference species="Trehalose" stoichiometry="1"/>
</listOfProducts>
<kineticLaw>
<math xmlns="http://www.w3.org/1998/Math/MathML">
<apply>
<times/>
<ci> compartment </ci>
<apply>
<ci> Trehalose_synthesis_kinetics </ci>
<ci> Ktrehalose </ci>
</apply>
</apply>
</math>
<listOfParameters>
<parameter id="Ktrehalose" name="Ktrehalose" value="2.4"/>
</listOfParameters>
</kineticLaw>
</reaction>
<reaction metaid="COPASI45" id="Succinate_Branch" name="Succinate Branch" reversible="false">
<listOfReactants>
<speciesReference species="AcAld" stoichiometry="2"/>
<speciesReference species="NAD" stoichiometry="3"/>
</listOfReactants>
<listOfProducts>
<speciesReference species="Succinate" stoichiometry="1"/>
<speciesReference species="NADH" stoichiometry="3"/>
</listOfProducts>
<kineticLaw>
<math xmlns="http://www.w3.org/1998/Math/MathML">
<apply>
<times/>
<ci> compartment </ci>
<apply>
<ci> Succinate_kinetics </ci>
<ci> AcAld </ci>
<ci> k </ci>
</apply>
</apply>
</math>
<listOfParameters>
<parameter id="k" name="k" value="21.4"/>
</listOfParameters>
</kineticLaw>
</reaction>
</listOfReactions>
</model>
</sbml>)=====";

}  // namespace sbml_test_data
