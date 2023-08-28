import os
import pathlib
import platform
from ctypes import Structure, c_int, c_char, c_ubyte, POINTER, CDLL

class CubeShape(Structure):
	#this class represents a cube shape structure in the shared library
	_fields_ = [("value", c_int), ("width", c_int), ("height", c_int), ("depth", c_int), ("shape", POINTER(c_char))]

class CubeShapesSL:
	#this class is used to access the functions in the shared library
	def __init__(self, path = None):
		#check the operating system and get the path for the shared library
		system = platform.system()
		if system == "Windows":
			slext = "dll"
		elif system == "Linux":
			slext = "so"
		else:
			print("Must run on windows or Linux system\n")
			self.isValid = False
			return
		if path is None:
			path = os.path.join(pathlib.Path(__file__).parent.absolute(), "CubeShapes." + slext)
		#try to load the shared library from the path
		try:
			self.SL = CDLL(path)
			self.isValid = True
		except Exception as e:
			print("There was an error loading the shared library at " + path + "\n" + str(e) + "\n")
			self.isValid = False
		#if the shared library was successfully loaded, we specify the return types and arguments for the various functions
		if self.isValid:
			#load the get descendents function
			self.getDescendents = self.SL.GetDescendents
			self.getDescendents.argtypes = [POINTER(POINTER(CubeShape)), POINTER(CubeShape), c_int]
			self.getDescendents.restype = c_int
			#load the clean shape list function
			self.cleanShapeList = self.SL.CleanShapeList
			self.cleanShapeList.argtypes = [POINTER(CubeShape), c_int]
			#load the compare shape list function
			self.compareShapeLists = self.SL.CompareShapeLists
			self.compareShapeLists.argtypes = [POINTER(CubeShape), c_int, POINTER(CubeShape), c_int, POINTER(CubeShape)]
			self.compareShapeLists.restype = c_int
			#load the set shape list values function
			self.setShapeListValues = self.SL.SetShapeListValues
			self.setShapeListValues.argtypes = [POINTER(CubeShape), c_int]
			#load the check distinct function
			self.checkDistinct = self.SL.CheckDistinct
			self.checkDistinct.argtypes = [POINTER(CubeShape), POINTER(CubeShape)]
			self.checkDistinct.restype = c_int
			#load the multi threaded get descendents function, (if on windows)
			if system == "Windows":
				self.getDescendentsMulti = self.SL.GetDescendentsMulti
				self.getDescendentsMulti.argtypes = [POINTER(POINTER(CubeShape)), POINTER(CubeShape), c_int, c_int]
				self.getDescendentsMulti.restype = c_int