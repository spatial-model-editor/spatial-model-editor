Overview
==============
In the following, each example model that comes with SME is described in more detail. The models have different levels of complexity and highlight different use cases and element of SME, and there are 2D and 3D models.
They are roughly ordered by complexity, starting with the most simple models. 2D models are treated first.
In the mathematical formulation of all these models, :math:`D_{x}` always denotes diffusion coefficients and :math:`j_{x, A \rightarrow B}` fluxes across membranes from compartment :math:`A` to compartment :math:`B`.

2D models
---------
- `Single compartment diffusion <../examples/singlediff.html>`_
    Showcases a simple diffusion system in a single compartment with two species.
- `AB-to-C Reaction model <../examples/AB-to-C.html>`_
    Shows how to define reactions in the bulk volume of compartments.
- `Gray-Scott pattern formation model <grayscott.html>`_
    A model for pattern formation in a reaction-diffusion system in a single compartment without exchange with the outside. This can be used to explore the phenomenology of nonlinear reaction diffusion systems.
- `A very simple model <../examples/verysimple.html>`_
    A system in which two species react in a compartment and are exchanged across membranes. It's mathematically simple, placing the emphasis on showing how to create exchange terms across membranes.
- `Brusselator model <../examples/brusselator.html>`_
    A model for a special type of autocatalytic reaction which can create patterns. Here, it is used to showcase the usage of singular events in the GUI and how local and global parameters can be mixed.
- `Liver cells model <../examples/livercells.html>`_
    Shows how to work with more complex geometries with different compartments and membranes and the definition of global parameters.

The following two models treat more realistic cases and can serve as starting points for similar systems.

- `Circadian clock model <../examples/circadian.html>`_
    This system models the circadian rythm of a cell. It defines a large number of reactions in a single compartment and shows how to define complex reaction systems.

- `Simplified liver cell reaction model <../examples/liver.html>`_
    This represents a simplified version of liver cell metabolic reactions.
    It again exemplifies the definition of complex metabolic reaction systems with differen parameters and functions in different compartments, but also adds interactions via membrane exchange.

3D models
---------
The 3D examples do not show any fundamentally new features of SME. Rather, they exist to get familiar with the 3D representation in SME and the handling of 3D geometries. Keep in mind that three dimensional models are compuationally significantly more expensive to solve.
- `Single compartment diffusion in 3D (same as in 2D) <../examples/singlediff.html>`_
    A 3D version of the single compartment diffusion model. Get familiar with the 3D representation of domains and concentrations in this one.
- `Selkov model for glycolysis oscillation <../examples/selkov.html>`_
    A model for glycolysis oscillations in a single compartment. This model is more complex than the previous one which shows bulk osciallations in the concentration of species.
- `Gray Scott pattern formation in 3D <../examples/grayscott.html>`_
    A 3D version of the Gray-Scott pattern formation model.
- `Fitzhugh-Nagumo model <../examples/fitzhughnagumo.html>`_
    A more complex pattern formation model in a 3D domain with 2 compartments and with membrane exchange. It is computationally more expensive than the other models. Use it to explore 3D pattern formation with membrane fluxes.
- `Calcium wave model <../examples/calciumwave.html>`_
    Mostly cited as modeling signal propagation in cells (especially neurons), this model has been augmented with a diffusion term here to turn it into a simple spatial system. This explores membrane exchange in 3D.
