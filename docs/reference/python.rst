Python Interface
================

A Python interface is available, which can be installed from `PyPI <https://pypi.org/project/sme/>`_ with

.. code-block:: bash

    pip install sme

.. note::

    If your python version or platform is not supported, please submit a `feature request <https://github.com/spatial-model-editor/spatial-model-editor/issues/new?assignees=&labels=&template=feature_request.md&title=add%20support%20for%20new%20Python%20platform>`_

Once it is installed, you should be able to import the ``sme`` module and load the built-in example model:

.. code-block:: python
  :emphasize-lines: 5

    $ python
    Python 3.8.6 (default, Sep 28 2020, 09:32:50)
    [GCC 10.0.1 20200416 (experimental) [master revision 3c3f12e2a76:dcee354ce56:44 on linux
    Type "help", "copyright", "credits" or "license" for more information.
    >>> import sme
    >>> model = sme.open_example_model()
    >>> print(model)
    <sme.Model>
      - name: 'Very Simple Model'
      - compartments:
         - Outside
         - Cell
         - Nucleus

.. tip ::

    There is an online `colab notebook <https://colab.research.google.com/github/spatial-model-editor/spatial-model-editor/blob/master/sme/sme_getting_started.ipynb>`_ where you can try it out in your browser without installing anything on your computer.

As shown in the `getting started notebook <https://colab.research.google.com/github/spatial-model-editor/spatial-model-editor/blob/master/sme/sme_getting_started.ipynb>`_, ``sme`` allows you to import an existing model, change the value of parameters in the model, simulate the model and produce images of the species concentrations from the simulation.

To get help on an object, its methods and properties, use the help function, e.g. ``help(sme.Model)``.
