import unittest
import sme
import os.path
import numpy as np


def _get_abs_path(filename):
    return os.path.join(os.path.dirname(os.path.abspath(__file__)), filename)


# root-mean-square of all elements
def _rms(a):
    return np.sqrt(np.mean(np.square(a)))


class TestModel(unittest.TestCase):
    def test_open_sbml_file(self):
        with self.assertRaises(sme.InvalidArgument):
            sme.open_sbml_file("idontexist.xml")

    def test_open_file(self):
        with self.assertRaises(sme.InvalidArgument):
            sme.open_file("idontexist.xml")

    def test_open_example_model(self):
        m = sme.open_example_model()
        self.assertEqual(repr(m), "<sme.Model named 'Very Simple Model'>")
        self.assertEqual(
            str(m),
            "<sme.Model>\n  - name: 'Very Simple Model'\n  - compartments:\n     - Outside\n     - Cell\n     - Nucleus\n  - membranes:\n     - Outside <-> Cell\n     - Cell <-> Nucleus",
        )
        self.assertEqual(m.name, "Very Simple Model")
        self.assertEqual(len(m.compartments), 3)
        self.assertEqual(len(m.membranes), 2)
        m.name = "Model !"
        self.assertEqual(m.name, "Model !")

    def test_export_sbml_file(self):
        m = sme.open_example_model()
        m.name = "Mod"
        m.compartments["Cell"].name = "C"
        m.export_sbml_file("tmp.xml")
        m2 = sme.open_sbml_file("tmp.xml")
        self.assertEqual(m2.name, "Mod")
        self.assertEqual(len(m2.membranes), 2)
        self.assertEqual(len(m2.compartments), 3)
        self.assertEqual(m2.compartments["C"].name, "C")
        self.assertEqual(m2.compartments["Nucleus"].name, "Nucleus")
        self.assertRaises(sme.InvalidArgument, lambda: m2.compartments["Cell"])

    def test_export_sme_file(self):
        m = sme.open_example_model()
        m.name = "Mod"
        m.compartments["Cell"].name = "C"
        m.export_sme_file("tmp.sme")
        m2 = sme.open_file("tmp.sme")
        self.assertEqual(m2.name, "Mod")
        self.assertEqual(len(m2.membranes), 2)
        self.assertEqual(len(m2.compartments), 3)
        self.assertEqual(m2.compartments["C"].name, "C")
        self.assertEqual(m2.compartments["Nucleus"].name, "Nucleus")
        self.assertRaises(sme.InvalidArgument, lambda: m2.compartments["Cell"])

    def test_simulate(self):
        for sim_type in [sme.SimulatorType.DUNE, sme.SimulatorType.Pixel]:
            m = sme.open_example_model()
            sim_results = m.simulate(0.002, 0.001, simulator_type=sim_type)
            self.assertEqual(len(sim_results), 3)

            sim_results2 = m.simulation_results()
            self.assertEqual(len(sim_results), len(sim_results2))

            # continue previous sim
            sim_results = m.simulate(
                0.002, 0.001, simulator_type=sim_type, continue_existing_simulation=True
            )
            self.assertEqual(len(sim_results), 5)

            sim_results2 = m.simulation_results()
            self.assertEqual(len(sim_results), len(sim_results2))

            # use string overload
            sim_results = m.simulate(
                "0.002;0.001",
                "0.001;0.001",
                simulator_type=sim_type,
                continue_existing_simulation=True,
            )
            self.assertEqual(len(sim_results), 8)

            sim_results2 = m.simulation_results()
            self.assertEqual(len(sim_results), len(sim_results2))

            # previous sim results are cleared by default
            sim_results = m.simulate(0.002, 0.001, simulator_type=sim_type)
            self.assertEqual(len(sim_results), 3)

            sim_results2 = m.simulation_results()
            self.assertEqual(len(sim_results), len(sim_results2))

            for res in [sim_results[1], sim_results2[1]]:
                self.assertEqual(
                    repr(res), "<sme.SimulationResult from timepoint 0.001>"
                )
                self.assertEqual(
                    str(res),
                    "<sme.SimulationResult>\n  - timepoint: 0.001\n  - number of species: 5\n",
                )
                self.assertEqual(res.time_point, 0.001)
                img = res.concentration_image
                self.assertEqual(len(img), 100)
                self.assertEqual(len(img[0]), 100)
                self.assertEqual(len(img[0][0]), 3)
                self.assertEqual(len(res.species_concentration), 5)
                conc = res.species_concentration["B_cell"]
                self.assertEqual(len(conc), 100)
                self.assertEqual(len(conc[0]), 100)
                self.assertEqual(conc[0][0], 0.0)

            # set timeout to 1 second: by default simulation throws on timeout
            # multiple timesteps before timeout:
            with self.assertRaises(sme.RuntimeError):
                m.simulate(10000, 0.1, timeout_seconds=0, simulator_type=sim_type)

        # approximate dcdt (only returned from simulate & pixel & last timepoint)
        m = sme.open_example_model()
        sim_results = m.simulate(0.002, 0.001, simulator_type=sme.SimulatorType.Pixel)
        self.assertEqual(len(sim_results), 3)
        self.assertEqual(len(sim_results[0].species_dcdt), 0)
        self.assertEqual(len(sim_results[1].species_dcdt), 0)
        self.assertEqual(len(sim_results[2].species_dcdt), 5)
        dcdt_approx = (
            sim_results[2].species_concentration["A_cell"]
            - sim_results[1].species_concentration["A_cell"]
        ) / 0.001
        dcdt = sim_results[2].species_dcdt["A_cell"]
        rms_norm = _rms(dcdt)
        rms_diff = _rms(dcdt - dcdt_approx)
        self.assertLess(rms_diff / rms_norm, 0.01)

        # don't get dcdt from simulation_results():
        sim_results2 = m.simulation_results()
        self.assertEqual(len(sim_results2[0].species_dcdt), 0)
        self.assertEqual(len(sim_results2[1].species_dcdt), 0)
        self.assertEqual(len(sim_results2[2].species_dcdt), 0)

        # single long timestep that times out (only pixel)
        with self.assertRaises(sme.RuntimeError):
            m.simulate(10000, 10000, 1)
        # set timeout to 1 second: don't throw on timeout, return partial results
        res1 = m.simulate(10000, 0.1, 1, False)
        self.assertGreaterEqual(len(res1), 1)
        res2 = m.simulate(10000, 10000, 1, False)
        self.assertEqual(len(res2), 1)

        # option to not return simulation results
        for sim_type in [sme.SimulatorType.DUNE, sme.SimulatorType.Pixel]:
            m = sme.open_example_model()
            sim_results = m.simulate(
                0.002, 0.001, simulator_type=sim_type, return_results=False
            )
            self.assertEqual(len(sim_results), 0)

            # but results are still available from the model
            sim_results2 = m.simulation_results()
            self.assertEqual(len(sim_results2), 3)

    def test_import_geometry_from_image(self):
        imgfile_original = _get_abs_path("concave-cell-nucleus-100x100.png")
        imgfile_modified = _get_abs_path("modified-concave-cell-nucleus-100x100.png")
        m = sme.open_example_model()
        comp_img_0 = m.compartment_image
        nucl_mask_0 = m.compartments["Nucleus"].geometry_mask
        m.import_geometry_from_image(imgfile_modified)
        comp_img_1 = m.compartment_image
        nucl_mask_1 = m.compartments["Nucleus"].geometry_mask
        self.assertFalse(np.array_equal(nucl_mask_0, nucl_mask_1))
        self.assertFalse(np.array_equal(comp_img_0, comp_img_1))
        m.import_geometry_from_image(imgfile_original)
        comp_img_2 = m.compartment_image
        nucl_mask_2 = m.compartments["Nucleus"].geometry_mask
        self.assertTrue(np.array_equal(comp_img_0, comp_img_2))
        self.assertTrue(np.array_equal(nucl_mask_0, nucl_mask_2))
