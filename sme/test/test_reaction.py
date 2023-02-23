import unittest
import sme


class TestReaction(unittest.TestCase):
    def test_reaction(self):
        # get an existing reaction
        m = sme.open_example_model()
        c = m.compartments["Nucleus"]
        r = c.reactions["A to B conversion"]

        # verify name and properties
        self.assertEqual(repr(r), "<sme.Reaction named 'A to B conversion'>")
        self.assertEqual(str(r)[0:44], "<sme.Reaction>\n  - name: 'A to B conversion'")
        self.assertEqual(r.name, "A to B conversion")
        self.assertEqual(len(r.parameters), 1)
        self.assertEqual(r.parameters[0].name, "k1")
        self.assertEqual(r.parameters[0].value, 0.3)

        # assign new values
        r.name = "New reac"
        r.parameters[0].name = "kk"
        r.parameters[0].value = 0.99
        self.assertEqual(repr(r), "<sme.Reaction named 'New reac'>")
        self.assertEqual(str(r)[0:35], "<sme.Reaction>\n  - name: 'New reac'")
        self.assertEqual(r.name, "New reac")

        # check change was propagated to model
        self.assertRaises(
            sme.InvalidArgument,
            lambda: c.reactions["A to B conversion"],
        )
        r2 = c.reactions["New reac"]
        self.assertEqual(r2.name, "New reac")
        self.assertEqual(len(r2.parameters), 1)
        self.assertEqual(r2.parameters[0].name, "kk")
        self.assertEqual(r2.parameters[0].value, 0.99)

    def test_parameter_list(self):
        # get an existing reaction parameter list
        m = sme.open_example_model()
        ps = m.compartments["Nucleus"].reactions["A to B conversion"].parameters

        # verify indexing / name-lookup
        self.assertEqual(len(ps), 1)
        self.assertRaises(sme.InvalidArgument, lambda: ps["k2"])
        self.assertRaises(sme.InvalidArgument, lambda: ps[1])
        self.assertRaises(sme.InvalidArgument, lambda: ps[-2])
        k = ps["k1"]
        self.assertEqual(k, ps[0])
        self.assertEqual(k, ps[-1])
        for p in ps:
            self.assertEqual(p, k)
