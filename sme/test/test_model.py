import unittest
import sme


# root-mean-square of all elements
def rms(a_ij):
    d = 0.0
    n = 0.0
    for a_i in a_ij:
        for a in a_i:
            d += a ** 2
            n += 1.0
    return (d / n) ** 0.5


# a_ij <- (a_ij - b_ij) / dt
def sub_div(a_ij, b_ij, dt=1.0):
    for i, (a_i, b_i) in enumerate(zip(a_ij, b_ij)):
        for j, (a, b) in enumerate(zip(a_i, b_i)):
            a_ij[i][j] = (a - b) / dt


class TestModel(unittest.TestCase):
    def test_load_open_sbml_file(self):
        with self.assertRaises(sme.InvalidArgument):
            sme.open_sbml_file("idontexist.xml")

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

    def test_simulate(self):
        m = sme.open_example_model()
        sim_results = m.simulate(0.002, 0.001)
        self.assertEqual(len(sim_results), 3)
        res = sim_results[1]
        self.assertEqual(repr(res), "<sme.SimulationResult from timepoint 0.001>")
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
        dcdt = res.species_dcdt["B_cell"]
        self.assertEqual(len(dcdt), 100)
        self.assertEqual(len(dcdt[0]), 100)
        self.assertEqual(dcdt[0][0], 0.0)

        # approximate dcdt
        dcdt_approx = sim_results[1].species_concentration["A_cell"]
        sub_div(dcdt_approx, sim_results[0].species_concentration["A_cell"], 0.001)
        dcdt = sim_results[1].species_dcdt["A_cell"]
        rms_norm = rms(dcdt)
        sub_div(dcdt, dcdt_approx)
        rms_diff = rms(dcdt)
        self.assertLess(rms_diff / rms_norm, 0.01)

        # set timeout to 1 second: by default simulation throws on timeout
        # multiple timesteps before timeout:
        with self.assertRaises(sme.RuntimeError):
            m.simulate(10000, 0.1, 1)
        # single long timestep that times out
        with self.assertRaises(sme.RuntimeError):
            m.simulate(10000, 10000, 1)
        # set timeout to 1 second: don't throw on timeout, return partial results
        res1 = m.simulate(10000, 0.1, 1, False)
        self.assertGreaterEqual(len(res1), 1)
        res2 = m.simulate(10000, 10000, 1, False)
        self.assertEqual(len(res2), 1)
