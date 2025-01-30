
Single compartment diffusion
============================
This example model does consists of pure diffusion without any reaction terms. It shows the baseline behavior of the diffusion solver in a simple 2D domain.
The system consists of a fast-diffusing species :math:`s`  and a slowly diffusing species :math:`f`.

Formulation
"""""""""""

   .. math::
      &\frac{\partial s}{\partial t} = D_{s} \nabla^2 s

      &\frac{\partial f}{\partial t} = D_{f} \nabla^2 f


Example Snapshot
"""""""""""""""""
.. figure:: img/singlecompartment2d.png
   :alt: screenshot of the final step of the single-compartment diffusion example model

   The last timestep of the single-compartment diffusion model. Both species initial concentrations overlap. Play around with the display options to see the difference in diffusion rates.
