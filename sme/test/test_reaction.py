import pytest
import sme


def test_reaction():
    # get an existing reaction
    m = sme.open_example_model()
    c = m.compartments["Nucleus"]
    r = c.reactions["A to B conversion"]

    # verify name and properties
    assert repr(r) == "<sme.Reaction named 'A to B conversion'>"
    assert str(r)[0:44] == "<sme.Reaction>\n  - name: 'A to B conversion'"
    assert r.name == "A to B conversion"
    assert len(r.parameters) == 1
    assert r.parameters[0].name == "k1"
    assert r.parameters[0].value == 0.3

    # assign new values
    r.name = "New reac"
    r.parameters[0].name = "kk"
    r.parameters[0].value = 0.99
    assert repr(r) == "<sme.Reaction named 'New reac'>"
    assert str(r)[0:35] == "<sme.Reaction>\n  - name: 'New reac'"
    assert r.name == "New reac"

    # check change was propagated to model
    with pytest.raises(sme.InvalidArgument):
        c.reactions["A to B conversion"]
    r2 = c.reactions["New reac"]
    assert r2.name == "New reac"
    assert len(r2.parameters) == 1
    assert r2.parameters[0].name == "kk"
    assert r2.parameters[0].value == 0.99


def test_parameter_list():
    # get an existing reaction parameter list
    m = sme.open_example_model()
    ps = m.compartments["Nucleus"].reactions["A to B conversion"].parameters

    # verify indexing / name-lookup
    assert len(ps) == 1
    with pytest.raises(sme.InvalidArgument):
        ps["k2"]
    with pytest.raises(sme.InvalidArgument):
        ps[1]
    with pytest.raises(sme.InvalidArgument):
        ps[-2]
    k = ps["k1"]
    assert k == ps[0]
    assert k == ps[-1]
    for p in ps:
        assert p == k
