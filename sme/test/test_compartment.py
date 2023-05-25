import sme
import pytest
import numpy as np


def test_compartment_image() -> None:
    m = sme.open_example_model()
    img = m.compartment_image
    assert img.shape == (1, 100, 100, 3)
    assert np.all(img[0, 0, 0] == [0, 2, 0])  # outside
    assert np.all(img[0, 30, 30] == [144, 97, 193])  # Cell
    assert np.all(img[0, 50, 50] == [197, 133, 96])  # Nucleus


def test_compartment() -> None:
    m = sme.open_example_model()
    c = m.compartments["Cell"]
    assert repr(c) == "<sme.Compartment named 'Cell'>"
    assert str(c)[0:34] == "<sme.Compartment>\n  - name: 'Cell'"
    assert c.name == "Cell"
    assert len(c.species) == 2
    c.name = "NewCell"
    assert c.name == "NewCell"
    assert m.compartments["NewCell"].name == "NewCell"
    with pytest.raises(sme.InvalidArgument):
        m.compartments["Cell"]
    img = c.geometry_mask
    assert img.shape == (1, 100, 100)
    assert img[0, 0, 0] == False
    assert img[0, 30, 30] == True
    assert img[0, 50, 50] == False
