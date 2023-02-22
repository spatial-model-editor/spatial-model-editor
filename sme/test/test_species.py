import unittest
import sme
import numpy as np


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
        self.assertEqual(s.concentration_type, sme.ConcentrationType.Uniform)
        self.assertEqual(s.uniform_concentration, 0.0)
        self.assertEqual(s.analytic_concentration, "")
        self.assertEqual(s.concentration_image.shape, (100, 100))
        self.assertEqual(s.concentration_image[1, 2], 0.0)
        self.assertEqual(s.concentration_image[23, 40], 0.0)

        # assign new values
        s.name = "New A!"
        s.diffusion_constant = 1.0
        s.uniform_concentration = 0.3
        self.assertEqual(repr(s), "<sme.Species named 'New A!'>")
        self.assertEqual(str(s)[0:32], "<sme.Species>\n  - name: 'New A!'")
        self.assertEqual(s.name, "New A!")
        self.assertEqual(s.diffusion_constant, 1.0)
        self.assertEqual(s.concentration_type, sme.ConcentrationType.Uniform)
        self.assertEqual(s.uniform_concentration, 0.3)
        self.assertEqual(s.analytic_concentration, "")
        self.assertEqual(s.concentration_image.shape, (100, 100))
        self.assertAlmostEqual(s.concentration_image[1, 2], 0.0)
        self.assertAlmostEqual(s.concentration_image[23, 40], 0.3)

        # check changes were propagated to model
        self.assertRaises(
            sme.InvalidArgument,
            lambda: m.compartments["Cell"].species["A_cell"],
        )
        s2 = m.compartments["Cell"].species["New A!"]
        self.assertEqual(s, s2)

        # set an analytic initial concentration
        s.analytic_concentration = "cos(x)+1"
        # type of initial concentration changes:
        self.assertEqual(s.concentration_type, sme.ConcentrationType.Analytic)
        # uniform concentration value is unchanged:
        self.assertEqual(s.uniform_concentration, 0.3)
        # analytic concentration expression is no longer empty:
        self.assertEqual(s.analytic_concentration, "cos(x) + 1")
        self.assertEqual(s.concentration_image.shape, (100, 100))
        self.assertAlmostEqual(s.concentration_image[1, 2], 0.0)
        self.assertAlmostEqual(s.concentration_image[23, 40], 0.05748050894911694)

        # set a uniform initial concentration
        s.uniform_concentration = 2.0
        # type of initial concentration changes:
        self.assertEqual(s.concentration_type, sme.ConcentrationType.Uniform)
        self.assertEqual(s.uniform_concentration, 2.0)
        # analytic concentration expression is now empty:
        self.assertEqual(s.analytic_concentration, "")
        self.assertEqual(s.concentration_image.shape, (100, 100))
        self.assertAlmostEqual(s.concentration_image[1, 2], 0.0)
        self.assertAlmostEqual(s.concentration_image[23, 40], 2.0)

        # round trip check of image concentration
        a1 = np.random.default_rng().uniform(0, 1, (100, 100))
        s.concentration_image = a1
        # uniform concentration value is unchanged:
        self.assertEqual(s.uniform_concentration, 2.0)
        self.assertEqual(s.concentration_type, sme.ConcentrationType.Image)
        # get concentration image
        a2 = s.concentration_image
        self.assertEqual(s.concentration_type, sme.ConcentrationType.Image)
        self.assertAlmostEqual(a1[23, 48], a2[23, 48])
        # pixels outside of compartment are ignored
        compartment_mask = m.compartments["Cell"].geometry_mask
        self.assertLess(np.sum(np.square(a2[~compartment_mask])), 1e-7)
        a1[~compartment_mask] = 0.0
        self.assertLess(np.sum(np.square(a1 - a2)), 1e-7)
        # set concentration to output concentration image
        s.concentration_image = a2
        a3 = s.concentration_image
        self.assertEqual(s.concentration_type, sme.ConcentrationType.Image)
        self.assertAlmostEqual(a2[23, 48], a3[23, 48])
        self.assertLess(np.sum(np.square(a2 - a3)), 1e-7)

        # invalid image assignments throw with helpful message
        with self.assertRaises(sme.InvalidArgument) as err:
            s.concentration_image = np.random.default_rng().uniform(0, 1, 100)
        self.assertEqual(
            "Invalid concentration image array: is 1-dimensional, should be 2-dimensional",
            str(err.exception),
        )

        with self.assertRaises(sme.InvalidArgument) as err:
            s.concentration_image = np.random.default_rng().uniform(0, 1, (10, 10, 10))
        self.assertEqual(
            "Invalid concentration image array: is 3-dimensional, should be 2-dimensional",
            str(err.exception),
        )

        with self.assertRaises(sme.InvalidArgument) as err:
            s.concentration_image = np.random.default_rng().uniform(0, 1, (10, 100))
        self.assertEqual(
            "Invalid concentration image array: height is 10, should be 100",
            str(err.exception),
        )

        with self.assertRaises(sme.InvalidArgument) as err:
            s.concentration_image = np.random.default_rng().uniform(0, 1, (100, 101))
        self.assertEqual(
            "Invalid concentration image array: width is 101, should be 100",
            str(err.exception),
        )
