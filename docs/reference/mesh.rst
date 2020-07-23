Mesh generation
===============

Generating a triangular mesh for the dune-copasi solver from a pixel image of the compartment geometry involves multiple steps:

- identify contours of compartment boundaries
- split contours into lines
- merge adjacent lines
- merge adjacent end points
- simplify boundary lines by removing points
- generate membrane compartments
- triangulate

These steps are described in more detail below.

Contour detection
-----------------

The first step in generating the mesh is to identify the set of contours that make up the boundaries of each compartment. Each contour is a closed loop of connected pixels that makes up a boundary between a compartment and the rest of the model. Each compartment has at least one contour around its outer boundary, and it may also contain inner contours around any holes in the compartment shape.

The contour tracing is done using the `findContours <https://docs.opencv.org/2.4/modules/imgproc/doc/structural_analysis_and_shape_descriptors.html#findcontours>`_ routine from the `OpenCV <https://opencv.org/>`_ library, which implements the method described in `Suzuki et. al. <https://www.sciencedirect.com/science/article/abs/pii/0734189X85900167>`_. This method returns an ordered set of pixels for each contour. The outer contour traces the outer boundary in the anti-clockwise direction, while the inner contours trace each inner boundary in a clockwise direction.

Contour splitting
-----------------

The contours are then split into lines which have a consistent neighbouring contour (including not having a neighbouring contour). For example, if part of contour `A` is adjacent to contour `B`, and the rest of contour A has no neighbouring contours, it will be split into two sections, one line for the part adjacent to contour `B`, and another line for the rest of the contour.

Contour merging
---------------

Next, adjacent contours need to be merged into a single boundary line. Adjacent contours are determined as the section of the appropriate contour with the smallest distance between the oriented pixels, and once a pair has been identified, one of the lines is removed to leave a single boundary line.

At the end of this step, we are left with a collection of boundary lines, some of which are closed loops, some not. All instances of adjacent contours have been replaced with a single boundary line of some kind.

End point merging
-----------------

At places where three compartments meet, we would like to have three non-loop boundary lines meeting at a point. However, from the previous step, we will have three such lines that terminate near to each other, but typically separated by a pixel or two.

Here we take an end point, then determine the two closest other end points to it, and merge the three points. This is done until all such end point triplets have been merged. If the new end point is not adjacent to the end of the line, pixels are added in a straight line to connect the line to its new end point.

Note: currently we assume all end points come in triplets - ideally we should take into account possible 2- or 4-point merging.

Boundary simplification
-----------------------

Once we have identified all of our boundaries, we want to simplify them by removing points from the boundary. We do this using `Visvalingam-Whyatt polyline simplification <https://www.tandfonline.com/doi/abs/10.1179/000870493786962263>`_. The algorithm starts by calculating the area of the triangle formed by each point on the boundary with its two nearest neighbouring points. Then at each step:
- the point with the smallest area is removed
- the two neighbouring points areas are recalculated
- the larger of the previous and the new area is used
This allows us to order the points in the boundary by their order of importance, and then the user can adjust the number of points used for each boundary according to how accurately they wish the represent the original boundaries.

Membrane boundaries
-------------------

[TODO]

Mesh generation
---------------

We use the `Triangle <https://www.cs.cmu.edu/~quake/triangle.html>`_ library to generate the triangular mesh. This generates a constrained conforming Delaunay triangulation (CCDT) from the boundary lines, by inserting points inside the compartments and triangulating them. If necessary it will also add aditional points on the boundary lines (known as Steiner points).

We currently require membrane boundaries to be represented by a quadrilateral mesh of single-element thickness. To generate these, we first generate an initial mesh of the boundaries, then discard all interior points but keep any inserted Steiner points along the boundaries. Next, any membrane boundaries are doubled and shifted apart, to give matching inner/outer points which can be connected with straight lines to form a single-element thick quadrilateral membrane compartment. The rest of the compartments (exluding these new membrane compartments) are then retriangulated, but this time disallowing the creation of Steiner points, to ensure that the points making up the quadrilateral elements are not altered. The user can then adjust the maximum allowed triangle area for each compartment, the number of points used to approximate a boundary, or the width of a membrane compartment.
