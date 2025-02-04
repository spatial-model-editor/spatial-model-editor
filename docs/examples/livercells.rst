Liver cells
===========
This model uses a more complex 2D geometry than seen before that consists of a row of cells at the top and bottom of the domain with a undulating channel and small gaps between the cells. Each cell additionally has a nucleus.

This model can serve as a playground,e.g., for working with exchange processes complicated geometries. The mathematical model is geared towards simplicity and does not represent a real physical system. There is one species :math:`g` which exists in all three compartments `nuclear`, `sinosoid` and `hepatocyte`.
There are no bulk reactions, only membrane fluxes and diffusion in the bulk volume, which has the same form everywhere. In the following, the abbreviations nuclear (n), sinosoid (s), hepatocyte (h) are used. :math:`c` and :math:`k` are reaction parameters.

Formulation
""""""""""""""
.. math::
   &\frac{\partial g_{n,s,h}}{\partial t} = D_{g_{n,s,h}} \nabla^2 g_{n,s,h}

   &j_{g, h \rightarrow s} = k c \left( g_{h} - g_{s} \right)

   &j_{g, n \rightarrow h} = k c \left( g_{n} - g_{h} \right)



Example Snapshot
"""""""""""""""""
.. figure:: img/livercells.png
   :alt: screenshot of the final step of the liver-cells example model

   Result of running the liver-cells model in SME.
