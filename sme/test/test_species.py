import pytest
import sme
import numpy as np


def test_species():
    # get an existing species
    m = sme.open_example_model()
    s = m.compartments["Cell"].species["A_cell"]

    # verify name and properties
    assert repr(s) == "<sme.Species named 'A_cell'>"
    assert str(s)[0:32] == "<sme.Species>\n  - name: 'A_cell'"
    assert s.name == "A_cell"
    assert s.diffusion_constant == 6.0
    assert s.concentration_type == sme.ConcentrationType.Uniform
    assert s.uniform_concentration == 0.0
    assert s.analytic_concentration == ""
    assert s.concentration_image.shape == (1, 100, 100)
    assert s.concentration_image[0, 1, 2] == 0.0
    assert s.concentration_image[0, 23, 40] == 0.0

    # assign new values
    s.name = "New A!"
    s.diffusion_constant = 1.0
    s.uniform_concentration = 0.3
    assert repr(s) == "<sme.Species named 'New A!'>"
    assert str(s)[0:32] == "<sme.Species>\n  - name: 'New A!'"
    assert s.name == "New A!"
    assert s.diffusion_constant == 1.0
    assert s.concentration_type == sme.ConcentrationType.Uniform
    assert s.uniform_concentration == 0.3
    assert s.analytic_concentration == ""
    assert s.concentration_image.shape == (1, 100, 100)
    assert np.allclose(s.concentration_image[0, 1, 2], 0.0)
    assert np.allclose(s.concentration_image[0, 23, 40], 0.3)

    # check changes were propagated to model
    with pytest.raises(sme.InvalidArgument):
        m.compartments["Cell"].species["A_cell"]
    s2 = m.compartments["Cell"].species["New A!"]
    assert s == s2

    # set an analytic initial concentration
    s.analytic_concentration = "cos(x)+1"
    # type of initial concentration changes:
    assert s.concentration_type == sme.ConcentrationType.Analytic
    # uniform concentration value is unchanged:
    assert s.uniform_concentration == 0.3
    # analytic concentration expression is no longer empty:
    assert s.analytic_concentration == "cos(x) + 1"
    assert s.concentration_image.shape == (1, 100, 100)
    assert np.allclose(s.concentration_image[0, 1, 2], 0.0)
    assert np.allclose(s.concentration_image[0, 23, 40], 0.05748050894911694)

    # set a uniform initial concentration
    s.uniform_concentration = 2.0
    # type of initial concentration changes:
    assert s.concentration_type == sme.ConcentrationType.Uniform
    assert s.uniform_concentration == 2.0
    # analytic concentration expression is now empty:
    assert s.analytic_concentration == ""
    assert s.concentration_image.shape == (1, 100, 100)
    assert np.allclose(s.concentration_image[0, 1, 2], 0.0)
    assert np.allclose(s.concentration_image[0, 23, 40], 2.0)

    # round trip check of image concentration
    a1 = np.random.default_rng().uniform(0, 1, (1, 100, 100))
    s.concentration_image = a1
    # uniform concentration value is unchanged:
    assert s.uniform_concentration == 2.0
    assert s.concentration_type == sme.ConcentrationType.Image
    # get concentration image
    a2 = s.concentration_image
    assert s.concentration_type == sme.ConcentrationType.Image
    compartment_mask = m.compartments["Cell"].geometry_mask
    # voxels inside the compartment have the assigned concentrations
    assert np.allclose(a1[compartment_mask], a2[compartment_mask])
    # voxels outside of the compartment are ignored
    assert np.sum(np.square(a2[~compartment_mask])) < 1e-7
    a1[~compartment_mask] = 0.0
    assert np.allclose(a1, a2)
    # set concentration to output concentration image
    s.concentration_image = a2
    a3 = s.concentration_image
    assert s.concentration_type == sme.ConcentrationType.Image
    assert np.allclose(a2, a3)

    # invalid image assignments throw with helpful message
    with pytest.raises(sme.InvalidArgument) as excinfo:
        s.concentration_image = np.random.default_rng().uniform(0, 1, 100)
    assert (
        "Invalid concentration image array: is 1-dimensional, should be 3-dimensional"
        in str(excinfo.value)
    )

    with pytest.raises(sme.InvalidArgument) as excinfo:
        s.concentration_image = np.random.default_rng().uniform(0, 1, (10, 10))
    assert (
        "Invalid concentration image array: is 2-dimensional, should be 3-dimensional"
        in str(excinfo.value)
    )

    with pytest.raises(sme.InvalidArgument) as excinfo:
        s.concentration_image = np.random.default_rng().uniform(0, 1, (2, 10, 100))
    assert "Invalid concentration image array: depth is 2, should be 1" in str(
        excinfo.value
    )

    with pytest.raises(sme.InvalidArgument) as excinfo:
        s.concentration_image = np.random.default_rng().uniform(0, 1, (1, 10, 100))
    assert "Invalid concentration image array: height is 10, should be 100" in str(
        excinfo.value
    )

    with pytest.raises(sme.InvalidArgument) as excinfo:
        s.concentration_image = np.random.default_rng().uniform(0, 1, (1, 100, 101))
    assert "Invalid concentration image array: width is 101, should be 100" in str(
        excinfo.value
    )
