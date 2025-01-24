Species properties
==================
The next step is to define the chemical species present in the system. To do this, click on the `species` tab. 
On the left, you see a list of compartments in the model geometry. Select the one for which you want to define species. This species will only exist in this compartment. As a consequence, even if the species in all compartments are chemically the same, you have to define them separately. 

To create a new species, click the `Add` button, select a suitable name, and hit enter. The new species will appear below the compartment name. 

On the right hand side, you see the Species Settings menu. Here you can define all relevant properties of the species. 

At the top, you can rename the species or assign it to a different compartment. You can also select whether it should be constant for the whole simulation or be variable. Constancy may be a useful simplifying assumption in some models if, for example, the species is produces far more quickly than the characteristic simulation timescale. 

.. figure:: img/define-species.apng 
   :alt: video showing species settings

   An example of defining a new species in a compartment together with its initial conditions and diffusion constant

Two more properties of a species are of primary importance. 
First is the initial concentratoin of the species. This initial condition can be set to be a constant value, an analytic expression over the spatial coordinates `x` and `y` (and `z` for 3D images) or an image that's loaded from disk.  FIXME: check if 3D images are supported 

The last property of the speices to be set is its diffusion constant. The diffusion term is always implicit in the definition of the model (see the `documentation on the mathematical formulation <../reference/maths.html>`_ for more details). The diffusion constant is assumed to be isotropic and homogeneous. If you don't want diffusion to be present, set this constant to zero.

.. figure:: img/concentration.apng
   :alt: screenshot showing species concentration settings

   An example of different ways to specify the initial spatial distribution of a species concentration.
