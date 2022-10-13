import pytest
import numpy as np
import sme


def test_parameter():
    # get an existing parameter
    m = sme.open_example_model()
    p = m.parameters["param"]

    # verify name and properties
    assert repr(p) == "<sme.Parameter named 'param'>"
    assert str(p)[0:33] == "<sme.Parameter>\n  - name: 'param'"
    assert p.name == "param"
    assert p.value == "1"

    # assign new values
    p.name = "New param"
    p.value = "0.8765"
    assert repr(p) == "<sme.Parameter named 'New param'>"
    assert str(p)[0:37] == "<sme.Parameter>\n  - name: 'New param'"
    assert p.name == "New param"
    assert p.value == "0.8765"

    # check change was propagated to model
    with pytest.raises(sme.InvalidArgument):
        m.parameters["param"]
    p2 = m.parameters["New param"]
    assert p2.name == "New param"
    assert p2.value == "0.8765"
    assert p2 == p
    assert p2 == m.parameters[0]
    assert p2 == m.parameters[-1]
