Defining parameters
=================== 
In the last section, we saw how to define parameters for reactions that are relevant for only the term reaction term we are currently concerned with. 
This can get cumbersome when the whole system of equations shares a number of parameters, since we have to redefine them for each reaction. 

To alleviate this, SME maintains a list of global parameters which you can find in the `Parameters` tab.
To define a new global parameter, click on the `Add` button below the list of parameters. Give it a suitable name and click `OK`. The new parameter will appear in the list. On the right, you can change that name and set the value of the parameter. Then, the new parameter can be used in every reaction that you define in the reactions tab. 

.. note:: 
    Parameters defined in this way do not have units! You have to make sure that you use them consistently in the reactions. Parameters with units should not go here! 

.. figure:: 
    img/parameter-definition.apng
    :alt: Video of how to define parameters using the GUI 

    How to define a parameter in the GUI.