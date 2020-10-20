import unittest
import sme


class TestMembrane(unittest.TestCase):
    def test_membrane(self):
        m = sme.open_example_model()
        self.assertEqual(len(m.membranes), 2)
        self.assertRaises(sme.InvalidArgument, lambda: m.membranes["X"])
        mem = m.membranes["Outside <-> Cell"]
        self.assertEqual(mem, m.membranes[0])
        self.assertEqual(m.membranes[-1], m.membranes[1])
        self.assertEqual(repr(mem), "<sme.Membrane named 'Outside <-> Cell'>")
        self.assertEqual(str(mem)[0:43], "<sme.Membrane>\n  - name: 'Outside <-> Cell'")
        self.assertEqual(mem.name, "Outside <-> Cell")
        self.assertEqual(len(mem.reactions), 2)
        self.assertRaises(sme.InvalidArgument, lambda: mem.reactions["X"])
        r = mem.reactions["A uptake from outside"]
        self.assertEqual(r.name, "A uptake from outside")
