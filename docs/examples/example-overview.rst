Overview
==============
In the following, each example model that comes with SME is described in more detail. The models have different levels of complexity and highlight different use cases and element of SME, and there are 2D and 3D models.
They are roughly ordered by complexity, starting with the most simple models. 2D models are treated first.
In the mathematical formulation of all these models, :math:`D_{x}` always denotes diffusion coefficients and :math:`j_{x, A \rightarrow B}` fluxes across membranes from compartment :math:`A` to compartment :math:`B`.

2D models
---------

- :doc:`Single compartment diffusion <./singlediff>`
    Showcases a simple diffusion system in a single compartment with two species.
- :doc:`AB-to-C Reaction model <./AB-to-C>`
    Shows how to define reactions in the bulk volume of compartments.
- :doc:`Gray-Scott pattern formation model <./grayscott>`
    A model for pattern formation in a reaction-diffusion system in a single compartment without exchange with the outside. This can be used to explore the phenomenology of nonlinear reaction diffusion systems.
- :doc:`A very simple model <./verysimple>`
    A system in which two species react in a compartment and are exchanged across membranes. It's mathematically simple, placing the emphasis on showing how to create exchange terms across membranes.
- :doc:`Brusselator model <./brusselator>`
    A model for a special type of autocatalytic reaction which can create patterns. Here, it is used to showcase the usage of singular events in the GUI and how local and global parameters can be mixed.
- :doc:`Liver cells model <./livercells>`
    Shows how to work with more complex geometries with different compartments and membranes and the definition of global parameters.

The following two models treat more realistic cases and can serve as starting points for similar systems.

- :doc:`Circadian clock model <./circadian>`
    This system models the circadian rhythm of a cell. It defines a large number of reactions in a single compartment and shows how to define complex reaction systems.

- :doc:`Simplified liver cell reaction model <./liver>`
    This represents a simplified version of liver cell metabolic reactions.
    It again exemplifies the definition of complex metabolic reaction systems with difference parameters and functions in different compartments, but also adds interactions via membrane exchange.

3D models
---------
The 3D examples do not show any fundamentally new features of SME. Rather, they exist to get familiar with the 3D representation in SME and the handling of 3D geometries. Keep in mind that three dimensional models are computationally significantly more expensive to solve.

- :doc:`Single compartment diffusion in 3D (same as in 2D) <./singlediff>`
    A 3D version of the single compartment diffusion model. Get familiar with the 3D representation of domains and concentrations in this one.
- :doc:`Selkov model for glycolysis oscillation <./selkov>`
    A model for glycolysis oscillations in a single compartment. This model is more complex than the previous one which shows bulk oscillations in the concentration of species.
- :doc:`Gray Scott pattern formation in 3D <./grayscott>`
    A 3D version of the Gray-Scott pattern formation model.
- :doc:`Fitzhugh-Nagumo model <./fitzhughnagumo>`
    A more complex pattern formation model in a 3D domain with 2 compartments and with membrane exchange. It is computationally more expensive than the other models. Use it to explore 3D pattern formation with membrane fluxes.
- :doc:`Calcium wave model <./calciumwave>`
    Mostly cited as modeling signal propagation in cells (especially neurons), this model has been augmented with a diffusion term here to turn it into a simple spatial system. This explores membrane exchange in 3D.
