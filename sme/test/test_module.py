import sme


def test_module():
    # check we can import the module
    assert str(sme)[0:18] == "<module 'sme' from"
