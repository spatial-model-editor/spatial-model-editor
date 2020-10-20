import unittest
import sme


class TestParameter(unittest.TestCase):
    def test_parameter(self):

        # get an existing parameter
        m = sme.open_example_model()
        p = m.parameters["param"]

        # verify name and properties
        self.assertEqual(repr(p), "<sme.Parameter named 'param'>")
        self.assertEqual(str(p)[0:33], "<sme.Parameter>\n  - name: 'param'")
        self.assertEqual(p.name, "param")
        self.assertEqual(p.value, "1")

        # assign new values
        p.name = "New param"
        p.value = "0.8765"
        self.assertEqual(repr(p), "<sme.Parameter named 'New param'>")
        self.assertEqual(str(p)[0:37], "<sme.Parameter>\n  - name: 'New param'")
        self.assertEqual(p.name, "New param")
        self.assertEqual(p.value, "0.8765")

        # check change was propagated to model
        self.assertRaises(
            sme.InvalidArgument,
            lambda: m.parameters["param"],
        )
        p2 = m.parameters["New param"]
        self.assertEqual(p2.name, "New param")
        self.assertEqual(p2.value, "0.8765")
        self.assertEqual(p2, p)
        self.assertEqual(p2, m.parameters[0])
        self.assertEqual(p2, m.parameters[-1])
