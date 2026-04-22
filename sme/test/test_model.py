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


def test_model_simulation_settings():
    m = sme.open_example_model()
    m.simulation_settings.simulator_type = sme.SimulatorType.Pixel
    assert m.simulation_settings.simulator_type == sme.SimulatorType.Pixel

    settings = m.simulation_settings
    settings.options.pixel.enable_multithreading = True
    settings.options.pixel.max_threads = 2
    m.simulation_settings = settings

    sim_results = m.simulate(0.002, 0.001)
    assert len(sim_results) == 3

    settings2 = m.simulation_settings
    assert settings2.simulator_type == sme.SimulatorType.Pixel
    assert settings2.options.pixel.enable_multithreading
    assert settings2.options.pixel.max_threads == 2

    # if simulation times are in model settings, simulate() can omit time args
    m.simulation_settings.times = [(2, 0.001)]
    sim_results2 = m.simulate()
    assert len(sim_results2) == 3


def test_simulate():
    for sim_type in [sme.SimulatorType.DUNE, sme.SimulatorType.Pixel]:
        m = sme.open_example_model()
        sim_results = m.simulate(0.002, 0.001, simulator_type=sim_type)
        assert len(sim_results) == 3
        assert sim_results[0].time_point == pytest.approx(0.0)
        assert np.mean(sim_results[0].species_concentration["A_cell"]) == pytest.approx(
            0.0
        )
        assert np.mean(sim_results[0].species_concentration["B_cell"]) == pytest.approx(
            0.0
        )
        assert np.mean(sim_results[0].species_concentration["A_nucl"]) == pytest.approx(
            0.0
        )
        assert np.mean(sim_results[0].species_concentration["B_nucl"]) == pytest.approx(
            0.0
        )

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
    settings = m.simulation_settings
    settings.simulator_type = sme.SimulatorType.Pixel
    m.simulation_settings = settings
    sim_results = m.simulate(0.002, 0.001)
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

    # explicit Pixel solver options
    m = sme.open_example_model()
    settings = m.simulation_settings
    settings.simulator_type = sme.SimulatorType.Pixel
    settings.options.pixel.integrator = sme.PixelIntegratorType.RK323
    settings.options.pixel.max_err.rel = 0.1
    settings.options.pixel.max_err.abs = 0.1
    settings.options.pixel.max_timestep = 0.01
    settings.options.pixel.enable_multithreading = True
    settings.options.pixel.max_threads = 1
    settings.options.pixel.do_cse = False
    settings.options.pixel.opt_level = 2
    m.simulation_settings = settings
    sim_results = m.simulate(0.002, 0.001, return_results=False)
    assert len(sim_results) == 0
    assert len(m.simulation_results()) == 3

    # explicit DUNE solver options
    m = sme.open_example_model()
    settings = m.simulation_settings
    settings.simulator_type = sme.SimulatorType.DUNE
    settings.options.dune.integrator = "Alexander2"
    settings.options.dune.write_vtk_files = False
    settings.options.dune.max_threads = 1
    m.simulation_settings = settings
    sim_results = m.simulate(0.002, 0.001, return_results=False)
    assert len(sim_results) == 0
    assert len(m.simulation_results()) == 3


def test_simulation_result_feature_values():
    m = sme.open_example_model("ABtoC")
    sim_results = m.simulate(0.001, 0.001, simulator_type=sme.SimulatorType.Pixel)
    assert len(sim_results) == 2

    first_result = sim_results[0]
    assert set(first_result.feature_values.keys()) == {"A average", "B average"}
    assert first_result.feature_values["A average"].shape == (1,)
    assert first_result.feature_values["A average"].dtype == np.float64
    assert first_result.feature_values["B average"].shape == (1,)

    mask = m.compartments["comp"].geometry_mask
    a_concentration = first_result.species_concentration["A"]
    b_concentration = first_result.species_concentration["B"]
    assert first_result.feature_values["A average"][0] == pytest.approx(
        np.mean(a_concentration[mask])
    )
    assert first_result.feature_values["B average"][0] == pytest.approx(
        np.mean(b_concentration[mask])
    )

    saved_results = m.simulation_results()
    assert len(saved_results) == len(sim_results)
    assert np.allclose(
        saved_results[0].feature_values["A average"],
        first_result.feature_values["A average"],
    )
    assert np.allclose(
        saved_results[0].feature_values["B average"],
        first_result.feature_values["B average"],
    )


def test_simulate_3d():
    for sim_type in [sme.SimulatorType.DUNE, sme.SimulatorType.Pixel]:
        m = sme.open_example_model("very-simple-model-3d")
        m.compartments["Cell"].species["B_cell"].uniform_concentration = 2.54345
        m.compartments["Nucleus"].species["A_nucl"].uniform_concentration = 1.123
        m.compartments["Nucleus"].species["B_nucl"].uniform_concentration = 0.10123
        sim_results = m.simulate(0.002, 0.001, simulator_type=sim_type)
        assert len(sim_results) == 3
        assert sim_results[0].time_point == pytest.approx(0.0)
        assert np.mean(sim_results[0].species_concentration["A_cell"]) == pytest.approx(
            0.0
        )
        assert np.mean(
            sim_results[0].species_concentration["B_cell"][
                ~m.compartments["Cell"].geometry_mask
            ]
        ) == pytest.approx(0.0)
        assert np.mean(
            sim_results[0].species_concentration["B_cell"][
                m.compartments["Cell"].geometry_mask
            ]
        ) == pytest.approx(2.54345)
        assert np.mean(
            sim_results[0].species_concentration["A_nucl"][
                ~m.compartments["Nucleus"].geometry_mask
            ]
        ) == pytest.approx(0.0)
        assert np.mean(
            sim_results[0].species_concentration["A_nucl"][
                m.compartments["Nucleus"].geometry_mask
            ]
        ) == pytest.approx(1.123)
        assert np.mean(
            sim_results[0].species_concentration["B_nucl"][
                ~m.compartments["Nucleus"].geometry_mask
            ]
        ) == pytest.approx(0.0)
        assert np.mean(
            sim_results[0].species_concentration["B_nucl"][
                m.compartments["Nucleus"].geometry_mask
            ]
        ) == pytest.approx(0.10123)


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
    assert not np.array_equal(nucl_mask_0, nucl_mask_1)
    assert not np.array_equal(comp_img_0, comp_img_1)
    m.import_geometry_from_image(imgfile_original)
    comp_img_2 = m.compartment_image
    nucl_mask_2 = m.compartments["Nucleus"].geometry_mask
    assert np.array_equal(comp_img_0, comp_img_2)
    assert np.array_equal(nucl_mask_0, nucl_mask_2)


def test_import_combine_archive():
    omex_file = get_abs_filename("liver-simplified.omex")
    m = sme.Model(omex_file)
    assert m.name == "TNFa Signaling"
    assert len(m.membranes) == 1
    assert len(m.compartments) == 2
    assert m.compartments["cytoplasm"].name == "cytoplasm"
    assert m.compartments["nucleus"].name == "nucleus"
