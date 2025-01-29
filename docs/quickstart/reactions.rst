Defining reactions
==================
Models in SME are always of the reaction diffusion type. For each chemical species in the system, there is on equation that describes how the species concentration changes over time and space. Hence, we end up with a system of coupled partial differential equations (PDEs).
Reactions in a compartmentalized spatial model come in two types:

* Bulk volume reactions
   * take place everywhere within a compartment
   * units: rate of change of species **concentration**: :math:`\frac{mol}{L \cdot s}`

* Membrane reactions
   * take place on the membrane between two compartments
   * units: rate of species **amount crossing unit area** of the membrane :math:`\frac{mol}{m^2 \cdot s}`

For more on the mathematical formulation, see `here <../reference/maths.html>`_.

To define a reaction, we have to follow six steps:

- click on the `Reactions` tab in the main window. You will see a rendering of the geometry of the selected compartment on the right. Scroll through the z-axis of a 3D model to see it's geometry slice by slice.

- In the middle, a list of different compartments is displayed. Volumes are displayed first, followed by membranes that link them. Select a compartment in which to define a volume reaction. Then click the 'Add' button below, choose a name for the reaction, and click 'OK'.

- On the right side, you see the model editor menu. At the top, the name you just chose is displayed. Below, you find a dropdown menu that lets you select the compartment in which the reaction is defined.

- Next, skip to the bottom of the window. There, you find the table of parameters for the reactions. Put in all the parameters that are relevant for this one reaction.

- Then, click into the input field below that which is labeled 'Rate'. Enter the rate at which the reaction happens. The units are displayed below the input field.

- Lastly, go back up to the field labeled 'Species' below the compartments dropdown list. You will find there a list of Species in the left column. The right column is labeled 'Stoichiometry'. Here you can define the stoichiometry of the reaction for the involved species. Since often the parameters of the equation take care of this, these often represent the coefficients of the reaction in the equation.

.. note::
    When the same reaction term appears in multiple reactions for different species, adding the respective stoichiometric coefficient in this menu takes care of adding the term to all relevant equations.

.. note:: 
   To become more acquainted with the way the stoichiometric coefficients work, have a look at `this example model <../examples/verysimple.html>`_.

Follow the same procedure to define a membrane reaction. See the figure below for how all this looks like in the GUI.

.. note::
    Take care to get the units of your parameters and rates right. Note how they differ between bulk volume and membrane reactions.


.. figure:: img/reactions.apng
   :alt: screenshot of the reactions in a model

   Editing the reactions in a spatial model.
