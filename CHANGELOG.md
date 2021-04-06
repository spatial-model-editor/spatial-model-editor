# Changelog

## [latest]
### Added

### Changed

### Removed

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
