import unittest
import sme


class TestCompartment(unittest.TestCase):
    def test_compartment_image(self):
        m = sme.open_example_model()
        img = m.compartment_image
        self.assertEqual(len(img), 100)
        self.assertEqual(len(img[0]), 100)
        self.assertEqual(len(img[0][0]), 3)
        self.assertEqual(img[0][0].tolist(), [0, 2, 0])  # outside
        self.assertEqual(img[30][30].tolist(), [144, 97, 193])  # Cell
        self.assertEqual(img[50][50].tolist(), [197, 133, 96])  # Nucleus

    def test_compartment(self):
        m = sme.open_example_model()
        c = m.compartments["Cell"]
        self.assertEqual(repr(c), "<sme.Compartment named 'Cell'>")
        self.assertEqual(str(c)[0:34], "<sme.Compartment>\n  - name: 'Cell'")
        self.assertEqual(c.name, "Cell")
        self.assertEqual(len(c.species), 2)
        c.name = "NewCell"
        self.assertEqual(c.name, "NewCell")
        self.assertEqual(m.compartments["NewCell"].name, "NewCell")
        self.assertRaises(sme.InvalidArgument, lambda: m.compartments["Cell"])
        img = c.geometry_mask
        self.assertEqual(len(img), 100)
        self.assertEqual(len(img[0]), 100)
        self.assertEqual(img[0][0], False)
        self.assertEqual(img[30][30], True)
        self.assertEqual(img[50][50], False)
