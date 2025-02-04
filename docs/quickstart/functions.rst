Defining additional functions
=============================
It sometimes makes sense to extract parts of the definition of a reaction into a separate function, especially if it is reused across multiple reactions.

To define a function, go the `Functions` tab. The logic follows what we have seen for reactions and parameters.
Click the 'Add' button at the bottom and name your function before clicking `OK`.
Then, you can define the function in the editor on the right. Start by adding all the variables the function depends on in the table below the `Name` field.
Proceed by defining the function in the input field below the parameters table.
The newly defined function can then be used in the definition of :doc:`Reactions <./reactions.html>` by using it's name like other variables but followed by parantheses that contain the arguments of the function. These arguments can be spatial coordinates, concentrations, or parameters.
Therefore, they can define spatial fields or other quantities that are not directly available in the reaction editor.
See the video below for how to use functions in the GUI.

.. note::
    Functions behave like parameters defined in the `Parameters` tab. They are global and can be used in all reactions.
    For an example that showcases the use of reactions, refer to the `circadian-clock` or the `liver-simplified` example model.

.. figure::
    img/function-definition.apng
    :alt: screenshot of the functions in a model

    How to define a function of three variables.
