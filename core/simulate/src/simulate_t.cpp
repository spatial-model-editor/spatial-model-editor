#include "catch_wrapper.hpp"
#include "model_test_utils.hpp"
#include "qt_test_utils.hpp"
#include "sme/mesh2d.hpp"
#include "sme/model.hpp"
#include "sme/serialization.hpp"
#include "sme/simulate.hpp"
#include "sme/simulate_options.hpp"
#include "sme/utils.hpp"
#include <QFile>
#include <algorithm>
#include <cmath>
#include <future>

using namespace sme;
using namespace sme::test;
using Catch::Matchers::ContainsSubstring;

TEST_CASE("Simulate: very_simple_model, single pixel geometry",
          "[core/simulate/simulate][core/simulate][core][simulate][pixel]") {
  // import non-spatial model
  auto s{getTestModel("very-simple-model-non-spatial")};

  // import geometry & assign compartments
  QImage img(1, 3, QImage::Format_RGB32);
  QRgb col1 = QColor(12, 243, 154).rgba();
  QRgb col2 = QColor(112, 243, 154).rgba();
  QRgb col3 = QColor(212, 243, 154).rgba();
  img.setPixel(0, 0, col1);
  img.setPixel(0, 1, col2);
  img.setPixel(0, 2, col3);
  img.save("tmpsimsinglepixel.bmp");
  s.getGeometry().importGeometryFromImages(
      common::ImageStack{{QImage("tmpsimsinglepixel.bmp")}}, false);
  s.getGeometry().setVoxelSize({1.0, 1.0, 1.0});
  s.getCompartments().setColour("c1", col1);
  s.getCompartments().setColour("c2", col2);
  s.getCompartments().setColour("c3", col3);
  std::string xml{s.getXml().toStdString()};
  // check we have identified the compartments and membranes
  REQUIRE(s.getCompartments().getIds() == QStringList{"c1", "c2", "c3"});
  REQUIRE(s.getMembranes().getIds() ==
          QStringList{"c1_c2_membrane", "c2_c3_membrane"});

  // check fields have correct compartments & sizes
  geometry::Field *fa1 = s.getSpecies().getField("A_c1");
  REQUIRE(fa1->getCompartment()->getId() == "c1");
  REQUIRE(fa1->getId() == "A_c1");
  geometry::Field *fb1 = s.getSpecies().getField("B_c1");
  REQUIRE(fb1->getCompartment()->getId() == "c1");
  REQUIRE(fb1->getId() == "B_c1");
  geometry::Field *fa2 = s.getSpecies().getField("A_c2");
  REQUIRE(fa2->getCompartment()->getId() == "c2");
  REQUIRE(fa2->getId() == "A_c2");
  geometry::Field *fb2 = s.getSpecies().getField("B_c2");
  REQUIRE(fb2->getCompartment()->getId() == "c2");
  REQUIRE(fb2->getId() == "B_c2");
  geometry::Field *fa3 = s.getSpecies().getField("A_c3");
  REQUIRE(fa3->getCompartment()->getId() == "c3");
  REQUIRE(fa3->getId() == "A_c3");
  geometry::Field *fb3 = s.getSpecies().getField("B_c3");
  REQUIRE(fb3->getCompartment()->getId() == "c3");
  REQUIRE(fb3->getId() == "B_c3");

  // check membranes have correct compartment pairs & sizes
  const auto &m0 = s.getMembranes().getMembranes()[0];
  REQUIRE(m0.getId() == "c1_c2_membrane");
  REQUIRE(m0.getCompartmentA()->getId() == "c1");
  REQUIRE(m0.getCompartmentB()->getId() == "c2");
  REQUIRE(m0.getIndexPairs(sme::geometry::Membrane::X).size() == 0);
  REQUIRE(m0.getIndexPairs(sme::geometry::Membrane::Y).size() == 1);
  REQUIRE(m0.getIndexPairs(sme::geometry::Membrane::Z).size() == 0);
  REQUIRE(m0.getIndexPairs(sme::geometry::Membrane::Y)[0] ==
          std::pair<std::size_t, std::size_t>{0, 0});

  const auto &m1 = s.getMembranes().getMembranes()[1];
  REQUIRE(m1.getId() == "c2_c3_membrane");
  REQUIRE(m1.getCompartmentA()->getId() == "c2");
  REQUIRE(m1.getCompartmentB()->getId() == "c3");
  REQUIRE(m1.getIndexPairs(sme::geometry::Membrane::X).size() == 0);
  REQUIRE(m1.getIndexPairs(sme::geometry::Membrane::Y).size() == 1);
  REQUIRE(m1.getIndexPairs(sme::geometry::Membrane::Z).size() == 0);
  REQUIRE(m1.getIndexPairs(sme::geometry::Membrane::Y)[0] ==
          std::pair<std::size_t, std::size_t>{0, 0});

  // move membrane reactions from compartment to membrane
  s.getReactions().setLocation("A_uptake", "c1_c2_membrane");
  s.getReactions().setLocation("A_transport", "c2_c3_membrane");
  s.getReactions().setLocation("B_transport", "c2_c3_membrane");
  s.getReactions().setLocation("B_excretion", "c1_c2_membrane");

  double dt = 0.134521234;
  // 1st order RK, multi-threaded fixed timestep simulation
  auto &options{s.getSimulationSettings().options};
  options.pixel.integrator = simulate::PixelIntegratorType::RK101;
  options.pixel.maxErr = {std::numeric_limits<double>::max(),
                          std::numeric_limits<double>::max()};
  options.pixel.maxTimestep = dt;
  options.pixel.enableMultiThreading = true;
  options.pixel.maxThreads = 2;
  s.getSimulationSettings().simulatorType = simulate::SimulatorType::Pixel;
  simulate::Simulation sim(s);

  // check initial concentrations:
  // note: A_c1 is constant, so not part of simulation
  REQUIRE(sim.getAvgMinMax(0, 0, 0).avg == dbl_approx(0.0));
  REQUIRE(sim.getAvgMinMax(0, 1, 0).avg == dbl_approx(0.0));
  REQUIRE(sim.getAvgMinMax(0, 1, 1).avg == dbl_approx(0.0));
  REQUIRE(sim.getAvgMinMax(0, 2, 0).avg == dbl_approx(0.0));
  REQUIRE(sim.getAvgMinMax(0, 2, 1).avg == dbl_approx(0.0));

  // check initial concentration image
  auto intialConcImg{sim.getConcImage(0)};
  REQUIRE(intialConcImg.volume().depth() == 1);
  REQUIRE(intialConcImg[0].size() == QSize(1, 3));
  REQUIRE(intialConcImg[0].pixel(0, 0) == qRgb(0, 0, 0));
  REQUIRE(intialConcImg[0].pixel(0, 1) == qRgb(0, 0, 0));
  REQUIRE(intialConcImg[0].pixel(0, 2) == qRgb(0, 0, 0));

  double volC1 = 10.0;
  SECTION("single Euler step") {
    auto steps = sim.doTimesteps(dt);
    REQUIRE(steps == 1);
    std::size_t it = 1;
    // B_c1 = 0
    REQUIRE(sim.getAvgMinMax(it, 0, 0).avg == dbl_approx(0.0));
    REQUIRE(sim.getDcdt(0, 0)[0] == dbl_approx(0.0));
    // A_c2 += k_1 A_c1 dt
    REQUIRE(sim.getAvgMinMax(it, 1, 0).avg == dbl_approx(0.1 * 1.0 * dt));
    REQUIRE(sim.getDcdt(1, 0)[0] == dbl_approx(0.1));
    // B_c2 = 0
    REQUIRE(sim.getAvgMinMax(it, 1, 1).avg == dbl_approx(0.0));
    REQUIRE(sim.getDcdt(1, 1)[0] == dbl_approx(0.0));
    // A_c3 = 0
    REQUIRE(sim.getAvgMinMax(it, 2, 0).avg == dbl_approx(0.0));
    REQUIRE(sim.getDcdt(2, 0)[0] == dbl_approx(0.0));
    // B_c3 = 0
    REQUIRE(sim.getAvgMinMax(it, 2, 1).avg == dbl_approx(0.0));
    REQUIRE(sim.getDcdt(2, 1)[0] == dbl_approx(0.0));
  }

  SECTION("two Euler steps") {
    auto steps = sim.doTimesteps(dt);
    REQUIRE(steps == 1);
    double A_c2 = sim.getAvgMinMax(1, 1, 0).avg;
    steps = sim.doTimesteps(dt);
    REQUIRE(steps == 1);
    std::size_t it = 2;
    // B_c1 = 0
    REQUIRE(sim.getAvgMinMax(it, 0, 0).avg == dbl_approx(0.0));
    REQUIRE(sim.getDcdt(0, 0)[0] == dbl_approx(0.0));
    // A_c2 += k_1 A_c1 dt - k1 * A_c2 * dt
    REQUIRE(sim.getAvgMinMax(it, 1, 0).avg ==
            dbl_approx(A_c2 + 0.1 * dt - A_c2 * 0.1 * dt));
    REQUIRE(sim.getDcdt(1, 0)[0] == dbl_approx(0.1 - A_c2 * 0.1));
    // B_c2 = 0
    REQUIRE(sim.getAvgMinMax(it, 1, 1).avg == dbl_approx(0.0));
    REQUIRE(sim.getDcdt(1, 1)[0] == dbl_approx(0.0));
    // A_c3 += k_1 A_c2 dt / c3
    REQUIRE(sim.getAvgMinMax(it, 2, 0).avg == dbl_approx(0.1 * A_c2 * dt));
    REQUIRE(sim.getDcdt(2, 0)[0] == dbl_approx(0.1 * A_c2));
    // B_c3 = 0
    REQUIRE(sim.getAvgMinMax(it, 2, 1).avg == dbl_approx(0.0));
    REQUIRE(sim.getDcdt(2, 1)[0] == dbl_approx(0.0));
  }

  SECTION("three Euler steps") {
    sim.doTimesteps(dt);
    sim.doTimesteps(dt);
    double A_c2 = sim.getAvgMinMax(2, 1, 0).avg;
    double A_c3 = sim.getAvgMinMax(2, 2, 0).avg;
    sim.doTimesteps(dt);
    std::size_t it = 3;
    // B_c1 = 0
    REQUIRE(sim.getAvgMinMax(it, 0, 0).avg == dbl_approx(0.0));
    // A_c2 += k_1 (A_c1 - A_c2) dt + k2 * A_c3 * dt
    REQUIRE(sim.getAvgMinMax(it, 1, 0).avg ==
            dbl_approx(A_c2 + 0.1 * dt - A_c2 * 0.1 * dt + A_c3 * 0.1 * dt));
    // B_c2 = 0
    REQUIRE(sim.getAvgMinMax(it, 1, 1).avg == dbl_approx(0.0));
    // A_c3 += k_1 A_c2 dt - k_1 A_c3 dt -
    REQUIRE(sim.getAvgMinMax(it, 2, 0).avg ==
            dbl_approx(A_c3 + (0.1 * (A_c2 - A_c3) * dt - 0.3 * A_c3 * dt)));
    // B_c3 = 0.2 * 0.3 * A_c3 * dt
    REQUIRE(sim.getAvgMinMax(it, 2, 1).avg == dbl_approx(0.3 * A_c3 * dt));
  }

  SECTION("many Euler steps -> steady state solution") {
    // when A & B saturate in all compartments, we reach a steady state
    // by conservation: flux of B of into c1 = flux of A from c1 = 0.1
    // all other net fluxes are zero
    double acceptable_error = 1.e-8;
    options.pixel.maxTimestep = 0.20138571;

    simulate::Simulation sim2(s);
    sim2.doTimesteps(1000);
    std::size_t it = sim2.getTimePoints().size() - 1;
    double A_c1 = 1.0;
    double A_c2 = sim2.getAvgMinMax(it, 1, 0).avg;
    double A_c3 = sim2.getAvgMinMax(it, 2, 0).avg;
    double B_c1 = sim2.getAvgMinMax(it, 0, 0).avg;
    double B_c2 = sim2.getAvgMinMax(it, 1, 1).avg;
    double B_c3 = sim2.getAvgMinMax(it, 2, 1).avg;

    // check concentration values
    REQUIRE(A_c1 == Catch::Approx(1.0).epsilon(acceptable_error));
    REQUIRE(A_c2 == Catch::Approx(0.5 * A_c1 * (0.06 + 0.10) / 0.06)
                        .epsilon(acceptable_error));
    REQUIRE(A_c3 == Catch::Approx(A_c2 - A_c1).epsilon(acceptable_error));
    // B_c1 "steady state" solution is linear growth
    REQUIRE(sim2.getDcdt(0, 0)[0] ==
            Catch::Approx(0.1).epsilon(acceptable_error));
    REQUIRE(
        B_c3 ==
        Catch::Approx((0.06 / 0.10) * A_c3 / 0.2).epsilon(acceptable_error));
    REQUIRE(B_c2 == Catch::Approx(B_c3 / 2.0).epsilon(acceptable_error));

    // check concentration derivatives
    double eps = 1.e-5;
    sim2.doTimesteps(eps);
    ++it;
    double dA2 = (sim2.getAvgMinMax(it, 1, 0).avg - A_c2) / eps;
    REQUIRE(dA2 == Catch::Approx(0).epsilon(acceptable_error));
    double dA3 = (sim2.getAvgMinMax(it, 2, 0).avg - A_c3) / eps;
    REQUIRE(dA3 == Catch::Approx(0).epsilon(acceptable_error));
    double dB1 = volC1 * (sim2.getAvgMinMax(it, 0, 0).avg - B_c1) / eps;
    REQUIRE(dB1 == Catch::Approx(1).epsilon(acceptable_error));
    double dB2 = (sim2.getAvgMinMax(it, 1, 1).avg - B_c2) / eps;
    REQUIRE(dB2 == Catch::Approx(0).epsilon(acceptable_error));
    double dB3 = (sim2.getAvgMinMax(it, 2, 1).avg - B_c3) / eps;
    REQUIRE(dB3 == Catch::Approx(0).epsilon(acceptable_error));
  }

  SECTION(
      "anisotropic voxels: many Euler steps -> same steady state solution") {
    // when A & B saturate in all compartments, we reach a steady state
    // by conservation: flux of B of into c1 = flux of A from c1 =
    // 1/voxel-height, all other net fluxes are zero
    s.getGeometry().setVoxelSize(
        {0.62342352135, 0.445698740687352, 0.8323498574235});
    double acceptable_error = 1.e-8;
    options.pixel.maxTimestep = 0.20138571;

    simulate::Simulation sim2(s);
    sim2.doTimesteps(1000);
    std::size_t it = sim2.getTimePoints().size() - 1;
    [[maybe_unused]] double A_c1 = 1.0;
    double A_c2 = sim2.getAvgMinMax(it, 1, 0).avg;
    double A_c3 = sim2.getAvgMinMax(it, 2, 0).avg;
    double B_c1 = sim2.getAvgMinMax(it, 0, 0).avg;
    double B_c2 = sim2.getAvgMinMax(it, 1, 1).avg;
    double B_c3 = sim2.getAvgMinMax(it, 2, 1).avg;
    // check concentration derivatives
    double eps = 1.e-5;
    sim2.doTimesteps(eps);
    ++it;
    double dA2 = (sim2.getAvgMinMax(it, 1, 0).avg - A_c2) / eps;
    REQUIRE(dA2 == Catch::Approx(0).epsilon(acceptable_error));
    double dA3 = (sim2.getAvgMinMax(it, 2, 0).avg - A_c3) / eps;
    REQUIRE(dA3 == Catch::Approx(0).epsilon(acceptable_error));
    double dB1 = volC1 * (sim2.getAvgMinMax(it, 0, 0).avg - B_c1) / eps;
    REQUIRE(dB1 == Catch::Approx(1.0 / s.getGeometry().getVoxelSize().height())
                       .epsilon(acceptable_error));
    double dB2 = (sim2.getAvgMinMax(it, 1, 1).avg - B_c2) / eps;
    REQUIRE(dB2 == Catch::Approx(0).epsilon(acceptable_error));
    double dB3 = (sim2.getAvgMinMax(it, 2, 1).avg - B_c3) / eps;
    REQUIRE(dB3 == Catch::Approx(0).epsilon(acceptable_error));
  }
}

TEST_CASE("Simulate: very_simple_model, 2d geometry",
          "[core/simulate/simulate][core/simulate][core][simulate][pixel]") {
  auto s{getExampleModel(Mod::VerySimpleModel)};
  // check fields have correct compartments & sizes
  const auto *fa1 = s.getSpecies().getField("A_c1");
  REQUIRE(fa1->getCompartment()->getId() == "c1");
  REQUIRE(fa1->getId() == "A_c1");
  const auto *fb1 = s.getSpecies().getField("B_c1");
  REQUIRE(fb1->getCompartment()->getId() == "c1");
  REQUIRE(fb1->getId() == "B_c1");
  const auto *fa2 = s.getSpecies().getField("A_c2");
  REQUIRE(fa2->getCompartment()->getId() == "c2");
  REQUIRE(fa2->getId() == "A_c2");
  const auto *fb2 = s.getSpecies().getField("B_c2");
  REQUIRE(fb2->getCompartment()->getId() == "c2");
  REQUIRE(fb2->getId() == "B_c2");
  const auto *fa3 = s.getSpecies().getField("A_c3");
  REQUIRE(fa3->getCompartment()->getId() == "c3");
  REQUIRE(fa3->getId() == "A_c3");
  const auto *fb3 = s.getSpecies().getField("B_c3");
  REQUIRE(fb3->getCompartment()->getId() == "c3");
  REQUIRE(fb3->getId() == "B_c3");

  // check membranes have correct compartment pairs & sizes
  const auto &m0 = s.getMembranes().getMembranes()[0];
  REQUIRE(m0.getId() == "c1_c2_membrane");
  REQUIRE(m0.getCompartmentA()->getId() == "c1");
  REQUIRE(m0.getCompartmentB()->getId() == "c2");
  REQUIRE(m0.getIndexPairs(sme::geometry::Membrane::X).size() == 178);
  REQUIRE(m0.getIndexPairs(sme::geometry::Membrane::Y).size() == 160);
  REQUIRE(m0.getIndexPairs(sme::geometry::Membrane::Z).size() == 0);

  const auto &m1 = s.getMembranes().getMembranes()[1];
  REQUIRE(m1.getId() == "c2_c3_membrane");
  REQUIRE(m1.getCompartmentA()->getId() == "c2");
  REQUIRE(m1.getCompartmentB()->getId() == "c3");
  REQUIRE(m1.getIndexPairs(sme::geometry::Membrane::X).size() == 58);
  REQUIRE(m1.getIndexPairs(sme::geometry::Membrane::Y).size() == 50);
  REQUIRE(m1.getIndexPairs(sme::geometry::Membrane::Z).size() == 0);

  // 1st order RK, fixed timestep simulation
  auto &options{s.getSimulationSettings().options};
  options.pixel.integrator = simulate::PixelIntegratorType::RK101;
  options.pixel.maxErr = {std::numeric_limits<double>::max(),
                          std::numeric_limits<double>::max()};
  options.pixel.maxTimestep = 0.01;
  options.pixel.enableMultiThreading = false;
  options.pixel.maxThreads = 1;
  s.getSimulationSettings().simulatorType = simulate::SimulatorType::Pixel;

  simulate::Simulation sim(s);

  // check initial concentrations:
  // note: A_c1 is constant, so not part of simulation
  REQUIRE(sim.getAvgMinMax(0, 0, 0).avg == dbl_approx(0.0));
  REQUIRE(sim.getAvgMinMax(0, 1, 0).avg == dbl_approx(0.0));
  REQUIRE(sim.getAvgMinMax(0, 1, 1).avg == dbl_approx(0.0));
  REQUIRE(sim.getAvgMinMax(0, 2, 0).avg == dbl_approx(0.0));
  REQUIRE(sim.getAvgMinMax(0, 2, 1).avg == dbl_approx(0.0));

  SECTION("one Euler steps: diffusion of A into c2") {
    sim.doTimesteps(0.01);
    REQUIRE(sim.getAvgMinMax(1, 0, 0).avg == dbl_approx(0.0));
    REQUIRE(sim.getAvgMinMax(1, 1, 0).avg > 0);
    REQUIRE(sim.getAvgMinMax(1, 1, 1).avg == dbl_approx(0.0));
    REQUIRE(sim.getAvgMinMax(1, 2, 0).avg == dbl_approx(0.0));
    REQUIRE(sim.getAvgMinMax(1, 2, 1).avg == dbl_approx(0.0));
  }

  SECTION("many Euler steps: all species non-zero") {
    sim.doTimesteps(1.00);
    REQUIRE(sim.getAvgMinMax(1, 0, 0).avg > 0);
    REQUIRE(sim.getAvgMinMax(1, 1, 0).avg > 0);
    REQUIRE(sim.getAvgMinMax(1, 1, 1).avg > 0);
    REQUIRE(sim.getAvgMinMax(1, 2, 0).avg > 0);
    REQUIRE(sim.getAvgMinMax(1, 2, 1).avg > 0);
  }
}

TEST_CASE("Simulate: very_simple_model, failing Pixel sim",
          "[core/simulate/simulate][core/simulate][core][simulate][pixel]") {
  auto s{getExampleModel(Mod::VerySimpleModel)};
  // set A uptake rate very high
  s.getReactions().setParameterValue("A_uptake", "k1", 1e40);
  // Pixel sim stops simulation as timestep required becomes too small
  s.getSimulationSettings().simulatorType = simulate::SimulatorType::Pixel;
  simulate::Simulation sim(s);
  REQUIRE(sim.errorMessage().empty());
  REQUIRE(sim.errorImages().empty());
  sim.doTimesteps(1.0);
  REQUIRE(!sim.errorMessage().empty());
  REQUIRE(sim.errorImages()[0].size() == QSize(100, 100));
}

TEST_CASE("Simulate: very_simple_model, empty compartment, DUNE sim",
          "[core/simulate/simulate][core/simulate][core][simulate][dune]") {
  // check that DUNE simulates a model with an empty compartment without
  // crashing
  auto s{getExampleModel(Mod::VerySimpleModel)};
  s.getSimulationSettings().simulatorType = simulate::SimulatorType::DUNE;
  SECTION("Outer species removed") {
    s.getSpecies().remove("A_c1");
    s.getSpecies().remove("B_c1");
    REQUIRE(s.getSpecies().getIds("c1").empty());
    simulate::Simulation sim(s);
    CAPTURE(sim.errorMessage());
    REQUIRE(sim.errorMessage().empty());
    sim.doTimesteps(0.1, 1);
    CAPTURE(sim.errorMessage());
    REQUIRE(sim.errorMessage().empty());
  }
  SECTION("Inner and Outer species removed") {
    s.getSpecies().remove("A_c1");
    s.getSpecies().remove("B_c1");
    REQUIRE(s.getSpecies().getIds("c1").empty());
    s.getSpecies().remove("A_c3");
    s.getSpecies().remove("B_c3");
    REQUIRE(s.getSpecies().getIds("c3").empty());
    simulate::Simulation sim(s);
    CAPTURE(sim.errorMessage());
    REQUIRE(sim.errorMessage().empty());
    sim.doTimesteps(0.1, 1);
    REQUIRE(sim.errorMessage().empty());
  }
}

static void rescaleMembraneReacRates(model::Model &s, double factor) {
  for (const auto &memId : s.getMembranes().getIds()) {
    for (const auto &reacId : s.getReactions().getIds(memId)) {
      for (const auto &paramId : s.getReactions().getParameterIds(reacId)) {
        double v{s.getReactions().getParameterValue(reacId, paramId)};
        s.getReactions().setParameterValue(reacId, paramId, v * factor);
      }
    }
  }
}

static void rescaleDiffusionConstants(model::Model &s, double factor) {
  for (const auto &cId : s.getCompartments().getIds()) {
    for (const auto &sId : s.getSpecies().getIds(cId)) {
      double d{s.getSpecies().getDiffusionConstant(sId)};
      s.getSpecies().setDiffusionConstant(sId, d * factor);
    }
  }
}

TEST_CASE("Simulate: very_simple_model, change pixel volume, Pixel sim",
          "[core/simulate/simulate][core/simulate][core][simulate]") {
  double epsilon{1e-8};
  double margin{1e-13};
  // import model
  auto s1{getExampleModel(Mod::VerySimpleModel)};
  // 1st order RK, fixed timestep simulation
  // pixel: 1x1x1
  // length unit: m
  // volume unit: L
  // alpha = length^3/volume = 1e3
  REQUIRE(s1.getGeometry().getVoxelSize().width() == dbl_approx(1.0));
  REQUIRE(s1.getGeometry().getVoxelSize().height() == dbl_approx(1.0));
  REQUIRE(s1.getGeometry().getVoxelSize().depth() == dbl_approx(1.0));
  REQUIRE(s1.getUnits().getLength().name == "m");
  REQUIRE(s1.getUnits().getVolume().name == "L");
  auto &options{s1.getSimulationSettings().options};
  options.pixel.integrator = simulate::PixelIntegratorType::RK101;
  options.pixel.maxErr = {std::numeric_limits<double>::max(),
                          std::numeric_limits<double>::max()};
  options.pixel.maxTimestep = 0.01;
  options.pixel.enableMultiThreading = false;
  options.pixel.maxThreads = 1;
  s1.getSimulationSettings().simulatorType = simulate::SimulatorType::Pixel;
  simulate::Simulation sim(s1);
  sim.doTimesteps(0.20);
  REQUIRE(sim.getTimePoints().size() == 2);

  // 3x pixel width
  {
    auto s{getExampleModel(Mod::VerySimpleModel)};
    s.getSimulationSettings().options = options;
    s.getSimulationSettings().simulatorType = simulate::SimulatorType::Pixel;
    s.getGeometry().setVoxelSize({3.0, 3.0, 3.0});
    REQUIRE(s.getGeometry().getVoxelSize().width() == dbl_approx(3.0));
    REQUIRE(s.getGeometry().getVoxelSize().height() == dbl_approx(3.0));
    REQUIRE(s.getGeometry().getVoxelSize().depth() == dbl_approx(3.0));
    // pixel width -> 3x larger
    // units, concentrations & reaction rates unchanged
    // but membrane flux per pixel -> 9x larger
    // while volume of pixel -> 27x larger
    // so net conc change from membrane flux in pixel -> 9x/27x = 1/3
    // multiply membrane rate by 3 to compensate for this
    rescaleMembraneReacRates(s, 3.0);
    // rescale diffusion such that rate in pixel units is the same
    rescaleDiffusionConstants(s, 9.0);
    simulate::Simulation sim2(s);
    sim2.doTimesteps(0.20);
    REQUIRE(sim2.getTimePoints().size() == 2);
    REQUIRE(sim2.errorMessage().empty());
    for (auto iComp : {std::size_t(0), std::size_t(1), std::size_t(2)}) {
      for (auto iSpec : {std::size_t(0), std::size_t(1)}) {
        if (!(iComp == 0 && iSpec == 1)) {
          REQUIRE(sim2.getAvgMinMax(1, iComp, iSpec).avg ==
                  Catch::Approx(sim.getAvgMinMax(1, iComp, iSpec).avg)
                      .epsilon(epsilon)
                      .margin(margin));
          REQUIRE(sim2.getAvgMinMax(1, iComp, iSpec).min ==
                  Catch::Approx(sim.getAvgMinMax(1, iComp, iSpec).min)
                      .epsilon(epsilon)
                      .margin(margin));
          REQUIRE(sim2.getAvgMinMax(1, iComp, iSpec).max ==
                  Catch::Approx(sim.getAvgMinMax(1, iComp, iSpec).max)
                      .epsilon(epsilon)
                      .margin(margin));
        }
      }
    }
    // save model, load it again and repeat test, to ensure pixel width is
    // propagated correctly to pixel simulator on model load:
    // https://github.com/spatial-model-editor/spatial-model-editor/issues/468
    s.exportSBMLFile("tmpsimpixelsize.xml");
    model::Model sload;
    sload.importSBMLFile("tmpsimpixelsize.xml");
    sload.getSimulationSettings().options = options;
    sload.getSimulationSettings().simulatorType =
        simulate::SimulatorType::Pixel;

    simulate::Simulation simload(sload);
    simload.doTimesteps(0.20);
    REQUIRE(simload.getTimePoints().size() == 2);
    REQUIRE(simload.errorMessage().empty());
    for (auto iComp : {std::size_t(0), std::size_t(1), std::size_t(2)}) {
      for (auto iSpec : {std::size_t(0), std::size_t(1)}) {
        if (!(iComp == 0 && iSpec == 1)) {
          REQUIRE(simload.getAvgMinMax(1, iComp, iSpec).avg ==
                  Catch::Approx(sim.getAvgMinMax(1, iComp, iSpec).avg)
                      .epsilon(epsilon)
                      .margin(margin));
          REQUIRE(simload.getAvgMinMax(1, iComp, iSpec).min ==
                  Catch::Approx(sim.getAvgMinMax(1, iComp, iSpec).min)
                      .epsilon(epsilon)
                      .margin(margin));
          REQUIRE(simload.getAvgMinMax(1, iComp, iSpec).max ==
                  Catch::Approx(sim.getAvgMinMax(1, iComp, iSpec).max)
                      .epsilon(epsilon)
                      .margin(margin));
        }
      }
    }
  }
  // 0.27* pixel width
  {
    auto s{getExampleModel(Mod::VerySimpleModel)};
    s.getSimulationSettings().options = options;
    s.getSimulationSettings().simulatorType = simulate::SimulatorType::Pixel;
    s.getGeometry().setVoxelSize({0.27, 0.27, 0.27});
    REQUIRE(s.getGeometry().getVoxelSize().width() == dbl_approx(0.27));
    REQUIRE(s.getGeometry().getVoxelSize().height() == dbl_approx(0.27));
    REQUIRE(s.getGeometry().getVoxelSize().depth() == dbl_approx(0.27));
    // as above
    rescaleMembraneReacRates(s, 0.27);
    rescaleDiffusionConstants(s, 0.27 * 0.27);
    simulate::Simulation sim2(s);
    sim2.doTimesteps(0.20);
    REQUIRE(sim2.errorMessage().empty());
    REQUIRE(sim2.getTimePoints().size() == 2);
    for (auto iComp : {std::size_t(0), std::size_t(1), std::size_t(2)}) {
      for (auto iSpec : {std::size_t(0), std::size_t(1)}) {
        if (!(iComp == 0 && iSpec == 1)) {
          REQUIRE(sim2.getAvgMinMax(1, iComp, iSpec).avg ==
                  Catch::Approx(sim.getAvgMinMax(1, iComp, iSpec).avg)
                      .epsilon(epsilon)
                      .margin(margin));
          REQUIRE(sim2.getAvgMinMax(1, iComp, iSpec).min ==
                  Catch::Approx(sim.getAvgMinMax(1, iComp, iSpec).min)
                      .epsilon(epsilon)
                      .margin(margin));
          REQUIRE(sim2.getAvgMinMax(1, iComp, iSpec).max ==
                  Catch::Approx(sim.getAvgMinMax(1, iComp, iSpec).max)
                      .epsilon(epsilon)
                      .margin(margin));
        }
      }
    }
    // save model, load it again and repeat test as above
    s.exportSBMLFile("tmpsimpixelsize2.xml");
    model::Model sload;
    sload.importSBMLFile("tmpsimpixelsize2.xml");
    sload.getSimulationSettings().options = options;
    sload.getSimulationSettings().simulatorType =
        simulate::SimulatorType::Pixel;
    simulate::Simulation simload(sload);
    simload.doTimesteps(0.20);
    REQUIRE(simload.getTimePoints().size() == 2);
    REQUIRE(simload.errorMessage().empty());
    for (auto iComp : {std::size_t(0), std::size_t(1), std::size_t(2)}) {
      for (auto iSpec : {std::size_t(0), std::size_t(1)}) {
        if (!(iComp == 0 && iSpec == 1)) {
          REQUIRE(simload.getAvgMinMax(1, iComp, iSpec).avg ==
                  Catch::Approx(sim.getAvgMinMax(1, iComp, iSpec).avg)
                      .epsilon(epsilon)
                      .margin(margin));
          REQUIRE(simload.getAvgMinMax(1, iComp, iSpec).min ==
                  Catch::Approx(sim.getAvgMinMax(1, iComp, iSpec).min)
                      .epsilon(epsilon)
                      .margin(margin));
          REQUIRE(simload.getAvgMinMax(1, iComp, iSpec).max ==
                  Catch::Approx(sim.getAvgMinMax(1, iComp, iSpec).max)
                      .epsilon(epsilon)
                      .margin(margin));
        }
      }
    }
  }
}

TEST_CASE("Simulate: very_simple_model, membrane reaction units consistency",
          "[core/simulate/simulate][core/simulate][core][simulate]") {
  for (auto simulatorType :
       {simulate::SimulatorType::DUNE, simulate::SimulatorType::Pixel}) {
    double epsilon{1e-8};
    double margin{1e-13};
    if (simulatorType == simulate::SimulatorType::DUNE) {
      // todo: why this has increased so much vs dune-copasi 1 (1e-6 epsilon,
      // 1e-13 margin)
      margin = 1e-6;
      epsilon = 1e-2;
    }
    double simTime{0.025};
    // import model
    auto s1{getExampleModel(Mod::VerySimpleModel)};
    // 1st order RK, fixed timestep simulation
    // pixel width: 1
    // length unit: m
    // volume unit: L
    // alpha = length^3/volume = 1e3
    REQUIRE(s1.getGeometry().getVoxelSize().width() == dbl_approx(1.0));
    REQUIRE(s1.getGeometry().getVoxelSize().height() == dbl_approx(1.0));
    REQUIRE(s1.getGeometry().getVoxelSize().depth() == dbl_approx(1.0));
    REQUIRE(s1.getUnits().getLength().name == "m");
    REQUIRE(s1.getUnits().getVolume().name == "L");
    s1.getSimulationSettings().simulatorType = simulatorType;
    auto &options{s1.getSimulationSettings().options};
    options.pixel.integrator = simulate::PixelIntegratorType::RK101;
    options.pixel.maxErr = {std::numeric_limits<double>::max(),
                            std::numeric_limits<double>::max()};
    options.pixel.maxTimestep = 0.01;
    options.pixel.enableMultiThreading = false;
    options.pixel.maxThreads = 1;
    options.dune.dt = 0.01;
    options.dune.minDt = 0.001;
    options.dune.maxDt = 0.011;
    CAPTURE(simulatorType);
    simulate::Simulation sim(s1);
    sim.doTimesteps(simTime);
    REQUIRE(sim.getTimePoints().size() == 2);

    // Length unit -> 10x smaller
    {
      auto s{getExampleModel(Mod::VerySimpleModel)};
      s.getSimulationSettings().options = options;
      s.getSimulationSettings().simulatorType = simulatorType;
      s.getUnits().setLengthIndex(1);
      // concentrations & compartment reaction rates unaffected by length units
      // membrane reaction in units of [amount]/[length]^3 is the same
      // but since [volume] is unchanged and [length] is 10x smaller,
      // the concentration in [amount]/[volume] is 1000x larger
      // divide membrane reaction rate by 1000 to compensate for this
      rescaleMembraneReacRates(s, 1e-3);
      s.getUnits().setLengthIndex(1);
      REQUIRE(s.getUnits().getLength().name == "dm");
      simulate::Simulation sim2(s);
      sim2.doTimesteps(simTime);
      REQUIRE(sim2.getTimePoints().size() == 2);
      REQUIRE(sim2.errorMessage().empty());
      for (auto iComp : {std::size_t(0), std::size_t(1), std::size_t(2)}) {
        for (auto iSpec : {std::size_t(0), std::size_t(1)}) {
          if (!(iComp == 0 && iSpec == 1)) {
            REQUIRE(sim2.getAvgMinMax(1, iComp, iSpec).avg ==
                    Catch::Approx(sim.getAvgMinMax(1, iComp, iSpec).avg)
                        .epsilon(epsilon)
                        .margin(margin));
          }
        }
      }
    }

    // Length unit -> 100x smaller
    {
      // as above
      auto s{getExampleModel(Mod::VerySimpleModel)};
      s.getSimulationSettings().options = options;
      s.getSimulationSettings().simulatorType = simulatorType;
      rescaleMembraneReacRates(s, 1e-6);
      s.getUnits().setLengthIndex(2);
      REQUIRE(s.getUnits().getLength().name == "cm");
      simulate::Simulation sim2(s);
      sim2.doTimesteps(simTime);
      REQUIRE(sim2.getTimePoints().size() == 2);
      REQUIRE(sim2.errorMessage().empty());
      for (auto iComp : {std::size_t(0), std::size_t(1), std::size_t(2)}) {
        for (auto iSpec : {std::size_t(0), std::size_t(1)}) {
          if (!(iComp == 0 && iSpec == 1)) {
            REQUIRE(sim2.getAvgMinMax(1, iComp, iSpec).avg ==
                    Catch::Approx(sim.getAvgMinMax(1, iComp, iSpec).avg)
                        .epsilon(epsilon)
                        .margin(margin));
          }
        }
      }
    }
    // Volume unit -> 10x smaller
    {
      // concentration values unchanged, but [volume] 10x smaller
      // i.e. concentrations are 10x higher in [amount]/[length]^3 units
      // so membrane rates need to be 10x higher to compensate
      auto s{getExampleModel(Mod::VerySimpleModel)};
      s.getSimulationSettings().options = options;
      s.getSimulationSettings().simulatorType = simulatorType;
      rescaleMembraneReacRates(s, 10);
      s.getUnits().setVolumeIndex(1);
      REQUIRE(s.getUnits().getVolume().name == "dL");
      simulate::Simulation sim2(s);
      sim2.doTimesteps(simTime);
      REQUIRE(sim2.getTimePoints().size() == 2);
      REQUIRE(sim2.errorMessage().empty());
      for (auto iComp : {std::size_t(0), std::size_t(1), std::size_t(2)}) {
        for (auto iSpec : {std::size_t(0), std::size_t(1)}) {
          if (!(iComp == 0 && iSpec == 1)) {
            REQUIRE(sim2.getAvgMinMax(1, iComp, iSpec).avg ==
                    Catch::Approx(sim.getAvgMinMax(1, iComp, iSpec).avg)
                        .epsilon(epsilon)
                        .margin(margin));
          }
        }
      }
    }
    // Volume unit -> 1000x smaller
    {
      // as above
      auto s{getExampleModel(Mod::VerySimpleModel)};
      s.getSimulationSettings().options = options;
      s.getSimulationSettings().simulatorType = simulatorType;
      rescaleMembraneReacRates(s, 1000);
      s.getUnits().setVolumeIndex(3);
      REQUIRE(s.getUnits().getVolume().name == "mL");
      simulate::Simulation sim2(s);
      sim2.doTimesteps(simTime);
      REQUIRE(sim2.getTimePoints().size() == 2);
      REQUIRE(sim2.errorMessage().empty());
      for (auto iComp : {std::size_t(0), std::size_t(1), std::size_t(2)}) {
        for (auto iSpec : {std::size_t(0), std::size_t(1)}) {
          if (!(iComp == 0 && iSpec == 1)) {
            REQUIRE(sim2.getAvgMinMax(1, iComp, iSpec).avg ==
                    Catch::Approx(sim.getAvgMinMax(1, iComp, iSpec).avg)
                        .epsilon(epsilon)
                        .margin(margin));
          }
        }
      }
    }
    // Volume unit -> 1000x larger (L -> m^3)
    {
      auto s{getExampleModel(Mod::VerySimpleModel)};
      s.getSimulationSettings().options = options;
      s.getSimulationSettings().simulatorType = simulatorType;
      rescaleMembraneReacRates(s, 1e-3);
      s.getUnits().setVolumeIndex(4);
      REQUIRE(s.getUnits().getVolume().name == "m3");
      simulate::Simulation sim2(s);
      sim2.doTimesteps(simTime);
      REQUIRE(sim2.getTimePoints().size() == 2);
      REQUIRE(sim2.errorMessage().empty());
      for (auto iComp : {std::size_t(0), std::size_t(1), std::size_t(2)}) {
        for (auto iSpec : {std::size_t(0), std::size_t(1)}) {
          if (!(iComp == 0 && iSpec == 1)) {
            REQUIRE(sim2.getAvgMinMax(1, iComp, iSpec).avg ==
                    Catch::Approx(sim.getAvgMinMax(1, iComp, iSpec).avg)
                        .epsilon(epsilon)
                        .margin(margin));
          }
        }
      }
    }
    // Volume unit equivalent (L -> dm^3)
    {
      auto s{getExampleModel(Mod::VerySimpleModel)};
      s.getSimulationSettings().options = options;
      s.getSimulationSettings().simulatorType = simulatorType;
      s.getUnits().setVolumeIndex(5);
      REQUIRE(s.getUnits().getVolume().name == "dm3");
      simulate::Simulation sim2(s);
      sim2.doTimesteps(simTime);
      REQUIRE(sim2.getTimePoints().size() == 2);
      REQUIRE(sim2.errorMessage().empty());
      for (auto iComp : {std::size_t(0), std::size_t(1), std::size_t(2)}) {
        for (auto iSpec : {std::size_t(0), std::size_t(1)}) {
          if (!(iComp == 0 && iSpec == 1)) {
            REQUIRE(sim2.getAvgMinMax(1, iComp, iSpec).avg ==
                    Catch::Approx(sim.getAvgMinMax(1, iComp, iSpec).avg)
                        .epsilon(epsilon)
                        .margin(margin));
          }
        }
      }
    }
  }
}

// return r2 distance from origin of point
static double r2(const QPoint &p) {
  // nb: flip y axis since qpoint has y=0 at top
  // use point at centre of pixel
  return std::pow(p.x() - 48 + 0.5, 2) + std::pow((99 - p.y()) - 48 + 0.5, 2);
}

// return 2d analytic prediction for concentration
// u(t) = [t0/(t+t0)] * exp(-r^2/(4Dt))
static double analytic_2d(const QPoint &p, double t, double D, double t0) {
  return (t0 / (t + t0)) * exp(-r2(p) / (4.0 * D * (t + t0)));
}

TEST_CASE("Simulate: single-compartment-diffusion, circular geometry",
          "[core/simulate/simulate][core/"
          "simulate][core][simulate][dune][pixel][expensive]") {
  // see docs/tests/diffusion.rst for analytic expressions used here
  // NB central point of initial distribution: (48,99-48) <-> ix=1577

  constexpr double pi = 3.14159265358979323846;
  double sigma2 = 36.0;
  double epsilon = 1e-10;
  auto s{getExampleModel(Mod::SingleCompartmentDiffusion)};

  // check fields have correct compartments
  const auto *slow = s.getSpecies().getField("slow");
  REQUIRE(slow->getCompartment()->getId() == "circle");
  REQUIRE(slow->getId() == "slow");
  const auto *fast = s.getSpecies().getField("fast");
  REQUIRE(fast->getCompartment()->getId() == "circle");
  REQUIRE(fast->getId() == "fast");

  // check total initial concentration matches analytic value
  double analytic_total = sigma2 * pi;
  for (const auto &c : {slow->getConcentration(), fast->getConcentration()}) {
    REQUIRE(std::abs(common::sum(c) - analytic_total) / analytic_total <
            epsilon);
  }

  // check initial distribution matches analytic one
  for (const auto &f : {slow, fast}) {
    double D = f->getDiffusionConstant();
    double t0 = sigma2 / 4.0 / D;
    double maxRelErr = 0;
    for (std::size_t i = 0; i < f->getCompartment()->nVoxels(); ++i) {
      const auto &v{f->getCompartment()->getVoxel(i)};
      double c = analytic_2d(v.p, 0, D, t0);
      double relErr = std::abs(f->getConcentration()[i] - c) / c;
      maxRelErr = std::max(maxRelErr, relErr);
    }
    CAPTURE(f->getDiffusionConstant());
    REQUIRE(maxRelErr < epsilon);
  }

  auto &options{s.getSimulationSettings().options};
  options.pixel.maxErr = {std::numeric_limits<double>::max(), 0.01};
  options.dune.dt = 1.0;
  options.dune.maxDt = 1.0;
  options.dune.minDt = 0.5;
  for (auto simType :
       {simulate::SimulatorType::Pixel, simulate::SimulatorType::DUNE}) {
    // relative error on integral of initial concentration over all pixels:
    double initialRelativeError{1e-9};
    // largest relative error of any pixel after simulation:
    double evolvedMaxRelativeError{0.02};
    // average of relative errors of all pixels after simulation:
    double evolvedAvgRelativeError{0.003};
    if (simType == simulate::SimulatorType::DUNE) {
      // increase allowed error for dune simulation
      initialRelativeError = 0.02;
      evolvedMaxRelativeError = 0.3;
      evolvedAvgRelativeError = 0.10;
    }
    s.getSimulationSettings().simulatorType = simType;
    s.getSimulationData().clear();

    // integrate & compare
    simulate::Simulation sim(s);
    double t = 10.0;
    for (std::size_t step = 0; step < 2; ++step) {
      sim.doTimesteps(t);
      for (auto speciesIndex : {std::size_t{0}, std::size_t{1}}) {
        // check total concentration is conserved
        auto c = sim.getConc(step + 1, 0, speciesIndex);
        double totalC = common::sum(c);
        double relErr = std::abs(totalC - analytic_total) / analytic_total;
        CAPTURE(simType);
        CAPTURE(speciesIndex);
        CAPTURE(sim.getTimePoints().back());
        CAPTURE(totalC);
        CAPTURE(analytic_total);
        REQUIRE(relErr < initialRelativeError);
      }
    }

    // check new distribution matches analytic one
    std::vector<double> D{slow->getDiffusionConstant(),
                          fast->getDiffusionConstant()};
    std::size_t timeIndex = sim.getTimePoints().size() - 1;
    t = sim.getTimePoints().back();
    for (auto speciesIndex : {std::size_t{0}, std::size_t{1}}) {
      double t0 = sigma2 / 4.0 / D[speciesIndex];
      auto conc = sim.getConc(timeIndex, 0, speciesIndex);
      double maxRelErr{0};
      double avgRelErr{0};
      std::size_t count{0};
      for (std::size_t i = 0; i < slow->getCompartment()->nVoxels(); ++i) {
        const auto &v{slow->getCompartment()->getVoxel(i)};
        // only check part within a radius of 16 units from centre to avoid
        // boundary effects: analytic solution is in infinite volume
        if (r2(v.p) < 16 * 16) {
          double c_analytic = analytic_2d(v.p, t, D[speciesIndex], t0);
          double relErr = std::abs(conc[i] - c_analytic) / c_analytic;
          avgRelErr += relErr;
          ++count;
          maxRelErr = std::max(maxRelErr, relErr);
        }
      }
      avgRelErr /= static_cast<double>(count);
      CAPTURE(simType);
      CAPTURE(t);
      REQUIRE(maxRelErr < evolvedMaxRelativeError);
      REQUIRE(avgRelErr < evolvedAvgRelativeError);
    }
  }
}

static double analytic_3d(const sme::common::VoxelF &v, double t, double D,
                          double t0) {
  return std::pow(t0 / (t + t0), 1.5) *
         exp(-(std::pow(v.p.x(), 2) + std::pow(v.p.y(), 2) + std::pow(v.z, 2)) /
             (4.0 * D * (t + t0)));
}

TEST_CASE("Simulate: single-compartment-diffusion-3d, spherical geometry",
          "[core/simulate/simulate][core/"
          "simulate][core][simulate][dune][pixel][expensive][3d]") {
  // see docs/tests/diffusion.rst for analytic expressions used here

  constexpr double pi = 3.14159265358979323846;
  double sigma2 = 36.0;
  double epsilon = 1e-10;
  auto s{getExampleModel(Mod::SingleCompartmentDiffusion3D)};
  REQUIRE(s.getGeometry().getVoxelSize().width() == dbl_approx(2.0));
  REQUIRE(s.getGeometry().getVoxelSize().height() == dbl_approx(2.0));
  REQUIRE(s.getGeometry().getVoxelSize().depth() == dbl_approx(2.0));
  auto initial_voxel_volume{s.getGeometry().getVoxelSize().volume()};
  // Asymmetric voxels shouldn't affect diffusion (apart from modifying
  // discretization errors)
  double w{1.4876432};
  double h{2.319873454};
  double d{2.0 * 2.0 * 2.0 / w / h};
  s.getGeometry().setVoxelSize({w, h, d});
  s.getSpecies().setAnalyticConcentration("slow",
                                          "exp((-1/36) * (x^2 + y^2 + z^2))");
  s.getSpecies().setAnalyticConcentration("fast",
                                          "exp((-1/36) * (x^2 + y^2 + z^2))");
  auto voxel_volume{s.getGeometry().getVoxelSize().volume()};
  REQUIRE(voxel_volume == dbl_approx(initial_voxel_volume));

  // check fields have correct compartments
  const auto *slow{s.getSpecies().getField("slow")};
  REQUIRE(slow->getCompartment()->getId() == "cube");
  REQUIRE(slow->getId() == "slow");
  const auto *fast{s.getSpecies().getField("fast")};
  REQUIRE(fast->getCompartment()->getId() == "cube");
  REQUIRE(fast->getId() == "fast");

  // check total initial species amount matches analytic value
  double analytic_total = sigma2 * pi * std::sqrt(sigma2 * pi);
  for (const auto &c : {slow->getConcentration(), fast->getConcentration()}) {
    CAPTURE(analytic_total);
    CAPTURE(std::abs(voxel_volume * common::sum(c)));
    REQUIRE(std::abs(voxel_volume * common::sum(c) - analytic_total) /
                analytic_total <
            epsilon);
  }

  // check initial distribution matches analytic one
  for (const auto &f : {slow, fast}) {
    double D = f->getDiffusionConstant();
    double t0 = sigma2 / 4.0 / D;
    double maxRelErr = 0;
    for (std::size_t i = 0; i < f->getCompartment()->nVoxels(); ++i) {
      const auto &v{f->getCompartment()->getVoxel(i)};
      auto physicalPoint{s.getGeometry().getPhysicalPoint(v)};
      double c = analytic_3d(physicalPoint, 0, D, t0);
      double relErr = std::abs(f->getConcentration()[i] - c) / c;
      maxRelErr = std::max(maxRelErr, relErr);
    }
    CAPTURE(f->getDiffusionConstant());
    REQUIRE(maxRelErr < epsilon);
  }

  auto &options{s.getSimulationSettings().options};
  options.pixel.maxErr = {std::numeric_limits<double>::max(), 0.01};
  options.dune.dt = 1.0;
  options.dune.maxDt = 1.0;
  options.dune.minDt = 0.5;
  // make a fine mesh for dune
  s.getGeometry().getMesh3d()->setCompartmentMaxCellVolume(0, 4);
  for (auto simType :
       {simulate::SimulatorType::Pixel, simulate::SimulatorType::DUNE}) {
    // relative error on integral of initial concentration over all pixels:
    double initialRelativeError{1e-9};
    // largest relative error of any pixel after simulation:
    double evolvedMaxRelativeError{0.030};
    // average of relative errors of all pixels after simulation:
    double evolvedAvgRelativeError{0.010};
    if (simType == simulate::SimulatorType::DUNE) {
      // increase allowed error for dune simulation
      initialRelativeError = 0.03;
      evolvedMaxRelativeError = 0.40;
      evolvedAvgRelativeError = 0.10;
    }
    s.getSimulationSettings().simulatorType = simType;
    s.getSimulationData().clear();

    // integrate & compare
    simulate::Simulation sim(s);
    double t = 10.0;
    for (std::size_t step = 0; step < 2; ++step) {
      sim.doTimesteps(t);
      for (auto speciesIndex : {0u, 1u}) {
        // check total species amount is conserved
        auto c = sim.getConc(step + 1, 0, speciesIndex);
        double totalC = voxel_volume * common::sum(c);
        double relErr = std::abs(totalC - analytic_total) / analytic_total;
        CAPTURE(simType);
        CAPTURE(speciesIndex);
        CAPTURE(sim.getTimePoints().back());
        CAPTURE(totalC);
        CAPTURE(analytic_total);
        REQUIRE(relErr < initialRelativeError);
      }
    }

    // check new distribution matches analytic_3d one
    std::vector<double> D{slow->getDiffusionConstant(),
                          fast->getDiffusionConstant()};
    std::size_t timeIndex = sim.getTimePoints().size() - 1;
    t = sim.getTimePoints().back();
    for (auto speciesIndex : {0u, 1u}) {
      double t0 = sigma2 / 4.0 / D[speciesIndex];
      auto conc = sim.getConc(timeIndex, 0, speciesIndex);
      double maxRelErr{0};
      double avgRelErr{0};
      std::size_t count{0};
      for (std::size_t i = 0; i < slow->getCompartment()->nVoxels(); ++i) {
        const auto &v{slow->getCompartment()->getVoxel(i)};
        // only check part within a radius of 16 units from centre to avoid
        // boundary effects: analytic solution is in infinite volume
        auto physicalPoint{s.getGeometry().getPhysicalPoint(v)};
        if (std::pow(physicalPoint.p.x(), 2) +
                std::pow(physicalPoint.p.y(), 2) +
                std::pow(physicalPoint.z, 2) <
            16 * 16) {
          double c_analytic =
              analytic_3d(physicalPoint, t, D[speciesIndex], t0);
          double relErr = std::abs(conc[i] - c_analytic) / c_analytic;
          avgRelErr += relErr;
          ++count;
          maxRelErr = std::max(maxRelErr, relErr);
        }
      }
      avgRelErr /= static_cast<double>(count);
      CAPTURE(simType);
      CAPTURE(t);
      REQUIRE(maxRelErr < evolvedMaxRelativeError);
      REQUIRE(avgRelErr < evolvedAvgRelativeError);
    }
  }
}

TEST_CASE(
    "Simulate: small-single-compartment-diffusion, circular geometry",
    "[core/simulate/simulate][core/simulate][core][simulate][dune][pixel]") {
  SECTION("many steps: both species end up equally & uniformly distributed") {
    double epsilon = 1e-3;
    auto s{getTestModel("small-single-compartment-diffusion")};
    auto &options{s.getSimulationSettings().options};
    options.pixel.maxErr = {std::numeric_limits<double>::max(), 0.001};
    options.pixel.maxThreads = 2;
    options.dune.dt = 0.5;
    for (auto simulator :
         {simulate::SimulatorType::DUNE, simulate::SimulatorType::Pixel}) {
      s.getSimulationData().clear();
      s.getSimulationSettings().simulatorType = simulator;

      auto sim = simulate::Simulation(s);
      REQUIRE(sim.getIsRunning() == false);
      REQUIRE(sim.getIsStopping() == false);
      REQUIRE(sim.getNCompletedTimesteps() == 1);
      // run simulation in another thread
      auto simSteps =
          std::async(std::launch::async, &simulate::Simulation::doTimesteps,
                     &sim, 50.0, 1, -1.0);
      // this `.get()` blocks until simulation is finished
      REQUIRE(simSteps.get() >= 1);
      REQUIRE(sim.getIsRunning() == false);
      REQUIRE(sim.getIsStopping() == false);
      REQUIRE(sim.getNCompletedTimesteps() == 2);
      auto timeIndex = sim.getTimePoints().size() - 1;
      // after many steps in a finite volume, diffusion has reached the limiting
      // case of a uniform distribution
      std::size_t speciesIndex = 0;
      for (const auto &species : {"slow", "fast"}) {
        auto conc = sim.getAvgMinMax(timeIndex, 0, speciesIndex++);
        CAPTURE(simulator);
        CAPTURE(species);
        CAPTURE(conc.min);
        CAPTURE(conc.avg);
        CAPTURE(conc.max);
        REQUIRE(std::abs((conc.min - conc.avg) / conc.avg) < epsilon);
        REQUIRE(std::abs((conc.max - conc.avg) / conc.avg) < epsilon);
      }
    }
  }
}

TEST_CASE("Pixel simulator: timeout",
          "[core/simulate/simulate][core/simulate][core][simulate][pixel]") {
  auto s{getExampleModel(Mod::Brusselator)};
  s.getSimulationSettings().simulatorType = simulate::SimulatorType::Pixel;
  // simulate 1 very long time step, with 200ms timeout
  simulate::Simulation sim(s);
  REQUIRE(sim.getNCompletedTimesteps() == 1);
  sim.doTimesteps(1e9, 1, 200.0);
  // step does not complete:
  REQUIRE(sim.getNCompletedTimesteps() == 1);

  // simulate many tiny time steps, with 200ms timeout
  s.getSimulationData().clear();
  simulate::Simulation sim2(s);
  REQUIRE(sim2.getNCompletedTimesteps() == 1);
  sim2.doTimesteps(1e-12, 100000, 200.0);
  // some steps complete before timeout:
  REQUIRE(sim2.getNCompletedTimesteps() > 1);
}

TEST_CASE("Pixel simulator: brusselator model, RK2, RK3, RK4",
          "[core/simulate/simulate][core/simulate][core][simulate][pixel]") {
  double eps{1e-20};
  double time{30.0};
  double maxAllowedRelErr{0.01};
  auto s{getExampleModel(Mod::Brusselator)};
  // do accurate simulation
  auto &options{s.getSimulationSettings().options};
  options.pixel.maxErr = {std::numeric_limits<double>::max(), 1e-6};
  options.pixel.maxThreads = 2;
  options.pixel.integrator = simulate::PixelIntegratorType::RK435;
  s.getSimulationSettings().simulatorType = simulate::SimulatorType::Pixel;

  simulate::Simulation sim(s);
  sim.doTimesteps(time);
  auto c4_accurate = sim.getConc(sim.getTimePoints().size() - 1, 0, 0);
  // check lower accuracy & different orders are consistent
  for (auto integrator : {simulate::PixelIntegratorType::RK212,
                          simulate::PixelIntegratorType::RK323,
                          simulate::PixelIntegratorType::RK435}) {
    // single threaded simulation
    double maxRelDiff = 0;
    options.pixel.integrator = integrator;
    options.pixel.enableMultiThreading = false;
    options.pixel.maxErr = {std::numeric_limits<double>::max(),
                            maxAllowedRelErr};
    s.getSimulationData().clear();
    simulate::Simulation sim2(s);
    sim2.doTimesteps(time);
    auto conc = sim2.getConc(sim.getTimePoints().size() - 1, 0, 0);
    for (std::size_t i = 0; i < conc.size(); ++i) {
      maxRelDiff = std::max(maxRelDiff, (conc[i] - c4_accurate[i]) /
                                            (c4_accurate[i] + eps));
    }
    // multi-threaded simulation
    options.pixel.enableMultiThreading = true;
    s.getSimulationData().clear();
    simulate::Simulation sim2_multi(s);
    sim2_multi.doTimesteps(time);
    auto conc_multi = sim2_multi.getConc(sim.getTimePoints().size() - 1, 0, 0);
    double absThreadingDiff{0};
    for (std::size_t i = 0; i < conc.size(); ++i) {
      absThreadingDiff += std::abs(conc[i] - conc_multi[i]);
    }
    absThreadingDiff /= static_cast<double>(conc.size());
    CAPTURE(integrator);
    // single threaded should agree with high order simulation within tolerance
    REQUIRE(maxRelDiff < maxAllowedRelErr);
    // single / multithreaded should agree to almost machine precision
    REQUIRE(absThreadingDiff < 1e-13);
  }
}

TEST_CASE("DUNE: simulation",
          "[core/simulate/simulate][core/simulate][core][simulate][dune]") {
  SECTION("ABtoC model") {
    auto s{getExampleModel(Mod::ABtoC)};

    // set spatially constant initial conditions
    s.getSpecies().setInitialConcentration("A", 1.0);
    s.getSpecies().setInitialConcentration("B", 1.0);
    s.getSpecies().setInitialConcentration("C", 0.0);

    auto &options{s.getSimulationSettings().options};
    options.dune.dt = 0.01;
    options.dune.maxDt = 0.01;
    options.dune.minDt = 0.001;
    options.dune.integrator = "Alexander2";
    s.getSimulationSettings().simulatorType = simulate::SimulatorType::DUNE;

    simulate::Simulation duneSim(s);
    REQUIRE(duneSim.getAvgMinMax(0, 0, 0).avg == dbl_approx(1.0));
    REQUIRE(duneSim.getAvgMinMax(0, 0, 1).avg == dbl_approx(1.0));
    REQUIRE(duneSim.getAvgMinMax(0, 0, 2).avg == dbl_approx(0.0));
    duneSim.doTimesteps(0.05);
    CAPTURE(duneSim.getTimePoints().size());
    auto timeIndex = duneSim.getTimePoints().size() - 1;
    auto imgConc = duneSim.getConcImage(timeIndex);
    REQUIRE(std::abs(duneSim.getAvgMinMax(timeIndex, 0, 0).avg - 0.995) < 1e-4);
    REQUIRE(std::abs(duneSim.getAvgMinMax(timeIndex, 0, 1).avg - 0.995) < 1e-4);
    REQUIRE(std::abs(duneSim.getAvgMinMax(timeIndex, 0, 2).avg - 0.005) < 1e-4);
    REQUIRE(imgConc.volume().depth() == 1);
    REQUIRE(imgConc[0].size() == QSize(100, 100));
  }
  SECTION("very-simple-model: different linearSolvers") {
    auto m{getExampleModel(Mod::VerySimpleModel)};
    m.getSimulationSettings().simulatorType = simulate::SimulatorType::DUNE;
    auto &options{m.getSimulationSettings().options};
    options.dune.dt = 0.01;
    SECTION("invalid: IDontExist") {
      options.dune.linearSolver = "IDontExist";
      simulate::Simulation duneSim(m);
      duneSim.doTimesteps(0.01);
      REQUIRE_THAT(duneSim.errorMessage(),
                   ContainsSubstring("not a known inverse operator type"));
    }
    SECTION(
        "valid & available but only works for positive-definite models: CG") {
      options.dune.linearSolver = "CG";
      simulate::Simulation duneSim(m);
      duneSim.doTimesteps(0.01);
      // simulation may not converge as CG is not a suitable solver for a
      // nonlinear model, but should not have a "solver not available" error
      REQUIRE_THAT(duneSim.errorMessage(), !ContainsSubstring("not available"));
    }
    SECTION("valid & available: RestartedGMRes") {
      options.dune.linearSolver = "RestartedGMRes";
      simulate::Simulation duneSim(m);
      duneSim.doTimesteps(0.01);
      REQUIRE(duneSim.errorMessage().empty());
    }
    SECTION("valid & available: BiCGSTAB") {
      options.dune.linearSolver = "BiCGSTAB";
      simulate::Simulation duneSim(m);
      duneSim.doTimesteps(0.01);
      REQUIRE(duneSim.errorMessage().empty());
    }
    SECTION("valid but unavailable: UMFPack") {
      options.dune.linearSolver = "UMFPack";
      simulate::Simulation duneSim(m);
      duneSim.doTimesteps(0.01);
      REQUIRE_THAT(duneSim.errorMessage(),
                   ContainsSubstring("not a known inverse operator type"));
    }
    SECTION("valid but unavailable: SuperLU") {
      options.dune.linearSolver = "SuperLU";
      simulate::Simulation duneSim(m);
      duneSim.doTimesteps(0.01);
      REQUIRE_THAT(duneSim.errorMessage(),
                   ContainsSubstring("not a known inverse operator type"));
    }
  }
}

TEST_CASE("getConcImage",
          "[core/simulate/simulate][core/simulate][core][simulate]") {
  SECTION("very-simple-model") {
    auto s{getExampleModel(Mod::VerySimpleModel)};
    s.getSimulationSettings().simulatorType = simulate::SimulatorType::Pixel;
    simulate::Simulation sim(s);
    sim.doTimesteps(0.5);
    sim.doTimesteps(0.5);
    REQUIRE(sim.errorMessage().empty());

    // draw no species, any normalisation
    for (auto allTime : {true, false}) {
      for (auto allSpecies : {true, false}) {
        auto img0{sim.getConcImage(1, {{}, {}, {}}, allTime, allSpecies)[0]};
        REQUIRE(img0.pixel(48, 43) == qRgb(0, 0, 0));
        REQUIRE(img0.pixel(49, 43) == qRgb(0, 0, 0));
        REQUIRE(img0.pixel(33, 8) == qRgb(0, 0, 0));
      }
    }

    // draw B_out species only
    for (auto allSpecies : {true, false}) {
      // this timepoint
      auto img1a{sim.getConcImage(1, {{0}, {}, {}}, false, allSpecies)[0]};
      REQUIRE(img1a.pixel(48, 43) == qRgb(0, 0, 0));
      REQUIRE(img1a.pixel(49, 43) == qRgb(0, 0, 0));
      REQUIRE(img1a.pixel(33, 8) == qRgb(0, 0, 0));
      REQUIRE(img1a.pixel(59, 33) == qRgb(145, 30, 180));

      // all timepoints
      auto img1b{sim.getConcImage(1, {{0}, {}, {}}, true, allSpecies)[0]};
      REQUIRE(img1b.pixel(48, 43) == qRgb(0, 0, 0));
      REQUIRE(img1b.pixel(49, 43) == qRgb(0, 0, 0));
      REQUIRE(img1b.pixel(33, 8) == qRgb(0, 0, 0));
      REQUIRE(img1b.pixel(33, 8) == qRgb(0, 0, 0));
    }

    // draw all species, normalise to max of each species, at this timepoint
    auto img1{sim.getConcImage(1, {}, false, false)[0]};
    REQUIRE(img1.pixel(48, 43) == qRgb(255, 255, 225));
    REQUIRE(img1.pixel(49, 43) == qRgb(245, 130, 48));
    REQUIRE(img1.pixel(33, 8) == qRgb(47, 142, 59));

    // draw all species, normalise to max of all species, at this timepoint
    auto img3{sim.getConcImage(1, {}, false, true)[0]};
    REQUIRE(img3.pixel(48, 43) == qRgb(0, 0, 0));
    REQUIRE(img3.pixel(49, 43) == qRgb(0, 0, 0));
    REQUIRE(img3.pixel(33, 8) == qRgb(47, 142, 59));

    // draw all species, normalise to max of each/all species, at all timepoints
    for (auto allSpecies : {true, false}) {
      auto img2{sim.getConcImage(1, {}, true, allSpecies)[0]};
      REQUIRE(img2.pixel(48, 43) == qRgb(0, 0, 0));
      REQUIRE(img2.pixel(49, 43) == qRgb(0, 0, 0));
      REQUIRE(img2.pixel(33, 8) == qRgb(31, 93, 39));
    }
  }
}

TEST_CASE("PyConc",
          "[core/simulate/simulate][core/simulate][core][simulate][python]") {
  SECTION("ABtoC model") {
    auto s{getExampleModel(Mod::ABtoC)};
    for (auto simType :
         {simulate::SimulatorType::Pixel, simulate::SimulatorType::DUNE}) {
      s.getSimulationSettings().simulatorType = simType;
      s.getSimulationData().clear();
      simulate::Simulation sim(s);
      sim.doTimesteps(0.01, 1);
      REQUIRE(sim.getTimePoints().size() == 2);
      REQUIRE(sim.getPyNames(0).size() == 3);
      REQUIRE(sim.getPyNames(0)[0] == "A");
      REQUIRE(sim.getPyNames(0)[1] == "B");
      REQUIRE(sim.getPyNames(0)[2] == "C");
      auto pyConcs0{sim.getPyConcs(0, 0)};
      REQUIRE(pyConcs0.size() == 3);
      const auto &cA0 = pyConcs0[0];
      REQUIRE(cA0.size() == 100 * 100);
      REQUIRE(cA0[0] == dbl_approx(0.0));

      auto pyConcs1{sim.getPyConcs(1, 0)};
      REQUIRE(pyConcs1.size() == 3);
      const auto &cA1{pyConcs1[0]};
      REQUIRE(cA1.size() == 100 * 100);
      REQUIRE(cA1[0] == dbl_approx(0.0));

      // onlh have dcdt for last timepoint of pixel sim:
      auto pyDcdts1{sim.getPyDcdts(0)};
      if (simType == simulate::SimulatorType::Pixel) {
        REQUIRE(pyDcdts1.size() == 3);
        const auto &dA1 = pyDcdts1[0];
        REQUIRE(dA1.size() == 100 * 100);
        REQUIRE(dA1[0] == dbl_approx(0.0));
      } else {
        REQUIRE(pyDcdts1.size() == 0);
      }
    }
  }
}

static double rel_diff(const simulate::SimulationData &a,
                       const simulate::SimulationData &b, std::size_t iTimeA,
                       std::size_t iTimeB) {
  double d{0.0};
  double n{0.0};
  for (std::size_t iC = 0; iC < a.concentration[iTimeA].size(); ++iC) {
    const auto &cA{a.concentration[iTimeA][iC]};
    const auto &cB{b.concentration[iTimeB][iC]};
    // normalise to max conc over all species & points in each compartment
    double norm{*std::max_element(cA.cbegin(), cA.cend())};
    if (norm == 0.0) {
      // if norm is exactly zero, use the other data
      norm = *std::max_element(cB.cbegin(), cB.cend());
      // if this is also zero, then all elements are exactly zero in both
      if (norm == 0.0) {
        return 0;
      }
    }
    n += static_cast<double>(cA.size());
    for (std::size_t timeIndex = 0; timeIndex < cA.size(); ++timeIndex) {
      CAPTURE(timeIndex);
      d += std::abs(cA[timeIndex] - cB[timeIndex]) / norm;
    }
  }
  return d / n;
}

static double rel_diff(const std::vector<double> &a,
                       const std::vector<double> &b) {
  double normA{*std::max_element(a.cbegin(), a.cend())};
  double normB{*std::max_element(b.cbegin(), b.cend())};
  double norm = std::max(normA, normB);
  if (norm == 0.0) {
    norm = 1e-14;
  }
  double n{static_cast<double>(a.size())};
  double d{0.0};
  for (std::size_t i = 0; i < a.size(); ++i) {
    d += std::abs(a[i] - b[i]);
  }
  return d / n / norm;
}

TEST_CASE("applyConcsToModel initial concentrations",
          "[core/simulate/simulate][core/simulate][core][simulate]") {
  auto s{getExampleModel(Mod::VerySimpleModel)};
  // set various types of initial concentrations
  s.getSpecies().setInitialConcentration("B_c1", 0.66);
  s.getSpecies().setInitialConcentration("A_c3", 0.123);
  s.getSpecies().setAnalyticConcentration("A_c2", "cos(x/5)+2");
  s.getSpecies().setInitialConcentration("B_c3", 1.0);
  auto c{s.getSpecies().getField("B_c3")->getConcentrationImageArray()};
  for (std::size_t i = 0; i < c.size(); ++i) {
    c[i] = 0.78 * static_cast<double>(i);
  }
  s.getSpecies().setSampledFieldConcentration("B_c3", c);

  // set up simulation of model
  s.getSimulationSettings().simulatorType = simulate::SimulatorType::Pixel;
  simulate::Simulation sim(s);
  sim.doTimesteps(0.01);

  // apply simulation initial concs to a copy of the original model
  auto s2{getExampleModel(Mod::VerySimpleModel)};
  sim.applyConcsToModel(s2, 0);
  // check concentrations match
  for (const auto &cId : s.getCompartments().getIds()) {
    for (const auto &sId : s.getSpecies().getIds(cId)) {
      auto spec1{s.getSpecies().getSampledFieldConcentration(sId)};
      auto spec2{s2.getSpecies().getSampledFieldConcentration(sId)};
      REQUIRE(spec1.size() == spec2.size());
      REQUIRE(rel_diff(spec1, spec2) == dbl_approx(0.0));
    }
  }
}

TEST_CASE("applyConcsToModel after simulation",
          "[core/simulate/simulate][core/simulate][core][simulate]") {
  auto s{getExampleModel(Mod::VerySimpleModel)};
  s.getSimulationSettings().simulatorType = simulate::SimulatorType::Pixel;
  s.exportSBMLFile("tmpsimapplyconcs.xml");
  auto &options{s.getSimulationSettings().options};
  options.dune.dt = 0.01;
  // apply initial concs from sim to model s, check they agree with copy of
  // model s2 (note any analytic initial concs in s are replaced with sampled
  // fields)
  model::Model s2;
  s2.importSBMLFile("tmpsimapplyconcs.xml");
  s2.getSimulationSettings().options = options;
  s2.getSimulationSettings().simulatorType = simulate::SimulatorType::Pixel;

  simulate::Simulation sim(s);
  sim.applyConcsToModel(s, 0);
  for (const auto &cId : s.getCompartments().getIds()) {
    for (const auto &sId : s.getSpecies().getIds(cId)) {
      auto spec1{s.getSpecies().getSampledFieldConcentration(sId)};
      auto spec2{s2.getSpecies().getSampledFieldConcentration(sId)};
      REQUIRE(spec1.size() == spec2.size());
      REQUIRE(rel_diff(spec1, spec2) == dbl_approx(0.0));
    }
  }
  // 0 -> 0.01 -> 0.02 sim in model s
  sim.doTimesteps(0.01, 2);
  REQUIRE(sim.getNCompletedTimesteps() == 3);
  // do t=0.01 sim2 using model s2
  simulate::Simulation sim2(s2);
  sim2.doTimesteps(0.01);
  REQUIRE(sim2.getNCompletedTimesteps() == 2);
  // apply concs at t=0.01 to model
  sim2.applyConcsToModel(s2, 1);
  // reset sim data
  s2.getSimulationData() = {};
  // create new sim3 from model s2
  simulate::Simulation sim3(s2);
  // sim3 at t=0 should equal sim at t=0.01
  for (std::size_t iComp = 0; iComp < sim3.getCompartmentIds().size();
       ++iComp) {
    const auto &specIds = sim3.getSpeciesIds(iComp);
    for (std::size_t iSpec = 0; iSpec < specIds.size(); ++iSpec) {
      auto concSim{sim.getConcArray(1, iComp, iSpec)};
      auto concSim3{sim3.getConcArray(0, iComp, iSpec)};
      REQUIRE(concSim.size() == concSim3.size());
      REQUIRE(rel_diff(concSim, concSim3) == dbl_approx(0.0));
    }
  }
  // simulate sim3 for t=0.01, now it should equal t=0.02 sim of original
  // model note: should differ by O(dt^2), not by machine epsilon, since for
  // the first step both simulations started from the same data and took the
  // same timestep, but for the second step it is a new simulation, so
  // timesteps will differ
  sim3.doTimesteps(0.01);
  REQUIRE(sim3.getNCompletedTimesteps() == 2);
  for (std::size_t iComp = 0; iComp < sim3.getCompartmentIds().size();
       ++iComp) {
    const auto &specIds = sim3.getSpeciesIds(iComp);
    for (std::size_t iSpec = 0; iSpec < specIds.size(); ++iSpec) {
      auto concSim{sim.getConcArray(2, iComp, iSpec)};
      auto concSim3{sim3.getConcArray(1, iComp, iSpec)};
      REQUIRE(concSim.size() == concSim3.size());
      REQUIRE(rel_diff(concSim, concSim3) < 5e-7);
    }
  }
}

TEST_CASE("Reactions depend on x, y, t",
          "[core/simulate/simulate][core/simulate][core][simulate]") {
  auto s{getTestModel("txy")};
  constexpr double eps{1e-20};
  constexpr double dt{1e-3};
  auto &options{s.getSimulationSettings().options};
  s.getSimulationSettings().simulatorType = simulate::SimulatorType::Pixel;
  options.dune.dt = dt;
  options.pixel.integrator = simulate::PixelIntegratorType::RK101;
  options.pixel.maxErr = {std::numeric_limits<double>::max(),
                          std::numeric_limits<double>::max()};
  options.pixel.maxTimestep = dt;
  SECTION("reaction with t-dependence") {
    // fairly tight tolerance, as solution is spatially uniform, so mesh vs
    // pixel geometry is not a factor when comparing Pixel and Dune here
    constexpr double maxAllowedAbsDiff{1e-10};
    constexpr double maxAllowedRelDiff{1e-7};
    s.getSpecies().remove("A");
    s.getSpecies().remove("B");
    simulate::Simulation simPixel{s};
    simPixel.doTimesteps(dt, 1);
    REQUIRE(simPixel.errorMessage().empty());
    REQUIRE(simPixel.getNCompletedTimesteps() == 2);
    s.getSimulationData().clear();
    s.getSimulationSettings().simulatorType = simulate::SimulatorType::DUNE;
    simulate::Simulation simDune{s};
    simDune.doTimesteps(dt, 1);
    CAPTURE(simDune.errorMessage());
    REQUIRE(simDune.errorMessage().empty());
    REQUIRE(simDune.getNCompletedTimesteps() == 2);
    auto p{simPixel.getConc(1, 0, 0)};
    auto d{simDune.getConc(1, 0, 0)};
    REQUIRE(p.size() == d.size());
    double maxAbsDiff{0};
    double maxRelDiff{0};
    for (std::size_t i = 0; i < p.size(); ++i) {
      maxAbsDiff = std::max(maxAbsDiff, std::abs(p[i] - d[i]));
      maxRelDiff = std::max(maxRelDiff, std::abs(p[i] - d[i]) /
                                            (std::abs(p[i] + d[i] + eps)));
    }
    CAPTURE(maxAbsDiff);
    CAPTURE(maxRelDiff);
    REQUIRE(maxAbsDiff < maxAllowedAbsDiff);
    REQUIRE(maxRelDiff < maxAllowedRelDiff);
  }
  SECTION("reaction with x,y-dependence") {
    // looser tolerance: mesh distorts results
    constexpr double maxAllowedAbsDiff{0.002};
    constexpr double maxAllowedRelDiff{0.03};
    s.getSpecies().remove("C");
    s.getSimulationSettings().simulatorType = simulate::SimulatorType::Pixel;
    simulate::Simulation simPixel{s};
    simPixel.doTimesteps(dt, 1);
    REQUIRE(simPixel.errorMessage().empty());
    REQUIRE(simPixel.getNCompletedTimesteps() == 2);
    s.getSimulationData().clear();
    s.getSimulationSettings().simulatorType = simulate::SimulatorType::DUNE;
    simulate::Simulation simDune{s};
    simDune.doTimesteps(dt, 1);
    REQUIRE(simDune.errorMessage().empty());
    REQUIRE(simDune.getNCompletedTimesteps() == 2);
    for (std::size_t speciesIndex = 0; speciesIndex < 2; ++speciesIndex) {
      auto p{simPixel.getConc(1, 0, speciesIndex)};
      auto d{simDune.getConc(1, 0, speciesIndex)};
      REQUIRE(p.size() == d.size());
      double maxAbsDiff{0};
      double maxRelDiff{0};
      for (std::size_t i = 0; i < p.size(); ++i) {
        maxAbsDiff = std::max(maxAbsDiff, std::abs(p[i] - d[i]));
        maxRelDiff = std::max(maxRelDiff, std::abs(p[i] - d[i]) /
                                              (std::abs(p[i] + d[i] + eps)));
      }
      CAPTURE(speciesIndex);
      CAPTURE(maxAbsDiff);
      CAPTURE(maxRelDiff);
      REQUIRE(maxAbsDiff < maxAllowedAbsDiff);
      REQUIRE(maxRelDiff < maxAllowedRelDiff);
    }
  }
  SECTION("reaction with t,x,y-dependence") {
    // looser tolerance: mesh distorts results
    constexpr double maxAllowedAbsDiff{0.002};
    constexpr double maxAllowedRelDiff{0.03};
    s.getSimulationSettings().simulatorType = simulate::SimulatorType::Pixel;
    simulate::Simulation simPixel{s};
    simPixel.doTimesteps(dt, 1);
    REQUIRE(simPixel.errorMessage().empty());
    REQUIRE(simPixel.getNCompletedTimesteps() == 2);
    s.getSimulationData().clear();
    s.getSimulationSettings().simulatorType = simulate::SimulatorType::DUNE;
    simulate::Simulation simDune{s};
    simDune.doTimesteps(dt, 1);
    REQUIRE(simDune.errorMessage().empty());
    REQUIRE(simDune.getNCompletedTimesteps() == 2);
    for (std::size_t speciesIndex = 0; speciesIndex < 3; ++speciesIndex) {
      auto p{simPixel.getConc(1, 0, speciesIndex)};
      auto d{simDune.getConc(1, 0, speciesIndex)};
      REQUIRE(p.size() == d.size());
      double maxAbsDiff{0};
      double maxRelDiff{0};
      for (std::size_t i = 0; i < p.size(); ++i) {
        maxAbsDiff = std::max(maxAbsDiff, std::abs(p[i] - d[i]));
        maxRelDiff = std::max(maxRelDiff, std::abs(p[i] - d[i]) /
                                              (std::abs(p[i] + d[i] + eps)));
      }
      CAPTURE(speciesIndex);
      CAPTURE(maxAbsDiff);
      CAPTURE(maxRelDiff);
      REQUIRE(maxAbsDiff < maxAllowedAbsDiff);
      REQUIRE(maxRelDiff < maxAllowedRelDiff);
    }
  }
}

TEST_CASE("circle membrane reaction",
          "[core/simulate/simulate][core/"
          "simulate][core][simulate][dune][pixel][expensive]") {
  auto mDune{getTestModel("membrane-reaction-circle")};
  auto mPixel{getTestModel("membrane-reaction-circle")};
  mDune.getSimulationSettings().simulatorType = simulate::SimulatorType::DUNE;
  simulate::Simulation simDune(mDune);
  mPixel.getSimulationSettings().simulatorType = simulate::SimulatorType::Pixel;
  simulate::Simulation simPixel(mPixel);
  REQUIRE(simDune.getAvgMinMax(0, 1, 0).avg == dbl_approx(0.0));
  REQUIRE(simPixel.getAvgMinMax(0, 1, 0).avg == dbl_approx(0.0));
  simDune.doTimesteps(0.25);
  simPixel.doTimesteps(0.25);
  REQUIRE(std::abs(simPixel.getAvgMinMax(1, 1, 0).avg -
                   simDune.getAvgMinMax(1, 1, 0).avg) /
              std::abs(simPixel.getAvgMinMax(1, 1, 0).avg +
                       simDune.getAvgMinMax(1, 1, 0).avg) <
          0.2);
  auto p{simPixel.getConc(1, 1, 0)};
  auto d{simDune.getConc(1, 1, 0)};
  REQUIRE(p.size() == d.size());
  double avgAbsDiff{0};
  double avgRelDiff{0};
  constexpr double cutoff{1e-1};
  double allowedAvgAbsDiff{0.8};
  double allowedAvgRelDiff{0.4};
  std::size_t n_compared_pixels{0};
  for (std::size_t i = 0; i < p.size(); ++i) {
    if (p[i] > cutoff) {
      avgAbsDiff += std::abs(p[i] - d[i]);
      avgRelDiff += std::abs(p[i] - d[i]) / (std::abs(p[i] + d[i]));
      ++n_compared_pixels;
    }
  }
  avgAbsDiff /= static_cast<double>(n_compared_pixels);
  avgRelDiff /= static_cast<double>(n_compared_pixels);
  CAPTURE(p.size());
  CAPTURE(n_compared_pixels);
  CAPTURE(avgAbsDiff);
  CAPTURE(avgRelDiff);
  REQUIRE(avgAbsDiff < allowedAvgAbsDiff);
  REQUIRE(avgRelDiff < allowedAvgRelDiff);
}

TEST_CASE(
    "pair of pixels membrane reaction",
    "[core/simulate/simulate][core/simulate][core][simulate][dune][pixel]") {
  auto mDune{getTestModel("membrane-reaction-pixels")};
  auto mPixel{getTestModel("membrane-reaction-pixels")};
  mDune.getSimulationSettings().simulatorType = simulate::SimulatorType::DUNE;
  simulate::Simulation simDune(mDune);
  mPixel.getSimulationSettings().simulatorType = simulate::SimulatorType::Pixel;
  simulate::Simulation simPixel(mPixel);
  REQUIRE(simDune.getAvgMinMax(0, 1, 0).avg == dbl_approx(0.0));
  REQUIRE(simPixel.getAvgMinMax(0, 1, 0).avg == dbl_approx(0.0));
  simDune.doTimesteps(5);
  simPixel.doTimesteps(5);
  REQUIRE(std::abs(simPixel.getAvgMinMax(1, 1, 0).avg -
                   simDune.getAvgMinMax(1, 1, 0).avg) /
              std::abs(simPixel.getAvgMinMax(1, 1, 0).avg +
                       simDune.getAvgMinMax(1, 1, 0).avg) <
          0.005);
  auto p{simPixel.getConc(1, 1, 0)};
  auto d{simDune.getConc(1, 1, 0)};
  REQUIRE(p.size() == d.size());
  double avgAbsDiff{0};
  double avgRelDiff{0};
  constexpr double eps{1e-15};
  double allowedAvgAbsDiff{0.5};
  double allowedAvgRelDiff{0.005};
  for (std::size_t i = 0; i < p.size(); ++i) {
    avgAbsDiff += std::abs(p[i] - d[i]);
    avgRelDiff += std::abs(p[i] - d[i]) / (std::abs(p[i] + d[i] + eps));
  }
  avgAbsDiff /= static_cast<double>(p.size());
  avgRelDiff /= static_cast<double>(p.size());
  CAPTURE(avgAbsDiff);
  CAPTURE(avgRelDiff);
  REQUIRE(avgAbsDiff < allowedAvgAbsDiff);
  REQUIRE(avgRelDiff < allowedAvgRelDiff);
}

TEST_CASE("SimulationData",
          "[core/simulate/simulate][core/simulate][core][simulate]") {
  auto s{getExampleModel(Mod::VerySimpleModel)};
  s.getSpecies().setAnalyticConcentration("B_c1", "cos(x/14.2)+1");
  s.getSpecies().setAnalyticConcentration("A_c2", "cos(x/12.2)+1");
  s.getSpecies().setAnalyticConcentration("B_c2", "cos(x/15.1)+1");
  s.getSpecies().setAnalyticConcentration("A_c3", "cos(x/7.2)+1");
  s.getSpecies().setAnalyticConcentration("B_c3", "cos(x/5.2)+1");
  SECTION("Continue previous simulation from data") {
    auto &options{s.getSimulationSettings().options};
    s.getSimulationSettings().simulatorType = simulate::SimulatorType::Pixel;
    options.pixel.maxErr.rel = 1e-2;
    simulate::Simulation sim(s);
    // sim: 0.00 -> 0.01 -> 0.02 -> 0.03 -> 0.04 -> 0.05
    sim.doTimesteps(0.01, 5);
    REQUIRE(sim.getNCompletedTimesteps() == 6);
    REQUIRE(sim.getTimePoints()[0] == dbl_approx(0.00));
    REQUIRE(sim.getTimePoints()[1] == dbl_approx(0.01));
    REQUIRE(sim.getTimePoints()[2] == dbl_approx(0.02));
    REQUIRE(sim.getTimePoints()[3] == dbl_approx(0.03));
    REQUIRE(sim.getTimePoints()[4] == dbl_approx(0.04));
    REQUIRE(sim.getTimePoints()[5] == dbl_approx(0.05));
    simulate::SimulationData data{sim.getSimulationData()};
    s.getSimulationData().clear();
    // simA: 0.00 -> 0.01 -> 0.02
    simulate::Simulation simA(s);
    simA.doTimesteps(0.01, 2);
    REQUIRE(simA.getNCompletedTimesteps() == 3);
    REQUIRE(simA.getTimePoints()[0] == dbl_approx(0.00));
    REQUIRE(simA.getTimePoints()[1] == dbl_approx(0.01));
    REQUIRE(simA.getTimePoints()[2] == dbl_approx(0.02));
    simulate::SimulationData dataA{simA.getSimulationData()};
    REQUIRE(dataA.timePoints.size() == 3);
    REQUIRE(dataA.timePoints[0] == dbl_approx(0.00));
    REQUIRE(dataA.timePoints[1] == dbl_approx(0.01));
    REQUIRE(dataA.timePoints[2] == dbl_approx(0.02));
    REQUIRE(dataA.avgMinMax.size() == 3);
    REQUIRE(dataA.concentrationMax.size() == 3);
    REQUIRE(dataA.concentration.size() == 3);
    // simA should match first three steps of sim
    REQUIRE(rel_diff(dataA, data, 0, 0) == dbl_approx(0));
    REQUIRE(rel_diff(dataA, data, 1, 1) == dbl_approx(0));
    REQUIRE(rel_diff(dataA, data, 2, 2) == dbl_approx(0));
    // create new simulation using existing data
    simulate::Simulation simB(s);
    const auto &dataB{simB.getSimulationData()};
    // simB: should be a copy of simA
    REQUIRE(simB.getNCompletedTimesteps() == 3);
    REQUIRE(simB.getTimePoints()[0] == dbl_approx(0.00));
    REQUIRE(simB.getTimePoints()[1] == dbl_approx(0.01));
    REQUIRE(simB.getTimePoints()[2] == dbl_approx(0.02));
    REQUIRE(rel_diff(dataB, data, 0, 0) == dbl_approx(0));
    REQUIRE(rel_diff(dataB, data, 1, 1) == dbl_approx(0));
    REQUIRE(rel_diff(dataB, data, 2, 2) == dbl_approx(0));
    // continue simB: 0.02 -> 0.03 -> 0.04 -> 0.05
    // simB results should now match sim
    simB.doTimesteps(0.01, 3);
    REQUIRE(simB.getNCompletedTimesteps() == 6);
    REQUIRE(simB.getTimePoints()[0] == dbl_approx(0.00));
    REQUIRE(simB.getTimePoints()[1] == dbl_approx(0.01));
    REQUIRE(simB.getTimePoints()[2] == dbl_approx(0.02));
    REQUIRE(simB.getTimePoints()[3] == dbl_approx(0.03));
    REQUIRE(simB.getTimePoints()[4] == dbl_approx(0.04));
    REQUIRE(simB.getTimePoints()[5] == dbl_approx(0.05));
    REQUIRE(dataB.timePoints.size() == 6);
    REQUIRE(dataB.timePoints[0] == dbl_approx(0.00));
    REQUIRE(dataB.timePoints[1] == dbl_approx(0.01));
    REQUIRE(dataB.timePoints[2] == dbl_approx(0.02));
    REQUIRE(dataB.timePoints[3] == dbl_approx(0.03));
    REQUIRE(dataB.timePoints[4] == dbl_approx(0.04));
    REQUIRE(dataB.timePoints[5] == dbl_approx(0.05));
    REQUIRE(dataB.avgMinMax.size() == 6);
    REQUIRE(dataB.concentrationMax.size() == 6);
    REQUIRE(dataB.concentration.size() == 6);
    REQUIRE(rel_diff(dataB, data, 0, 0) < 1e-14);
    REQUIRE(rel_diff(dataB, data, 1, 1) < 1e-14);
    REQUIRE(rel_diff(dataB, data, 2, 2) < 1e-14);
    // exactly, expected difference between simulations goes from being approx
    // note: at this point we start a new simulator, so timesteps don't match
    // machine precision to being approx the O(dt^2) integration error
    constexpr double allowedDifference{1e-7};
    REQUIRE(rel_diff(dataB, data, 3, 3) < allowedDifference);
    REQUIRE(rel_diff(dataB, data, 4, 4) < allowedDifference);
    REQUIRE(rel_diff(dataB, data, 5, 5) < allowedDifference);
  }
  SECTION("Repeat with smaller integration errors: difference reduced") {
    auto &options{s.getSimulationSettings().options};
    s.getSimulationSettings().simulatorType = simulate::SimulatorType::Pixel;
    options.pixel.maxErr.rel = 1e-5;
    simulate::Simulation sim(s);
    sim.doTimesteps(0.01, 5);
    simulate::SimulationData data{sim.getSimulationData()};
    s.getSimulationData().clear();
    simulate::Simulation simA(s);
    simA.doTimesteps(0.01, 2);
    simulate::Simulation simB(s);
    const auto &dataB{simB.getSimulationData()};
    simB.doTimesteps(0.01, 3);
    // first three timesteps match exactly
    REQUIRE(rel_diff(dataB, data, 0, 0) < 1e-14);
    REQUIRE(rel_diff(dataB, data, 1, 1) < 1e-14);
    REQUIRE(rel_diff(dataB, data, 2, 2) < 1e-14);
    // reduced integration error -> smaller difference between the two
    // simulations compared to previous test
    constexpr double allowedDifference{1e-12};
    REQUIRE(rel_diff(dataB, data, 3, 3) < allowedDifference);
    REQUIRE(rel_diff(dataB, data, 4, 4) < allowedDifference);
    REQUIRE(rel_diff(dataB, data, 5, 5) < allowedDifference);
  }
}

TEST_CASE("doMultipleTimesteps vs doTimesteps",
          "[core/simulate/simulate][core/simulate][core][simulate]") {
  auto m1{getExampleModel(Mod::VerySimpleModel)};
  m1.getSimulationSettings().simulatorType = simulate::SimulatorType::Pixel;
  simulate::Simulation sim1(m1);
  sim1.doTimesteps(0.1, 3);
  sim1.doTimesteps(0.2, 2);
  const auto &data1{m1.getSimulationData()};
  auto m2{getExampleModel(Mod::VerySimpleModel)};
  m2.getSimulationSettings().simulatorType = simulate::SimulatorType::Pixel;
  simulate::Simulation sim2(m2);
  sim2.doMultipleTimesteps({{3, 0.1}, {2, 0.2}});
  const auto &data2{m2.getSimulationData()};
  REQUIRE(data1.size() == data2.size());
  for (std::size_t i = 0; i < data1.size(); ++i) {
    REQUIRE(rel_diff(data1, data2, i, i) == dbl_approx(0.0));
  }
}

TEST_CASE("Events: setting species concentrations",
          "[core/simulate/simulate][core/simulate][core][simulate][events]") {
  auto m1{getExampleModel(Mod::VerySimpleModel)};
  for (auto simType :
       {simulate::SimulatorType::DUNE, simulate::SimulatorType::Pixel}) {
    CAPTURE(simType);
    // disable all reactions
    for (const auto &id : {"A_uptake", "A_transport", "A_B_conversion",
                           "B_transport", "B_excretion"}) {
      m1.getReactions().remove(id);
    }
    // add an absolute DUNE convergence tolerance to avoid simulation not
    // converging
    m1.getSimulationSettings().options.dune.newtonAbsErr = 1e-14;

    // add a bunch of events that set species concentrations so we can easily
    // check they were done correctly
    auto &events{m1.getEvents()};
    events.add("eB_c1a", "B_c1");
    // param = 1 in model
    events.setExpression("eB_c1a", "9*param");
    events.setTime("eB_c1a", 0.07);
    events.add("eB_c1b", "B_c1");
    events.setExpression("eB_c1b", "2");
    events.setTime("eB_c1b", 0.12);
    // change value of p
    events.add("change_p", "param");
    events.setExpression("change_p", "27");
    events.setTime("change_p", 0.11);
    events.add("eB_c2", "B_c2");
    events.setExpression("eB_c2", "param");
    events.setTime("eB_c2", 0.12);
    events.add("eA_c3a", "A_c3");
    events.setExpression("eA_c3a", "666");
    events.setTime("eA_c3a", 0.199999);
    events.add("eA_c3b", "A_c3");
    events.setExpression("eA_c3b", "123");
    // will be applied *during* 0.1->0.2 timestep: visible in 0.2 timestep
    events.setTime("eA_c3b", 0.1999999);
    events.add("eA_c2", "A_c2");
    events.setExpression("eA_c2", "6");
    // applied *at start of* 0.1-0.2 timestep: visible in 0.2 timestep
    events.setTime("eA_c2", 0.1);
    m1.getSimulationSettings().simulatorType = simType;
    simulate::Simulation sim(m1);
    sim.doTimesteps(0.1, 3);
    const auto &data{m1.getSimulationData()};
    // t=0.1
    REQUIRE(data.timePoints[1] == dbl_approx(0.1));
    // B_c1=9
    REQUIRE(data.concentration[1][0][3] == dbl_approx(9.0));
    REQUIRE(data.concentration[1][0][16] == dbl_approx(9.0));
    REQUIRE(data.concentration[1][0][38] == dbl_approx(9.0));
    // t=0.2
    REQUIRE(data.timePoints[2] == dbl_approx(0.2));
    // B_c1=2
    REQUIRE(data.concentration[2][0][3] == dbl_approx(2.0));
    REQUIRE(data.concentration[2][0][16] == dbl_approx(2.0));
    REQUIRE(data.concentration[2][0][38] == dbl_approx(2.0));
    // B_c2=27 (odd indices)
    REQUIRE(data.concentration[2][1][183] == dbl_approx(27));
    REQUIRE(data.concentration[2][1][197] == dbl_approx(27));
    REQUIRE(data.concentration[2][1][203] == dbl_approx(27));
    // A_c2=6 (even indices)
    REQUIRE(data.concentration[2][1][4] == dbl_approx(6.0));
    REQUIRE(data.concentration[2][1][16] == dbl_approx(6.0));
    REQUIRE(data.concentration[2][1][40] == dbl_approx(6.0));
    // A_c3=123 (even indices)
    REQUIRE(data.concentration[2][2][4] == dbl_approx(123.0));
    REQUIRE(data.concentration[2][2][16] == dbl_approx(123.0));
    REQUIRE(data.concentration[2][2][40] == dbl_approx(123.0));
  }
}

TEST_CASE("Events: continuing existing simulation",
          "[core/simulate/simulate][core/"
          "simulate][core][simulate][events][expensive]") {
  for (auto simType :
       {simulate::SimulatorType::DUNE, simulate::SimulatorType::Pixel}) {
    CAPTURE(simType);
    double allowedRelDiff{1e-5};
    if (simType == simulate::SimulatorType::DUNE) {
      // dunesim save/load of concentrations involves mesh -> pixels -> mesh
      // conversion, and we use a coarse mesh to avoid simulation taking too
      // long, so this results in fairly large errors:
      allowedRelDiff = 0.15;
    }
    auto m1{getExampleModel(Mod::Brusselator)};
    // make mesh coarser to speed up dune tests
    auto *mesh{m1.getGeometry().getMesh2d()};
    mesh->setBoundaryMaxPoints(0, 9);
    mesh->setCompartmentMaxTriangleArea(0, 999);
    // reduce DUNE max timestep to avoid simulation not converging
    m1.getSimulationSettings().options.dune.minDt = 1e-9;
    m1.getSimulationSettings().options.dune.dt = 0.005;
    m1.getSimulationSettings().options.dune.maxDt = 0.02;
    m1.getSimulationSettings().simulatorType = simType;
    simulate::Simulation s1(m1);
    s1.doTimesteps(20, 1);
    // t=20: no events so far
    m1.exportSMEFile("tmpsim_t20.sme");
    s1.doTimesteps(20, 1);
    // t=20: k2 increased
    m1.exportSMEFile("tmpsim_t40.sme");
    s1.doTimesteps(20, 1);
    // t=60: k2 decreased
    m1.exportSMEFile("tmpsim_t60.sme");
    s1.doTimesteps(20, 1);
    // s1 has sim data at t = 0, 20, 40, 60, 80
    const auto &d1{s1.getSimulationData()};
    REQUIRE(d1.size() == 5);
    REQUIRE(d1.timePoints.back() == dbl_approx(80.0));

    // load at t=20, simulate
    model::Model m20;
    m20.importFile("tmpsim_t20.sme");
    const auto &d20{m20.getSimulationData()};
    REQUIRE(d20.size() == 2);
    REQUIRE(d20.timePoints.back() == dbl_approx(20.0));
    simulate::Simulation s20(m20);
    s20.doTimesteps(20, 3);
    REQUIRE(d20.timePoints.back() == dbl_approx(80.0));
    REQUIRE(d20.size() == 5);
    // first two timepoints agree exactly
    REQUIRE(rel_diff(d1, d20, 0, 0) == dbl_approx(0.0));
    REQUIRE(rel_diff(d1, d20, 1, 1) == dbl_approx(0.0));
    // after that they differ by O(dt^2)
    REQUIRE(rel_diff(d1, d20, 2, 2) < allowedRelDiff);
    REQUIRE(rel_diff(d1, d20, 3, 3) < allowedRelDiff);
    REQUIRE(rel_diff(d1, d20, 4, 4) < allowedRelDiff);

    // load at t=40, simulate
    model::Model m40;
    m40.importFile("tmpsim_t40.sme");
    const auto &d40{m40.getSimulationData()};
    REQUIRE(d40.size() == 3);
    REQUIRE(d40.timePoints.back() == dbl_approx(40.0));
    simulate::Simulation s40(m40);
    s40.doTimesteps(20, 2);
    REQUIRE(d40.size() == 5);
    REQUIRE(d40.timePoints.back() == dbl_approx(80.0));
    // first three timepoints agree exactly
    REQUIRE(rel_diff(d1, d40, 0, 0) == dbl_approx(0.0));
    REQUIRE(rel_diff(d1, d40, 1, 1) == dbl_approx(0.0));
    REQUIRE(rel_diff(d1, d40, 2, 2) == dbl_approx(0.0));
    // after that they differ by O(dt^2)
    REQUIRE(rel_diff(d1, d40, 3, 3) < allowedRelDiff);
    REQUIRE(rel_diff(d1, d40, 4, 4) < allowedRelDiff);

    // load at t=60, simulate
    model::Model m60;
    m60.importFile("tmpsim_t60.sme");
    const auto &d60{m60.getSimulationData()};
    REQUIRE(d60.size() == 4);
    REQUIRE(d60.timePoints.back() == dbl_approx(60.0));
    simulate::Simulation s60(m60);
    s60.doTimesteps(20, 1);
    REQUIRE(d60.size() == 5);
    REQUIRE(d60.timePoints.back() == dbl_approx(80.0));
    // first four timepoints agree exactly
    REQUIRE(rel_diff(d1, d60, 0, 0) == dbl_approx(0.0));
    REQUIRE(rel_diff(d1, d60, 1, 1) == dbl_approx(0.0));
    REQUIRE(rel_diff(d1, d60, 2, 2) == dbl_approx(0.0));
    REQUIRE(rel_diff(d1, d60, 3, 3) == dbl_approx(0.0));
    // after that they differ by O(dt^2)
    REQUIRE(rel_diff(d1, d60, 4, 4) < allowedRelDiff);
  }
}

TEST_CASE(
    "simulate w/options & save, load, re-simulate",
    "[core/simulate/simulate][core/simulate][core][simulate][expensive]") {
  for (auto simulatorType :
       {simulate::SimulatorType::DUNE, simulate::SimulatorType::Pixel}) {
    CAPTURE(simulatorType);
    auto m1{getExampleModel(Mod::VerySimpleModel)};
    m1.getSimulationSettings().times.clear();
    auto &options1{m1.getSimulationSettings().options};
    options1.pixel.integrator = simulate::PixelIntegratorType::RK435;
    options1.pixel.maxErr.rel = 1e-3;
    options1.dune.dt = 0.009;
    options1.dune.increase = 1.44;
    m1.getSimulationSettings().simulatorType = simulatorType;
    simulate::Simulation sim1(m1);
    sim1.doMultipleTimesteps({{2, 0.01}, {1, 0.02}});
    const auto &data1{m1.getSimulationData()};
    REQUIRE(data1.concentration.size() == 4);
    m1.exportSMEFile("tmpsimoptions.sme");

    // clear simulation data, then do simulation, should regenerate same data
    model::Model m2;
    m2.importFile("tmpsimoptions.sme");
    REQUIRE(m2.getSimulationSettings().simulatorType == simulatorType);
    REQUIRE(m2.getSimulationSettings().options.pixel.integrator ==
            simulate::PixelIntegratorType::RK435);
    REQUIRE(m2.getSimulationSettings().options.pixel.maxErr.rel ==
            dbl_approx(1e-3));
    REQUIRE(m2.getSimulationSettings().options.dune.dt == dbl_approx(0.009));
    REQUIRE(m2.getSimulationSettings().options.dune.increase ==
            dbl_approx(1.44));
    REQUIRE(m2.getSimulationSettings().times.size() == 2);
    REQUIRE(m2.getSimulationData().concentration.size() == 4);
    auto times{m2.getSimulationSettings().times};
    m2.getSimulationData().clear();
    m2.getSimulationSettings().times.clear();
    REQUIRE(times.size() == 2);
    REQUIRE(m2.getSimulationData().concentration.size() == 0);
    REQUIRE(m2.getSimulationSettings().times.size() == 0);
    simulate::Simulation sim2(m2);
    sim2.doMultipleTimesteps(times);
    const auto &data2{m2.getSimulationData()};
    REQUIRE(data1.size() == data2.size());
    // single-threaded pixel sim is deterministic: same inputs give same outputs
    double allowedRelativeDifference = 1e-14;
    if (simulatorType == simulate::SimulatorType::DUNE) {
      // dune sim is multi-threaded so two runs with same inputs can differ
      allowedRelativeDifference = 1.e-4;
    }
    for (std::size_t i = 0; i < data1.size(); ++i) {
      REQUIRE(std::abs(rel_diff(data1, data2, i, i)) <=
              allowedRelativeDifference);
      REQUIRE(data1.timePoints[i] == dbl_approx(data2.timePoints[i]));
    }
  }
}

TEST_CASE("stop, then continue pixel simulation",
          "[core/simulate/simulate][core/simulate][core][simulate]") {
  // see
  // https://github.com/spatial-model-editor/spatial-model-editor/issues/544
  auto m{getExampleModel(Mod::VerySimpleModel)};
  m.getSimulationSettings().simulatorType = simulate::SimulatorType::Pixel;
  simulate::Simulation sim(m);
  // start simulation with single huge timestep in another thread
  auto simSteps =
      std::async(std::launch::async, &simulate::Simulation::doTimesteps, &sim,
                 5000.0, 1, -1.0);
  // ask it to stop early
  sim.requestStop();
  // this `.get()` blocks until simulation is finished
  REQUIRE(simSteps.get() >= 1);
  REQUIRE(m.getSimulationData().timePoints.size() == 1);
  REQUIRE(sim.getIsRunning() == false);
  REQUIRE(sim.getIsStopping() == false);
  // check we can do more timesteps
  sim.doMultipleTimesteps({{3, 0.01}});
  REQUIRE(m.getSimulationData().timePoints.size() == 4);
}

TEST_CASE("pixel simulation with invalid reaction rate expression",
          "[core/simulate/simulate][core/simulate][core][simulate]") {
  auto m{getExampleModel(Mod::ABtoC)};
  m.getReactions().add("r2", "comp", "A * A / idontexist");
  m.getReactions().setSpeciesStoichiometry("r2", "A", 1.0);
  m.getSimulationSettings().simulatorType = simulate::SimulatorType::Pixel;
  REQUIRE_THAT(simulate::Simulation(m).errorMessage(),
               ContainsSubstring("Unknown symbol 'idontexist'"));
}

TEST_CASE("Fish model: simulation with piecewise function in reactions",
          "[core/simulate/simulate][core/simulate][core][simulate][fish]") {
  // todo: make this test less trivial (#909)
  auto m{getTestModel("fish_300x300")};
  // pixel
  m.getSimulationSettings().simulatorType = simulate::SimulatorType::Pixel;
  simulate::Simulation simPixel(m);
  REQUIRE(simPixel.errorMessage() == "");
  REQUIRE(m.getSimulationData().timePoints.size() == 1);
  simPixel.doMultipleTimesteps({{2, 0.01}});
  REQUIRE(simPixel.errorMessage() == "");
  REQUIRE(m.getSimulationData().timePoints.size() == 3);
  // dune
  m.getSimulationSettings().simulatorType = simulate::SimulatorType::DUNE;
  m.getSimulationData().clear();
  simulate::Simulation simDune(m);
  REQUIRE(simDune.errorMessage() == "");
  REQUIRE(m.getSimulationData().timePoints.size() == 1);
  simDune.doMultipleTimesteps({{2, 0.00001}}); // use tiny timestep for now
  REQUIRE(simDune.errorMessage() == "");
  REQUIRE(m.getSimulationData().timePoints.size() == 3);
}

TEST_CASE("Simulate gray-scott-3d model",
          "[core/simulate/simulate][core/"
          "simulate][core][simulate][dune][pixel][3d]") {
  // todo: make this test less trivial
  SECTION("do a tiny step, don't crash") {
    auto s{getExampleModel(sme::test::Mod::GrayScott3D)};
    for (auto simulator :
         {simulate::SimulatorType::DUNE, simulate::SimulatorType::Pixel}) {
      s.getSimulationData().clear();
      s.getSimulationSettings().simulatorType = simulator;
      auto sim = simulate::Simulation(s);
      REQUIRE(sim.getNCompletedTimesteps() == 1);
      sim.doTimesteps(0.01);
      REQUIRE(sim.getNCompletedTimesteps() == 2);
    }
  }
}
