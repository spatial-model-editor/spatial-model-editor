Units
=====

.. toctree::
   :maxdepth: 2
   :caption: Contents:

The quantity of a species can be defined in terms of *amount* or in terms of *concentration*.

In a spatial simulation, as the diffusion equation is a function of the concentration we choose to use concentration as our unit of species quantity. Ensuring that units are consistent when importing an existing SBML model requires some care.

SBML
----

When importing a SBML model, if a species has an `initialAmount`, this is converted to an `initialConcentration` by dividing the amount by the compartment volume.

If `hasOnlySubstanceUnits` is `true` then when the species identifier appears in a mathematical expression it refers to the *amount*. In this case we need to first multiply our value for the species *concentration* by the compartment volume before inserting this value into any mathematical expressions where it is used. Additionally, if the result of the expression is the rate of change for a species in units of *amount* per time, we need to divide this by the compartment volume to get the *concentration* per time.

See the SBML `cpp-api class_species <http://sbml.org/Software/libSBML/5.18.0/docs/cpp-api/class_species.html>`_ for more details on the SBML specification.