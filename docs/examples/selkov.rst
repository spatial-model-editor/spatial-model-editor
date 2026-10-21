Sel'Kov Model
=============
This model represents a 3D spatial version of the Sel'Kov model for glycolysis to which a diffusion term has been added. As defined there is no exchange between the two domains.
With the preset parameters, this model results in bulk oscillations in the two media with different characteristic timescales due to the different values of the parameters :math:`\rho_{i}` and :math:`\sigma_{i}` in each domain.

Formulation
""""""""""""""
.. math::
    &\frac{\partial p_{i}}{\partial t} = D_{p_{i}} \nabla^2 p_{i} + \rho_{i} - \sigma_{i} p_{i} - p_{i} s_{i}^{2}

    &\frac{\partial s_{i}}{\partial t} = D_{s_{i}} \nabla^2 s_{i} - s_{i} + p_{i} s_{i}^{2} + \sigma_{i} p_{i}

    &j_{p, 1 \rightarrow 2} = 0

    &j_{p, 2 \rightarrow 1} = 0

    &j_{s, 1 \rightarrow 2} = 0

    &j_{s, 2 \rightarrow 1} = 0

Example snapshot:
^^^^^^^^^^^^^^^^^
.. figure:: img/selkov.png
   :alt: screenshot of the final step of the Selkov-glycolysis oscillation example model

   The last timestep of the Selkov-glycolysis oscillation model in a 3D domain.
