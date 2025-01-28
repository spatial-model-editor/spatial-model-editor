Find your bearings
==================
This guide will help you get started with the graphical user interface of spatial-model-editor.
For this to make sense, we need to understand the basic model structure first.

Model structure in spatial-model-editor
---------------------------------------
A model in spatial-model-editor consists of five basic parts which all must be defined in order to run a simulation:

#. A definition of the spatial domain the model should run on. This can be 2D or 3D and can be divided into an arbitrary number of compartments which have interfaces with each other. In the cellular context that is relevant here, these interfaces can represent membranes between cell compartments or cells.

#. A set of chemical species whose concentration is defined in different compartments. These concentrations are assumed to undergo diffusion by default. These chemical species need to have an initial distribution of concentration in the compartments they are defined in and an isotropic, homogeneous diffusion constant.

#. The chemical reactions that can occur between the different species. These can be divided into reactions in the volume of the compartments and fluxes and reactions that only happen on the interfaces between compartments.

#. The chemical reactions between species typically come with a number of parameters that we need to set to defined numerical values. Often, these define the rates at which a reaction occurs.

#. How the model equations defined in the last three steps should be solved numerically in space and time. This means we have to choose a time interval in which to solve the equations and algorithm to do so.

The graphical user interface of spatial-model-editor follows this basic structure. For each of the steps, there is a tab in the main window that allows you to define the corresponding part of the model. 

Additional functions for each step are available via the menu bar at the top of the main window. 

- The `File` menu lets you load and save models for future use or load example models from the model library. 

- The `Import` menu provides dialogs to import the spatial geometry of the problem from an exisitng SME model or from an image. 

- The `Tools` menu lets you edit various fundamental elements of the model, like the basic units it works with, the geometry or the names of spatial coordinates. It also gives you access to parameter optimation algorithms. See `here <../reference/parameter-fitting.html>`_ for more details. 

- Under `View` you find various options to change the way the model variables and geometry is displayed. 

- The `Advanced` menu lets you customize the numerical method used to solve the model equations and the mesh generator. See `here <../reference/mesh.html>`_ for more details on the discretization algorithm as well as `here <../reference/dune.html>`_ and `here <../reference/pixel.html>`_ for more details on the numerics.

.. figure:: img/sme-gui.png
   :alt: screenshot showing main window of spatial-model-editor

   The main window of the spatial-model-editor. The five tabs at in the upper center correspond to the five parts of the model that need to be defined.


In the following, you will learn how to define your own model in spatial-model-editor by following the above scheme.
If instead you are more interested in turning an existing non-spatial model defined in COPASI or SBML into a spatial model and run it via spatial-model-editor, you can skip ahead and go the respective `user guide <../userguides/work-with-copasi.html>`_ right away. However, we recommend to work through this guide once because parts of it are needed to make predefined ODE models work in SME.
