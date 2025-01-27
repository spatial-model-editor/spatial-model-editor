How to create your own geometry
===============================
*SME* can work with png images for 2D- and tiff images for 3D geometries. You can create these in whatever way you feel comfortable with. Here, we are going to use Python to create example geometries. We will use the `numpy <https://numpy.org/>`_ and `matplotlib <https://matplotlib.org/>`_ libraries to create a 2D geometry and save it as a png image. For the 3D tiff image, we will use
`tifffile <https://pypi.org/project/tifffile/>`_ which allows us to save a 3D array to a tiff image.

Create a 2D Geometry
--------------------
We first import the necessary models.
.. code-block:: python
    import numpy as np
    import matplotlib.pyplot as plt

Next, we create a 2D numpy array that represents the geometry. We will use a simple example of a 2D geometry with a circle in the middle.
.. code-block:: python
    # create a 2D numpy array
    size = 100
    geometry = np.zeros((size, size))
    for i in range(size):
        for j in range(size):
            if (i - size // 2) ** 2 + (j - size // 2) ** 2 < 20 ** 2:
                geometry[i, j] = 1

Finally, we plot the geometry and save it as a png image.
.. code-block:: python
    # plot the geometry
    plt.imshow(geometry, cmap="gray")
    plt.axis("off")
    plt.savefig("2d_geometry.png", bbox_inches="tight", pad_inches=0)
    plt.show()

Create a 3D Geometry
--------------------
We follow the same steps as for the 2D geometry: We define the basic shape of the domain and then create functions that defines the various compartments in the domain. Here, we will create 2 spheres that intersect each other.
This time, we use `tifffile` instead of matplotlib:
.. code-block:: python
    import tifffile as tiff
    import numpy as np

Then, we create the 3D domain and the compartments.
.. code-block:: python
    arr = np.zeros((100, 100, 100))

    def sphere(x,y,z, x0=50, y0=50, z0=50, r=25):
        return (x-x0)**2 + (y-y0)**2 + (z-z0)**2 < r**2

    for i in range(100):
        for j in range(100):
            for k in range(100):
                arr[i,j,k] = sphere(i, j, k, 40, 40, 40) or sphere(i, j, k, 60, 60, 60)

Finally, we save the 3D geometry as a tiff image.
.. code-block:: python
    tiff.imwrite('/home/hmack/Seafile/project_resources/SME/3d_skewed_hourglass.tiff', arr)
