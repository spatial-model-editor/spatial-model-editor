import unittest
import sme


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
        sim_results = m.simulate(0.02, 0.01)
        self.assertEqual(len(sim_results), 3)
        res = sim_results[1]
        self.assertEqual(repr(res), "<sme.SimulationResult from timepoint 0.01>")
        self.assertEqual(
            str(res),
            "<sme.SimulationResult>\n  - timepoint: 0.01\n  - number of species: 5\n",
        )
        self.assertEqual(res.time_point, 0.01)
        img = res.concentration_image
        self.assertEqual(len(img), 100)
        self.assertEqual(len(img[0]), 100)
        self.assertEqual(len(img[0][0]), 3)
        self.assertEqual(len(res.species_concentration), 5)
        conc = res.species_concentration["B_cell"]
        self.assertEqual(len(conc), 100)
        self.assertEqual(len(conc[0]), 100)
        self.assertEqual(conc[0][0], 0.0)
        # set timeout to 1 second: simulation throws on timeout
        with self.assertRaises(sme.RuntimeError):
            m.simulate(10000, 0.01, 1)
