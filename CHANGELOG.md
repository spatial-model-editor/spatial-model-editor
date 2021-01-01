# Changelog

## [latest]
### Added

### Changed

### Removed

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
  - use pixel edges as boundary edges insted of pixels
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
