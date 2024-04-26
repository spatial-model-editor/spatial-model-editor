import pytest
import sme
import pathlib
import numpy as np


def get_abs_filename(filename: str):
    return str(pathlib.Path(__file__).parent.resolve() / filename)


# root-mean-square of all elements
def rms(a):
    return np.sqrt(np.mean(np.square(a)))


def test_open_sbml_file_invalid():
    with pytest.raises(ValueError):
        sme.open_sbml_file("idontexist.xml")


def test_open_file_invalid():
    with pytest.raises(ValueError):
        sme.open_file("idontexist.xml")


def test_open_example_model_invalid():
    with pytest.raises(ValueError):
        sme.open_example_model("idontexist")

    with pytest.raises(ValueError):
        sme.open_example_model("")

    with pytest.raises(TypeError):
        sme.open_example_model([1, 2, 3])


def test_open_example_model():
    m = sme.open_example_model()
    assert repr(m) == "<sme.Model named 'Very Simple Model'>"
    assert (
        str(m)
        == "<sme.Model>\n  - name: 'Very Simple Model'\n  - compartments:\n     - Outside\n     - Cell\n     - Nucleus\n  - membranes:\n     - Outside <-> Cell\n     - Cell <-> Nucleus"
    )

    assert m.name == "Very Simple Model"
    assert len(m.compartments) == 3
    assert len(m.membranes) == 2
    m.name = "Model !"
    assert m.name == "Model !"

    m = sme.open_example_model("brusselator-model")
    assert repr(m) == "<sme.Model named 'The Brusselator'>"
    assert len(m.compartments) == 1
    assert len(m.membranes) == 0

    m = sme.open_example_model("gray-scott")
    assert repr(m) == "<sme.Model named 'Gray-Scott Model'>"
    assert len(m.compartments) == 1
    assert len(m.membranes) == 0


def test_export_sbml_file(tmp_path):
    m = sme.open_example_model()
    m.name = "Mod"
    m.compartments["Cell"].name = "C"
    tmp_filename = str(tmp_path / "tmp.xml")
    m.export_sbml_file(tmp_filename)
    m2 = sme.open_sbml_file(tmp_filename)
    assert m2.name == "Mod"
    assert len(m2.membranes) == 2
    assert len(m2.compartments) == 3
    assert m2.compartments["C"].name == "C"
    assert m2.compartments["Nucleus"].name == "Nucleus"
    with pytest.raises(ValueError):
        m2.compartments["Cell"]


def test_export_sme_file(tmp_path):
    m = sme.open_example_model()
    m.name = "Mod"
    m.compartments["Cell"].name = "C"
    tmp_filename = str(tmp_path / "tmp.sme")
    m.export_sme_file(tmp_filename)
    m2 = sme.open_file(tmp_filename)
    assert m2.name == "Mod"
    assert len(m2.membranes) == 2
    assert len(m2.compartments) == 3
    assert m2.compartments["C"].name == "C"
    assert m2.compartments["Nucleus"].name == "Nucleus"
    with pytest.raises(ValueError):
        m2.compartments["Cell"]


def test_simulate_invalid_timesteps():
    for sim_type in [sme.SimulatorType.DUNE, sme.SimulatorType.Pixel]:
        m = sme.open_example_model()
        with pytest.raises(ValueError):
            m.simulate("10;seven", "1;1", simulator_type=sim_type)


def test_simulate():
    for sim_type in [sme.SimulatorType.DUNE, sme.SimulatorType.Pixel]:
        m = sme.open_example_model()
        sim_results = m.simulate(0.002, 0.001, simulator_type=sim_type)
        assert len(sim_results) == 3

        sim_results2 = m.simulation_results()
        assert len(sim_results) == len(sim_results2)

        # continue previous sim
        sim_results = m.simulate(
            0.002, 0.001, simulator_type=sim_type, continue_existing_simulation=True
        )
        assert len(sim_results) == 5

        sim_results2 = m.simulation_results()
        assert len(sim_results) == len(sim_results2)

        # use string overload
        sim_results = m.simulate(
            "0.002;0.001",
            "0.001;0.001",
            simulator_type=sim_type,
            continue_existing_simulation=True,
        )
        assert len(sim_results) == 8

        sim_results2 = m.simulation_results()
        assert len(sim_results) == len(sim_results2)

        # previous sim results are cleared by default
        sim_results = m.simulate(0.002, 0.001, simulator_type=sim_type, n_threads=2)
        assert len(sim_results) == 3

        sim_results2 = m.simulation_results()
        assert len(sim_results) == len(sim_results2)

        for res in [sim_results[1], sim_results2[1]]:
            assert repr(res) == "<sme.SimulationResult from timepoint 0.001>"
            assert (
                str(res)
                == "<sme.SimulationResult>\n  - timepoint: 0.001\n  - number of species: 5\n"
            )
            assert res.time_point == 0.001
            img = res.concentration_image
            assert img.shape == (1, 100, 100, 3)
            assert len(res.species_concentration) == 5
            conc = res.species_concentration["B_cell"]
            assert conc.shape == (1, 100, 100)
            assert conc[0, 0, 0] == 0.0

        # set timeout to 1 second: by default simulation throws on timeout
        # multiple timesteps before timeout:
        with pytest.raises(RuntimeError):
            m.simulate(10000, 0.1, timeout_seconds=0, simulator_type=sim_type)

    # approximate dcdt (only returned from simulate & pixel & last timepoint)
    m = sme.open_example_model()
    sim_results = m.simulate(0.002, 0.001, simulator_type=sme.SimulatorType.Pixel)
    assert len(sim_results) == 3
    assert len(sim_results[0].species_dcdt) == 0
    assert len(sim_results[1].species_dcdt) == 0
    assert len(sim_results[2].species_dcdt) == 5
    dcdt_approx = (
        sim_results[2].species_concentration["A_cell"]
        - sim_results[1].species_concentration["A_cell"]
    ) / 0.001
    dcdt = sim_results[2].species_dcdt["A_cell"]
    rms_norm = rms(dcdt)
    rms_diff = rms(dcdt - dcdt_approx)
    assert rms_diff / rms_norm < 0.01

    # don't get dcdt from simulation_results():
    sim_results2 = m.simulation_results()
    assert len(sim_results2[0].species_dcdt) == 0
    assert len(sim_results2[1].species_dcdt) == 0
    assert len(sim_results2[2].species_dcdt) == 0

    # single long timestep that times out (only pixel)
    with pytest.raises(RuntimeError):
        m.simulate(10000, 10000, 1)
    # set timeout to 1 second: don't throw on timeout, return partial results
    res1 = m.simulate(10000, 0.1, 1, False)
    assert len(res1) >= 1
    res2 = m.simulate(10000, 10000, 1, False)
    assert len(res2) == 1

    # option to not return simulation results
    for sim_type in [sme.SimulatorType.DUNE, sme.SimulatorType.Pixel]:
        m = sme.open_example_model()
        sim_results = m.simulate(
            0.002, 0.001, simulator_type=sim_type, return_results=False
        )
        assert len(sim_results) == 0

        # but results are still available from the model
        sim_results2 = m.simulation_results()
        assert len(sim_results2) == 3


def test_import_geometry_from_image_invalid():
    m = sme.open_example_model()
    with pytest.raises(ValueError):
        m.import_geometry_from_image("idontexist.tiff")


def test_import_geometry_from_image():
    imgfile_original = get_abs_filename("concave-cell-nucleus-100x100.png")
    imgfile_modified = get_abs_filename("modified-concave-cell-nucleus-100x100.png")
    m = sme.open_example_model()
    comp_img_0 = m.compartment_image
    nucl_mask_0 = m.compartments["Nucleus"].geometry_mask
    m.import_geometry_from_image(imgfile_modified)
    comp_img_1 = m.compartment_image
    nucl_mask_1 = m.compartments["Nucleus"].geometry_mask
    assert np.array_equal(nucl_mask_0, nucl_mask_1) == False
    assert np.array_equal(comp_img_0, comp_img_1) == False
    m.import_geometry_from_image(imgfile_original)
    comp_img_2 = m.compartment_image
    nucl_mask_2 = m.compartments["Nucleus"].geometry_mask
    assert np.array_equal(comp_img_0, comp_img_2) == True
    assert np.array_equal(nucl_mask_0, nucl_mask_2) == True


def test_import_combine_archive():
    omex_file = get_abs_filename("liver-simplified.omex")
    m = sme.Model(omex_file)
    assert m.name == "TNFa Signaling"
    assert len(m.membranes) == 1
    assert len(m.compartments) == 2
    assert m.compartments["cytoplasm"].name == "cytoplasm"
    assert m.compartments["nucleus"].name == "nucleus"
