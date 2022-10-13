import numpy as np
import pytest
import sme


def test_membrane():
    m = sme.open_example_model()
    assert len(m.membranes) == 2
    with pytest.raises(sme.InvalidArgument):
        m.membranes["X"]
    mem = m.membranes["Outside <-> Cell"]
    assert mem == m.membranes[0]
    assert m.membranes[-1] == m.membranes[1]
    assert repr(mem) == "<sme.Membrane named 'Outside <-> Cell'>"
    assert str(mem)[0:43] == "<sme.Membrane>\n  - name: 'Outside <-> Cell'"
    assert mem.name == "Outside <-> Cell"
    mem.name = "new name"
    assert mem.name == "new name"
    assert len(mem.reactions) == 2
    with pytest.raises(sme.InvalidArgument):
        mem.reactions["X"]
    r = mem.reactions["A uptake from outside"]
    assert r.name == "A uptake from outside"
    # export model, open again, check membrane is preserved
    m.export_sbml_file("tmp.xml")
    m2 = sme.open_sbml_file("tmp.xml")
    assert len(m2.membranes) == 2
    mem2 = m2.membranes["new name"]
    assert mem2.name == mem.name
    assert len(mem2.reactions) == 2
    r2 = mem2.reactions["A uptake from outside"]
    assert r2.name == r.name
