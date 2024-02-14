# Changelog
## [1.5.0] - 2023-12-01
### Added
- support for dune-copasi 2 [#908](https://github.com/spatial-model-editor/spatial-model-editor/issues/908)
- Python 3.12 wheels [#897](https://github.com/spatial-model-editor/spatial-model-editor/issues/897)
- Apple Silicon wheels and binaries [ssciwr/sme-osx-arm64](https://github.com/ssciwr/sme-osx-arm64/)

## [1.4.0] - 2023-09-04
### Added
- initial support for 3d spatial models [#850](https://github.com/spatial-model-editor/spatial-model-editor/issues/850)
- support for anisotropic voxels [#879](https://github.com/spatial-model-editor/spatial-model-editor/issues/879)

### Changed
- disable OK button while selecting geometry image colours [#878](https://github.com/spatial-model-editor/spatial-model-editor/issues/878).
- image arrays in sme (Python interface) now include a z dimension [#850](https://github.com/spatial-model-editor/spatial-model-editor/issues/850)

### Fixed
- lack of geometry image could cause a crash in some cases [#872](https://github.com/spatial-model-editor/spatial-model-editor/issues/872)

## [1.3.6] - 2023-05-23
### Added
- support for importing RGBA TIFF image files [#862](https://github.com/spatial-model-editor/spatial-model-editor/issues/862)

## [1.3.5] - 2023-03-02
### Fixed
- incorrect function evaluations in some cases [#855](https://github.com/spatial-model-editor/spatial-model-editor/issues/855), [#856](https://github.com/spatial-model-editor/spatial-model-editor/issues/856), [#857](https://github.com/spatial-model-editor/spatial-model-editor/issues/857)

## [1.3.4] - 2023-02-23
### Fixed
- incorrect initial concentrations with DUNE simulator for some models [#852](https://github.com/spatial-model-editor/spatial-model-editor/issues/852)

## [1.3.3] - 2022-12-05
### Changed
- export format of Image Sampled Field from colors to indices [#710](https://github.com/spatial-model-editor/spatial-model-editor/issues/710) and [#830](https://github.com/spatial-model-editor/spatial-model-editor/issues/830).

## [1.3.2] - 2022-10-13
### Added
- support for importing Combine archives [#828](https://github.com/spatial-model-editor/spatial-model-editor/issues/828)

## [1.3.1] - 2022-08-10
### Changed
- offer to use alternative simulator if simulation setup fails [#814](https://github.com/spatial-model-editor/spatial-model-editor/issues/814)

### Fixed
- crash caused by analytic initial species concentrations involving dividing by zero [#805](https://github.com/spatial-model-editor/spatial-model-editor/issues/805)
- disappearing simulation progress bar dialog [#752](https://github.com/spatial-model-editor/spatial-model-editor/issues/752)

## [1.3.0] - 2022-06-10
### Added
- [parameter fitting](https://spatial-model-editor.readthedocs.io/en/stable/reference/parameter-fitting.html) functionality to GUI [#757](https://github.com/spatial-model-editor/spatial-model-editor/issues/757)
- support for reading SBML files compressed with bzip2 [#246](https://github.com/spatial-model-editor/spatial-model-editor/issues/246)

### Changed
- GUI no longer writes logging output to console [#771](https://github.com/spatial-model-editor/spatial-model-editor/issues/771)

### Fixed
- analytic initial species concentrations were not updated when a parameter value was modified [#776](https://github.com/spatial-model-editor/spatial-model-editor/issues/776)

## [1.2.2] - 2022-04-28
### Added
- support for reading compressed sampledFields [#709](https://github.com/spatial-model-editor/spatial-model-editor/issues/709)

### Changed
- TBB is now used everywhere for multithreading (previously linux Python wheels used OpenMP) [#732](https://github.com/spatial-model-editor/spatial-model-editor/issues/732)

### Fixed
- improved multi-threading performance [#742](https://github.com/spatial-model-editor/spatial-model-editor/issues/742)
- spurious simulation times are no longer appended upon simulation [#751](https://github.com/spatial-model-editor/spatial-model-editor/issues/751)

## [1.2.1] - 2021-09-30
### Added
- topology-preserving line simplification option [#328](https://github.com/spatial-model-editor/spatial-model-editor/issues/328)
- python interface: `ctrl+c` now cancels a currently running simulation [#665](https://github.com/spatial-model-editor/spatial-model-editor/issues/665)

### Changed
- model reactions with invalid locations can now be seen and removed or relocated [#615](https://github.com/spatial-model-editor/spatial-model-editor/issues/615)

### Fixed
- functions with zero arguments are now fully supported [#674](https://github.com/spatial-model-editor/spatial-model-editor/issues/674)
- python interface: setting `n_threads=1` for simulate takes precedence over `n_threads` value in model [#672](https://github.com/spatial-model-editor/spatial-model-editor/issues/672)
- bug where membrane reactions could disappear when geometry was changed [#679](https://github.com/spatial-model-editor/spatial-model-editor/issues/679)
- bug where removing a compartment could result in an invalid model or a crash [#685](https://github.com/spatial-model-editor/spatial-model-editor/issues/685)
- SBML export includes any ModifierSpecies in reactions [#133](https://github.com/spatial-model-editor/spatial-model-editor/issues/133)

### Removed
- incomplete Parametric geometry sbml export support [#599](https://github.com/spatial-model-editor/spatial-model-editor/issues/599)

## [1.2.0] - 2021-09-13
### Added
- non-spatial model guided import with automatic reaction rate rescaling [#607](https://github.com/spatial-model-editor/spatial-model-editor/issues/607)
- python interface: optional multithreading (via OpenMP) for Pixel simulations on linux [#662](https://github.com/spatial-model-editor/spatial-model-editor/issues/662)

### Fixed
- bug where loading a model with simulation data and changing model units could cause a crash [#666](https://github.com/spatial-model-editor/spatial-model-editor/issues/666)
- show warning message before dune simulation of model with non-spatial species [#664](https://github.com/spatial-model-editor/spatial-model-editor/issues/664)
- minimum supported version of MacOS is now 10.14 instead of 10.15 [#669](https://github.com/spatial-model-editor/spatial-model-editor/issues/669)

## [1.1.5] - 2021-09-01
### Added
- python interface: access to all built-in example models [#637](https://github.com/spatial-model-editor/spatial-model-editor/pull/637)
- python interface: view and edit uniform/analytic/image species initial concentrations [#644](https://github.com/spatial-model-editor/spatial-model-editor/issues/644)

### Changed
- compartment colour assignments are now preserved when the geometry image is resized [#587](https://github.com/spatial-model-editor/spatial-model-editor/issues/587)

### Fixed
- python interface: bug where compartment geometry mask was not updated after geometry image changed [#630](https://github.com/spatial-model-editor/spatial-model-editor/issues/630)
- slow loading of models with large geometry images [#632](https://github.com/spatial-model-editor/spatial-model-editor/issues/632)
- avoid constructing mesh twice on model load [#597](https://github.com/spatial-model-editor/spatial-model-editor/issues/597)
- crash when importing geometry image after importing non-spatial model [#651](https://github.com/spatial-model-editor/spatial-model-editor/issues/651)

## [1.1.4] - 2021-08-17
### Added
- python interface: `Model.simulation_results()` provides access to existing model simulation data [#622](https://github.com/spatial-model-editor/spatial-model-editor/issues/622)

### Changed
- python interface: simulation results now provided in [numpy.ndarray](https://numpy.org/doc/stable/reference/generated/numpy.ndarray.html) format [#610](https://github.com/spatial-model-editor/spatial-model-editor/issues/610)

### Fixed
- bug where opening a corrupted sme file caused a crash [#618](https://github.com/spatial-model-editor/spatial-model-editor/issues/618)

## [1.1.3] - 2021-08-10
### Added
- python interface: add option to simulate without returning the results to reduce RAM usage [#610](https://github.com/spatial-model-editor/spatial-model-editor/issues/610)
- more detailed error message when a model cannot be loaded [#449](https://github.com/spatial-model-editor/spatial-model-editor/issues/449)

### Fixed
- bug where invalid reaction rate expression caused simulation to crash [#609](https://github.com/spatial-model-editor/spatial-model-editor/issues/609)

## [1.1.2] - 2021-07-13
### Added
- autocomplete when editing maths [#559](https://github.com/spatial-model-editor/spatial-model-editor/issues/559)
- support for all L3 sbml math in reaction rates (with Pixel simulator) [#569](https://github.com/spatial-model-editor/spatial-model-editor/issues/569)
- option to invert y-axis of images [#568](https://github.com/spatial-model-editor/spatial-model-editor/issues/568)
- option to set size of geometry image in third dimension [#582](https://github.com/spatial-model-editor/spatial-model-editor/issues/582)
- `umol` unit of amount added to built-in units [#600](https://github.com/spatial-model-editor/spatial-model-editor/issues/600)

### Changed
- simulation length/intervals only loaded from model on 'reset' click or new model load [#565](https://github.com/spatial-model-editor/spatial-model-editor/issues/565)
- models exported as 3d SBML models (to be consistent with our 3d units for species concentrations and volumes) [#588](https://github.com/spatial-model-editor/spatial-model-editor/issues/588)

### Fixed
- bug when clicking on simulation results plot selected wrong timepoint [#570](https://github.com/spatial-model-editor/spatial-model-editor/issues/570)
- reaction rates involving species from unrelated compartments now displays an error, instead of only giving an error on simulation [#552](https://github.com/spatial-model-editor/spatial-model-editor/issues/552)
- simulation crash when species is changed to constant & simulation data are re-loaded [#561](https://github.com/spatial-model-editor/spatial-model-editor/issues/561)
- inaccuracy when re-starting a simulation that was stopped early [#388](https://github.com/spatial-model-editor/spatial-model-editor/issues/388)
- bug where compartment sizes were not updated when compartment image resolution was altered [#583](https://github.com/spatial-model-editor/spatial-model-editor/issues/583)
- bug where invalid mesh could cause a crash [#585](https://github.com/spatial-model-editor/spatial-model-editor/issues/585)
- bug where simulating after changing geometry image size could cause a crash [#591](https://github.com/spatial-model-editor/spatial-model-editor/issues/591)
- incorrect units used for compartment volumes when referenced in reaction rates [#584](https://github.com/spatial-model-editor/spatial-model-editor/issues/584)
- bug where membrane area could be incorrect on model load [#595](https://github.com/spatial-model-editor/spatial-model-editor/issues/595)

## [1.1.1] - 2021-05-30
### Added
- COPASI cps file import support (if COPASI is installed and on path) [#492](https://github.com/spatial-model-editor/spatial-model-editor/issues/492)
- shift + mouse scroll to zoom in and out of images [#532](https://github.com/spatial-model-editor/spatial-model-editor/issues/532)
- support for spatial models with reactions whose compartment is not specified [#481](https://github.com/spatial-model-editor/spatial-model-editor/issues/481)

### Changed
- mouse scroll no longer changes species stoichiometry in reactions [#549](https://github.com/spatial-model-editor/spatial-model-editor/issues/549)
- membrane reaction rates automatically rescaled by membrane area on non-spatial model import [#557](https://github.com/spatial-model-editor/spatial-model-editor/pull/557)
- reactions moved between compartments and membranes rescaled by ratio of area to volume [#558](https://github.com/spatial-model-editor/spatial-model-editor/issues/558)

### Fixed
- bug loading simulation settings depending on system locale setting [#535](https://github.com/spatial-model-editor/spatial-model-editor/issues/535)
- slow loading of simulation data [#504](https://github.com/spatial-model-editor/spatial-model-editor/issues/504)
- bug where simulation doesn't run again after being stopped [#545](https://github.com/spatial-model-editor/spatial-model-editor/issues/545)
- bug where selected reaction sometimes changes when reaction location is changed [#548](https://github.com/spatial-model-editor/spatial-model-editor/issues/548)
- ensure species involved in a reaction are valid when reaction location is changed [#551](https://github.com/spatial-model-editor/spatial-model-editor/issues/551)
- bug where loading existing simulation uses initial concentrations instead of latest simulation concentrations [#541](https://github.com/spatial-model-editor/spatial-model-editor/issues/541)

## [1.1.0] - 2021-05-04
### Added
- sme file format, contains model, simulation settings and simulation results [#482](https://github.com/spatial-model-editor/spatial-model-editor/issues/482)
- support multiple simulation lengths as comma delimited lists [#464](https://github.com/spatial-model-editor/spatial-model-editor/issues/464)
- support for simulating models with empty compartments with the Dune-Copasi simulator [#435](https://github.com/spatial-model-editor/spatial-model-editor/issues/435)
- option to resize the number of pixels in the geometry image [#462](https://github.com/spatial-model-editor/spatial-model-editor/issues/462)
- option to reduce the number of colours in the geometry image [#280](https://github.com/spatial-model-editor/spatial-model-editor/issues/280)
- support for non-integer stoichiometries in reactions [#495](https://github.com/spatial-model-editor/spatial-model-editor/issues/495)
- optional grid and scale to geometry image [#497](https://github.com/spatial-model-editor/spatial-model-editor/issues/497)
- python interface: DUNE simulator can now be used [#276](https://github.com/spatial-model-editor/spatial-model-editor/issues/276)
- python interface: existing simulations can be continued [#514](https://github.com/spatial-model-editor/spatial-model-editor/issues/514)
- zoom option to geometry image [#516](https://github.com/spatial-model-editor/spatial-model-editor/issues/516)

### Changed
- use v1.1.0 of Dune-Copasi simulator

### Fixed
- python interface: revert change in default simulate() behaviour when doing a second simulation of a model [#475](https://github.com/spatial-model-editor/spatial-model-editor/issues/475)
- multiple event related simulation bugs [#484](https://github.com/spatial-model-editor/spatial-model-editor/issues/484), [#485](https://github.com/spatial-model-editor/spatial-model-editor/issues/485), [#486](https://github.com/spatial-model-editor/spatial-model-editor/issues/486)
- incorrect compartment name displayed in simulate display options [#487](https://github.com/spatial-model-editor/spatial-model-editor/issues/487)
- simulation image intervals changing length of simulation [#492](https://github.com/spatial-model-editor/spatial-model-editor/issues/492)
- bug where removing compartment could cause a crash [#506](https://github.com/spatial-model-editor/spatial-model-editor/issues/506)
- artefacts in import of species concentrations into simulation [#508](https://github.com/spatial-model-editor/spatial-model-editor/issues/508)
- DUNE maximum iteraction count simulation error [#490](https://github.com/spatial-model-editor/spatial-model-editor/issues/490)

## [1.0.9] - 2021-04-06
### Added
- open/save as `.sme` filetype: contains both the model and the simulation results [#461](https://github.com/spatial-model-editor/spatial-model-editor/issues/461)
- support for time-based events: parameters and species concentrations can be set at specified times in the simulation
- diagnostic info & image when a simulation fails [#432](https://github.com/spatial-model-editor/spatial-model-editor/issues/432)
- zoom option for Boundary and Mesh images in the Geometry Tab
- new option in Simulation Tab to export concentrations to model as initial concentrations

### Changed
- simulation is no longer reset when the Simulate tab is left in the GUI
- simulations can be continued using a different simulator
- use v1.0.0 of Dune-Copasi simulator
- GUI warns before closing a model with unsaved changes [#346](https://github.com/spatial-model-editor/spatial-model-editor/issues/346)
- Membrane names can now be edited in the GUI and in the python interface

### Fixed
- diffusion constant bug in the Pixel simulator [#468](https://github.com/spatial-model-editor/spatial-model-editor/issues/468)
- bug in dune-copasi mesh generation that could cause a crash [#434](https://github.com/spatial-model-editor/spatial-model-editor/issues/434)
- bug where unused diffusion constant parameter was sometimes not removed from sbml document
- data race in generating simulation concentration images [#434](https://github.com/spatial-model-editor/spatial-model-editor/issues/434)

### Removed
- incomplete Parametric geometry sbml import support [#452](https://github.com/spatial-model-editor/spatial-model-editor/issues/452)

## [1.0.8] - 2021-02-10
### Added
- python library can now import a new geometry image using `sme.Model.import_geometry_from_image()`

### Changed
- mesh generation is now done using the CGAL library

### Removed
- dependence on Triangle mesh library removed due to its non-free license

## [1.0.7] - 2021-02-07
### Fixed
- illegal instruction crash on old CPUs [#422](https://github.com/spatial-model-editor/spatial-model-editor/issues/422)

## [1.0.6] - 2021-01-29
### Added
- python library `sme.Model.simulate`:
  - simulation results now also contain rate of change of concentrations
  - option to return partial results instead of throwing on simulation timeout

### Changed
- more accurate initial concentration interpolation for dune-copasi simulations
  - applies to both GUI simulations and exported TIFF files for stand-alone dune-copasi simulation
- improved performance of interior point determination in mesh construction

### Fixed
- meshing bug where invalid 2-point boundary line could cause crash

## [1.0.5] - 2021-01-02
### Added
- user can edit dune-copasi Newton solver parameters
- Pixel simulator supports reactions that explicitly depend on t, x, y

### Changed
- dune-copasi simulations now support membrane flux terms
  - rectangular membrane compartments no longer required
  - simpler mesh generation
  - faster and more accurate simulations
- improved boundary construction
  - use pixel edges as boundary edges instead of pixels
  - shared boundaries and their end points now unambiguously determined
- improved line simplification

### Fixed
- bug in scaling of membrane reactions in Pixel simulations
  - added missing `1/a` factor, where `a` is the width of a pixel in model units

### Removed
- inconsistent `RateRule` support in SBML import

## [1.0.4] - 2020-10-22
### Added
- simulation display options are now saved

### Changed
- python library API
  - lists now have name-based look-up, old name-based look-up functions removed
  - examples of old -> new change required in user code:
    - `model.compartment("name")` -> `model.compartments["name"]`
    - `model.specie("name")` -> `model.species["name"]`

### Fixed
- species removal bug

## [1.0.3] - 2020-10-19
### Added
- python library example notebooks

### Changed
- python library performance improvements

### Fixed
- python library segfault / missing data issue on some linux platforms

## [1.0.2] - 2020-10-16
### Added
- python library documentation and example notebooks
- support for import of geometry image with alpha channel / transparency
- display compartment size and membrane length

### Changed
- python library performance improvements

### Fixed
- normalisation of simulation concentration images
- geometry imported from another model not saved bug
- python library segfault / missing data issue on linux
- reaction removal bug

## [1.0.1] - 2020-10-5
### Added
- membranes to python library
- compartment pixel mask to python library
- timeout parameter to simulate function in python library

## [1.0.0] - 2020-10-2
First official release.

Changelog format based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).
