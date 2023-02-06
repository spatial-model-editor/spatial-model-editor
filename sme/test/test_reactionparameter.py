import unittest
import sme


class TestReactionParameter(unittest.TestCase):
    def test_reactionparameter(self):
        # get an existing reaction parameter
        m = sme.open_example_model()
        r = m.compartments["Nucleus"].reactions["A to B conversion"]
        k = r.parameters["k1"]

        # verify name and properties
        self.assertEqual(repr(k), "<sme.ReactionParameter named 'k1'>")
        self.assertEqual(str(k)[0:38], "<sme.ReactionParameter>\n  - name: 'k1'")
        self.assertEqual(k.name, "k1")
        self.assertEqual(k.value, 0.3)

        # check getting it again doesn't make a copy
        k2 = r.parameters["k1"]
        self.assertEqual(k, k2)
        self.assertEqual(id(k), id(k2))

        # assign new values
        k.name = "New k"
        k.value = 0.8765
        self.assertEqual(repr(k), "<sme.ReactionParameter named 'New k'>")
        self.assertEqual(str(k)[0:41], "<sme.ReactionParameter>\n  - name: 'New k'")
        self.assertEqual(k.name, "New k")
        self.assertEqual(k.value, 0.8765)

        # check change was propagated to model
        self.assertRaises(
            sme.InvalidArgument,
            lambda: r.parameters["k1"],
        )
        k3 = r.parameters["New k"]
        self.assertEqual(k, k2)
        self.assertEqual(k, k3)
