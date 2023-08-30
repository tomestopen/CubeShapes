# CubeShapes

This project can be used for the generation and display of polycubes (https://en.wikipedia.org/wiki/Polycube).
It is inspired by the similar project by Mike Pound (https://github.com/mikepound/cubes/blob/main/cubes.py) and the associated ComputerPhile video.

To run it, first compile the c code into a shared library (tested as a DLL on windows but should work as an .so on linux).
Then call the shapeFinder module to create archives for the cube count you want (ex: python shapeFinder.py 6).
You can then call the shapeVision module to display any of the shapes in the archive (ex: python shapeVison.py 6).

You can also create instances of the ShapeFinder and ShapeVison classes in the python interpreter to access more functions.

The shapeVision module requires matplotlib
