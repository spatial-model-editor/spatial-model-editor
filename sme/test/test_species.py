import pytest
import sme
import numpy as np


def test_species():
    # get an existing species
    m = sme.open_example_model()
    s = m.compartments["Cell"].species["A_cell"]
    assert sme.SpatialDataType.Uniform == sme.ConcentrationType.Uniform
    assert sme.SpatialDataType.Analytic == sme.ConcentrationType.Analytic
    assert sme.SpatialDataType.Image == sme.ConcentrationType.Image

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
    assert s.diffusion_type == sme.ConcentrationType.Uniform
    assert s.uniform_diffusion == 6.0
    assert s.analytic_diffusion == ""
    assert s.diffusion_image.shape == (1, 100, 100)
    compartment_mask = m.compartments["Cell"].geometry_mask
    assert np.allclose(s.diffusion_image[compartment_mask], 6.0)
    assert np.allclose(s.diffusion_image[~compartment_mask], 0.0)

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
    assert s.diffusion_type == sme.ConcentrationType.Uniform
    assert s.uniform_diffusion == 1.0
    assert s.analytic_diffusion == ""
    assert s.diffusion_image.shape == (1, 100, 100)
    compartment_mask = m.compartments["Cell"].geometry_mask
    assert np.allclose(s.diffusion_image[compartment_mask], 1.0)
    assert np.allclose(s.diffusion_image[~compartment_mask], 0.0)

    # check changes were propagated to model
    with pytest.raises(ValueError):
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

    # set an analytic diffusion constant
    s.analytic_diffusion = "sin(y)+2"
    assert s.diffusion_type == sme.ConcentrationType.Analytic
    assert np.isfinite(s.uniform_diffusion)
    assert s.analytic_diffusion.replace(" ", "") == "sin(y)+2"
    assert s.diffusion_image.shape == (1, 100, 100)
    compartment_mask = m.compartments["Cell"].geometry_mask
    assert np.allclose(s.diffusion_image[~compartment_mask], 0.0)
    diff_img = s.diffusion_image[0]
    mask2d = compartment_mask[0]
    ys = np.where(np.any(mask2d, axis=1))[0]
    y_samples = [ys[0], ys[len(ys) // 2], ys[-1]]
    for y in y_samples:
        row_vals = diff_img[y, mask2d[y]]
        assert np.std(row_vals) < 1e-6
        y_phys = (diff_img.shape[0] - 1 - y) + 0.5
        expected = np.sin(y_phys) + 2.0
        assert abs(np.mean(row_vals) - expected) < 1e-2
    prev_analytic_diffusion = s.analytic_diffusion
    prev_diffusion_image = s.diffusion_image.copy()
    with pytest.raises(ValueError) as excinfo:
        s.analytic_diffusion = "sin(y)+("
    assert "Invalid analytic diffusion expression: 'sin(y)+('" in str(excinfo.value)
    assert s.analytic_diffusion == prev_analytic_diffusion
    assert np.allclose(s.diffusion_image, prev_diffusion_image)

    # set a uniform diffusion constant
    s.uniform_diffusion = 3.0
    assert s.diffusion_type == sme.ConcentrationType.Uniform
    assert s.uniform_diffusion == 3.0
    assert s.analytic_diffusion == ""
    assert s.diffusion_image.shape == (1, 100, 100)
    compartment_mask = m.compartments["Cell"].geometry_mask
    assert np.allclose(s.diffusion_image[compartment_mask], 3.0)
    assert np.allclose(s.diffusion_image[~compartment_mask], 0.0)

    # round trip check of image diffusion constant
    d1 = np.random.default_rng().uniform(0, 1, (1, 100, 100))
    s.diffusion_image = d1
    assert np.isfinite(s.uniform_diffusion)
    assert s.diffusion_type == sme.ConcentrationType.Image
    d2 = s.diffusion_image
    assert s.diffusion_type == sme.ConcentrationType.Image
    compartment_mask = m.compartments["Cell"].geometry_mask
    assert np.allclose(d1[compartment_mask], d2[compartment_mask])
    assert np.sum(np.square(d2[~compartment_mask])) < 1e-7
    d1[~compartment_mask] = 0.0
    assert np.allclose(d1, d2)
    s.diffusion_image = d2
    d3 = s.diffusion_image
    assert s.diffusion_type == sme.ConcentrationType.Image
    assert np.allclose(d2, d3)

    # invalid diffusion image assignments throw with helpful message
    with pytest.raises(ValueError) as excinfo:
        s.diffusion_image = np.random.default_rng().uniform(0, 1, 100)
    assert (
        "Invalid diffusion image array: is 1-dimensional, should be 3-dimensional"
        in str(excinfo.value)
    )

    with pytest.raises(ValueError) as excinfo:
        s.diffusion_image = np.random.default_rng().uniform(0, 1, (10, 10))
    assert (
        "Invalid diffusion image array: is 2-dimensional, should be 3-dimensional"
        in str(excinfo.value)
    )

    with pytest.raises(ValueError) as excinfo:
        s.diffusion_image = np.random.default_rng().uniform(0, 1, (2, 10, 100))
    assert "Invalid diffusion image array: depth is 2, should be 1" in str(
        excinfo.value
    )

    with pytest.raises(ValueError) as excinfo:
        s.diffusion_image = np.random.default_rng().uniform(0, 1, (1, 10, 100))
    assert "Invalid diffusion image array: height is 10, should be 100" in str(
        excinfo.value
    )

    with pytest.raises(ValueError) as excinfo:
        s.diffusion_image = np.random.default_rng().uniform(0, 1, (1, 100, 101))
    assert "Invalid diffusion image array: width is 101, should be 100" in str(
        excinfo.value
    )

    # invalid image assignments throw with helpful message
    with pytest.raises(ValueError) as excinfo:
        s.concentration_image = np.random.default_rng().uniform(0, 1, 100)
    assert (
        "Invalid concentration image array: is 1-dimensional, should be 3-dimensional"
        in str(excinfo.value)
    )

    with pytest.raises(ValueError) as excinfo:
        s.concentration_image = np.random.default_rng().uniform(0, 1, (10, 10))
    assert (
        "Invalid concentration image array: is 2-dimensional, should be 3-dimensional"
        in str(excinfo.value)
    )

    with pytest.raises(ValueError) as excinfo:
        s.concentration_image = np.random.default_rng().uniform(0, 1, (2, 10, 100))
    assert "Invalid concentration image array: depth is 2, should be 1" in str(
        excinfo.value
    )

    with pytest.raises(ValueError) as excinfo:
        s.concentration_image = np.random.default_rng().uniform(0, 1, (1, 10, 100))
    assert "Invalid concentration image array: height is 10, should be 100" in str(
        excinfo.value
    )

    with pytest.raises(ValueError) as excinfo:
        s.concentration_image = np.random.default_rng().uniform(0, 1, (1, 100, 101))
    assert "Invalid concentration image array: width is 101, should be 100" in str(
        excinfo.value
    )
