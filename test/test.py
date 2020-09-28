import unittest
import sme


class TestModule(unittest.TestCase):
    def test_module(self):
        self.assertEqual(str(sme)[0:18], "<module 'sme' from")


class TestModel(unittest.TestCase):
    def test_load_open_sbml_file(self):
        with self.assertRaises(ValueError):
            sme.open_sbml_file("idontexist.xml")

    def test_open_example_model(self):
        m = sme.open_example_model()
        self.assertEqual(repr(m), "<sme.Model named 'Very Simple Model'>")
        self.assertEqual(
            str(m),
            "<sme.Model>\n  - name: 'Very Simple Model'\n  - compartments:\n     - Outside\n     - Cell\n     - Nucleus",
        )
        self.assertEqual(m.name, "Very Simple Model")
        self.assertEqual(len(m.compartments), 3)
        m.name = "Model !"
        self.assertEqual(m.name, "Model !")

    def test_export_sbml_file(self):
        m = sme.open_example_model()
        m.name = "Mod"
        m.compartment("Cell").name = "C"
        m.export_sbml_file("tmp.xml")
        m2 = sme.open_sbml_file("tmp.xml")
        self.assertEqual(m2.name, "Mod")
        self.assertEqual(len(m2.compartments), 3)
        self.assertEqual(m2.compartment("C").name, "C")
        self.assertEqual(m2.compartment("Nucleus").name, "Nucleus")
        self.assertRaises(ValueError, lambda: m2.compartment("Cell"))

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


class TestCompartment(unittest.TestCase):
    def test_compartment_image(self):
        m = sme.open_example_model()
        img = m.compartment_image
        self.assertEqual(len(img), 100)
        self.assertEqual(len(img[0]), 100)
        self.assertEqual(len(img[0][0]), 3)

    def test_compartment(self):
        m = sme.open_example_model()
        c = m.compartment("Cell")
        self.assertEqual(repr(c), "<sme.Compartment named 'Cell'>")
        self.assertEqual(str(c)[0:34], "<sme.Compartment>\n  - name: 'Cell'")
        self.assertEqual(c.name, "Cell")
        self.assertEqual(len(c.species), 2)
        c.name = "NewCell"
        self.assertEqual(c.name, "NewCell")
        self.assertEqual(m.compartment("NewCell").name, "NewCell")
        self.assertRaises(ValueError, lambda: m.compartment("Cell"))


class TestSpecies(unittest.TestCase):
    def test_species(self):
        m = sme.open_example_model()
        c = m.compartment("Cell")
        s = c.specie("A_cell")
        self.assertEqual(repr(s), "<sme.Species named 'A_cell'>")
        self.assertEqual(str(s)[0:32], "<sme.Species>\n  - name: 'A_cell'")
        self.assertEqual(s.name, "A_cell")
        self.assertEqual(s.diffusion_constant, 6.0)
        s.name = "New A!"
        s.diffusion_constant = 1.0
        self.assertEqual(repr(s), "<sme.Species named 'New A!'>")
        self.assertEqual(str(s)[0:32], "<sme.Species>\n  - name: 'New A!'")
        self.assertEqual(s.name, "New A!")
        self.assertEqual(s.diffusion_constant, 1.0)
        self.assertEqual(m.compartment("Cell").specie("New A!").name, "New A!")


class TestParameter(unittest.TestCase):
    def test_parameter(self):
        m = sme.open_example_model()
        self.assertEqual(len(m.parameters), 1)
        p = m.parameter("param")
        self.assertEqual(repr(p), "<sme.Parameter named 'param'>")
        self.assertEqual(str(p)[0:33], "<sme.Parameter>\n  - name: 'param'")
        self.assertEqual(p.name, "param")
        self.assertEqual(p.value, "1")
        p.name = "New param"
        p.value = "0.8765"
        self.assertEqual(repr(p), "<sme.Parameter named 'New param'>")
        self.assertEqual(str(p)[0:37], "<sme.Parameter>\n  - name: 'New param'")
        self.assertEqual(p.name, "New param")
        self.assertEqual(p.value, "0.8765")
        self.assertEqual(len(m.parameters), 1)
        self.assertRaises(ValueError, lambda: m.parameter("param"))
        self.assertEqual(m.parameter("New param").name, "New param")


class TestReaction(unittest.TestCase):
    def test_reaction(self):
        m = sme.open_example_model()
        c = m.compartment("Nucleus")
        r = c.reaction("A to B conversion")
        self.assertEqual(repr(r), "<sme.Reaction named 'A to B conversion'>")
        self.assertEqual(str(r)[0:44], "<sme.Reaction>\n  - name: 'A to B conversion'")
        self.assertEqual(r.name, "A to B conversion")
        self.assertEqual(len(r.parameters), 1)
        r.name = "New reac"
        self.assertEqual(repr(r), "<sme.Reaction named 'New reac'>")
        self.assertEqual(str(r)[0:35], "<sme.Reaction>\n  - name: 'New reac'")
        self.assertEqual(r.name, "New reac")
        self.assertEqual(c.reaction("New reac").name, "New reac")
        self.assertEqual(m.compartment("Nucleus").reaction("New reac").name, "New reac")
        self.assertEqual(len(r.parameters), 1)


class TestReactionParameter(unittest.TestCase):
    def test_reactionparameter(self):
        m = sme.open_example_model()
        c = m.compartment("Nucleus")
        r = c.reaction("A to B conversion")
        self.assertEqual(len(r.parameters), 1)
        k = r.parameter("k1")
        self.assertEqual(repr(k), "<sme.ReactionParameter named 'k1'>")
        self.assertEqual(str(k)[0:38], "<sme.ReactionParameter>\n  - name: 'k1'")
        self.assertEqual(k.name, "k1")
        self.assertEqual(k.value, 0.3)
        k.name = "New k"
        k.value = 0.8765
        self.assertEqual(repr(k), "<sme.ReactionParameter named 'New k'>")
        self.assertEqual(str(k)[0:41], "<sme.ReactionParameter>\n  - name: 'New k'")
        self.assertEqual(k.name, "New k")
        self.assertEqual(k.value, 0.8765)
        r2 = m.compartment("Nucleus").reaction("A to B conversion")
        self.assertEqual(len(r2.parameters), 1)
        self.assertRaises(ValueError, lambda: r2.parameter("k1"))
        self.assertEqual(r2.parameter("New k").name, "New k")


if __name__ == "__main__":
    unittest.main()
