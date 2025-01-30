Brusselator model
==================
The `Brusselator model <https://en.wikipedia.org/wiki/Brusselator>`_ is a theoretical model for a type of autocatalytic reaction that is capable of forming patterns. The best known real world examples is the `Belousov-Zhabotinsky reaction <https://en.wikipedia.org/wiki/Belousov%E2%80%93Zhabotinsky_reaction>`_. In this model we show how local and global parameters can be mixed in a model definition, and how special events can be set to happen at a certain time point. Explore how these work in the `Events` tab in the GUI. It consists of six species :math:`X`, :math:`Y`, :math:`A` :math:`B`, :math:`D` and :math:`E`. :math:`k_{i}` are reaction parameters.

Formulation
"""""""""""

.. math::
    &\frac{\partial A}{\partial t} = D_{A} \nabla^2 A - A k_{1}

    &\frac{\partial B}{\partial t} = D_{B} \nabla^2 B - B X k_{3}

    &\frac{\partial X}{\partial t} = D_{X} \nabla^2 X + A k_{1} - B X k_{3} + X^{2} Y k_{2} - X k_{1}

    &\frac{\partial Y}{\partial t} = D_{Y} \nabla^2 Y + B X k_{3} - X^{2} Y k_{2}

    &\frac{\partial D}{\partial t} = D_{D} \nabla^2 D + B X k_{3}

    &\frac{\partial E}{\partial t} = D_{E} \nabla^2 E + X k_{1}

Example Snapshot
"""""""""""""""""
.. figure:: img/brusselator.png
    :alt: screenshot of the final step of the Brusselator example model

    The final step of a run of the Brusselator model in SME.
