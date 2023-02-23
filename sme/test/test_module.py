import unittest
import sme


class TestModule(unittest.TestCase):
    def test_module(self):
        # check we can have imported the module
        self.assertEqual(str(sme)[0:18], "<module 'sme' from")
