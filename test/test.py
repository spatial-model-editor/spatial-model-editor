import unittest
import sme

class TestSme(unittest.TestCase):
    def test_module(self):
        self.assertEqual(str(sme)[0:13], "<module 'sme'")
    def test_load_open_sbml_file(self):
        with self.assertRaises(ValueError):
            sme.open_sbml_file('idontexist.xml')
    def test_open_example_model(self):
        m = sme.open_example_model()
        self.assertEqual(repr(m), "<sme.Model named 'Very Simple Model'>")
        self.assertEqual(str(m), "<sme.Model>\n  - name: 'Very Simple Model'\n  - compartments:\n     - Outside\n     - Cell\n     - Nucleus")
        self.assertEqual(m.name, 'Very Simple Model')
        self.assertEqual(len(m.compartments), 3)
        m.name = 'Model !'
        self.assertEqual(m.name, 'Model !')
    def test_export_sbml_file(self):
        m = sme.open_example_model()
        m.name = "Mod"
        m.compartments['Cell'].name = "C"
        m.export_sbml_file('tmp.xml')
        m2 = sme.open_sbml_file('tmp.xml')
        self.assertEqual(m2.name, 'Mod')
        self.assertEqual(len(m2.compartments), 3)
        self.assertEqual(m2.compartments['C'].name, 'C')
        self.assertEqual(m2.compartments['Nucleus'].name, 'Nucleus')
    def test_compartment_image(self):
        m = sme.open_example_model()
        img = m.compartment_image()
        self.assertEqual(len(img), 100)
        self.assertEqual(len(img[0]), 100)
        self.assertEqual(len(img[0][0]), 3)
    def test_simulate(self):
        m = sme.open_example_model()
        m.simulate(0.02, 0.01)
        time_points = m.simulation_time_points()
        self.assertEqual(len(time_points), 3)
        img = m.concentration_image(1)
        self.assertEqual(len(img), 100)
        self.assertEqual(len(img[0]), 100)
        self.assertEqual(len(img[0][0]), 3)
    def test_compartment(self):
        m = sme.open_example_model()
        c = m.compartments['Cell']
        self.assertEqual(repr(c), "<sme.Compartment named 'Cell'>")
        self.assertEqual(str(c)[0:34], "<sme.Compartment>\n  - name: 'Cell'")
        self.assertEqual(c.name, 'Cell')
        self.assertEqual(len(c.species), 2)
        c.name = 'NewCell'
        self.assertEqual(c.name, 'NewCell')
        self.assertEqual(m.compartments['NewCell'].name, 'NewCell')
    def test_species(self):
        m = sme.open_example_model()
        c = m.compartments['Cell']
        s = c.species['A_cell']
        self.assertEqual(repr(s), "<sme.Species named 'A_cell'>")
        self.assertEqual(str(s)[0:32], "<sme.Species>\n  - name: 'A_cell'")
        self.assertEqual(s.name, 'A_cell')
        self.assertEqual(s.diffusion_constant, 6.0)
        s.name = 'New A!'
        s.diffusion_constant = 1.0
        self.assertEqual(repr(s), "<sme.Species named 'New A!'>")
        self.assertEqual(str(s)[0:32], "<sme.Species>\n  - name: 'New A!'")
        self.assertEqual(s.name, 'New A!')
        self.assertEqual(s.diffusion_constant, 1.0)
        self.assertEqual(m.compartments['Cell'].species['New A!'].name, 'New A!')
    def test_reaction(self):
        m = sme.open_example_model()
        c = m.compartments['Nucleus']
        r = c.reactions['A to B conversion']
        self.assertEqual(repr(r), "<sme.Reaction named 'A to B conversion'>")
        self.assertEqual(str(r)[0:44], "<sme.Reaction>\n  - name: 'A to B conversion'")
        self.assertEqual(r.name, 'A to B conversion')
        r.name = 'New reac'
        self.assertEqual(repr(r), "<sme.Reaction named 'New reac'>")
        self.assertEqual(str(r)[0:35], "<sme.Reaction>\n  - name: 'New reac'")
        self.assertEqual(r.name, 'New reac')
        self.assertEqual(m.compartments['Nucleus'].reactions['New reac'].name, 'New reac')

if __name__ == '__main__':
    unittest.main()
