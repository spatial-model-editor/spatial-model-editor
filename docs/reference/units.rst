Units
=====

Fundamental Units
-----------------

To describe a spatial model we need to define the following fundamental units:

- ``amount`` (e.g. `Mole`)
- ``length`` (e.g. `metre`)
- ``time`` (e.g. `second`)

Volume is not a fundamental unit. However, for user convenience we treat volume as a fundamental unit

- ``volume`` (e.g. `litre`)

This allows the use of units such as `cm` for length and `mMol/mL` for concentration, instead of the equivalent but less common `mMol/cm^3`.

Derived Units
-------------

All quantities in the model have units that can be written as some combination of these fundamental units:

- species concentrations
    - have units of ``amount`` / ``volume``
- reactions *inside* a compartment
    - describe the rate of change of the concentration of species
    - have units of concentration / time, i.e. ``amount`` / ``volume`` / ``time``
- reactions *between* two compartments
    - describe that rate at which a unit amount of species crosses a unit area of the *membrane*
    - where the membrane is the area where the two compartments touch each other
    - have units of amount / membrane-area / time, i.e. ``amount`` / ``length`` / ``length`` / ``time``
- diffusion constants
    - have units of `area / time`, i.e. ``length`` * ``length`` / ``time``

More information
----------------

For more information see :download:`units.pdf <units.pdf>`
