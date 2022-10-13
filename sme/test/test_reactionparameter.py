import pytest
import sme


def test_reactionparameter():
    # get an existing reaction parameter
    m = sme.open_example_model()
    r = m.compartments["Nucleus"].reactions["A to B conversion"]
    k = r.parameters["k1"]

    # verify name and properties
    assert repr(k) == "<sme.ReactionParameter named 'k1'>"
    assert str(k)[0:38] == "<sme.ReactionParameter>\n  - name: 'k1'"
    assert k.name == "k1"
    assert k.value == 0.3

    # check getting it again doesn't make a copy
    k2 = r.parameters["k1"]
    assert k == k2
    assert id(k) == id(k2)

    # assign new values
    k.name = "New k"
    k.value = 0.8765
    assert repr(k) == "<sme.ReactionParameter named 'New k'>"
    assert str(k)[0:41] == "<sme.ReactionParameter>\n  - name: 'New k'"
    assert k.name == "New k"
    assert k.value == 0.8765

    # check change was propagated to model
    with pytest.raises(sme.InvalidArgument):
        r.parameters["k1"]
    k3 = r.parameters["New k"]
    assert k == k2
    assert k == k3
