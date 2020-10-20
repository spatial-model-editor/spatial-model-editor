import unittest
import sme


class TestSpecies(unittest.TestCase):
    def test_species(self):

        # get an existing species
        m = sme.open_example_model()
        s = m.compartments["Cell"].species["A_cell"]

        # verify name and properties
        self.assertEqual(repr(s), "<sme.Species named 'A_cell'>")
        self.assertEqual(str(s)[0:32], "<sme.Species>\n  - name: 'A_cell'")
        self.assertEqual(s.name, "A_cell")
        self.assertEqual(s.diffusion_constant, 6.0)

        # assign new values
        s.name = "New A!"
        s.diffusion_constant = 1.0
        self.assertEqual(repr(s), "<sme.Species named 'New A!'>")
        self.assertEqual(str(s)[0:32], "<sme.Species>\n  - name: 'New A!'")
        self.assertEqual(s.name, "New A!")
        self.assertEqual(s.diffusion_constant, 1.0)

        # check change was propagated to model
        self.assertRaises(
            sme.InvalidArgument,
            lambda: m.compartments["Cell"].species["A_cell"],
        )
        s2 = m.compartments["Cell"].species["New A!"]
        self.assertEqual(s, s2)
