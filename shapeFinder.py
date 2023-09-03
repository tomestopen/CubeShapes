import os
import pathlib
import time
import sys
from CSAdapt import CubeShape, CubeShapesSL
from ctypes import POINTER, pointer, c_char, cast
from datetime import datetime, timezone

class ShapeFinder:

	def __init__(self):
		#get the cube shapes library
		self.CSSL = CubeShapesSL()
		#find current directory path
		sourcePath = pathlib.Path(__file__).parent.absolute()
		#add class function shortcuts
		self.getArchivePath = ShapeFinder.getArchivePath
		self.shapeArrayToList = ShapeFinder.shapeArrayToList
		self.atol = ShapeFinder.shapeArrayToList
		self.loadTextArchive = ShapeFinder.loadTextArchive
		self.saveTextArchive = ShapeFinder.saveTextArchive
		self.extract = ShapeFinder.extract
		#check that the shapes directory exists, create it if it doesn't
		self.shapeDirPath = os.path.join(sourcePath, "shapes/")
		if not os.path.isdir(self.shapeDirPath):
			os.mkdir(self.shapeDirPath)

	def find(self, cubeCount, threadCount = 1):
		#this function tries to find the all the unique shapes that can be made with the specified amount of cubes
		#first we check that a shapes file is present in the shapes directory
		found = False
		cubeCountP = cubeCount
		while not found and cubeCountP > 0:
			shapeFilePath = self.getArchivePath(cubeCountP, self.shapeDirPath)
			if not shapeFilePath is None:
				#if the shape file for the current cube count exists, start from there
				found = True
				break
			cubeCountP -= 1
		#check if a cube shape file was found
		if found:
			if cubeCountP == cubeCount:
				#if a shapes file was found for the required cube count, warn the user and exit
				print("shape file found for cube count " + str(cubeCount) + " at " + shapeFilePath)
				return
			else:
				#if a shape file was found for a cube count lower than the required, load it into the source shape list
				sourceShapeList = self.loadTextArchive(shapeFilePath)
				sourceShapeCount = len(sourceShapeList)
				#adjust the cube count to correspond to the shape being searched for
				cubeCountP += 1
		else:
			#if a shape file was not found, we start from a one cube shape
			cubeCountP = 1
			sourceShapeList = POINTER(CubeShape)()
			sourceShapeCount = 0
		#find and save all shapes for cube counts starting at the first one with a shape file and ending at the requested count
		sourceList = sourceShapeList
		while cubeCountP <= cubeCount:
			#signal the start of the search for the current cube count
			print("Searching shapes for cube count " + str(cubeCountP))
			#get the descendents shapes for the current source shapes
			time = datetime.now(timezone.utc).timestamp()
			descList = POINTER(CubeShape)()
			if (threadCount > 1) and (callable(getattr(self.CSSL, "getDescendentsMulti", None))):
				#use the multi threaded get descendents function if it is available
				descCount = self.CSSL.getDescendentsMulti(pointer(descList), sourceList, sourceShapeCount, threadCount, 0)
			else:
				#otherwise use the single thread version
				descCount = self.CSSL.getDescendents(pointer(descList), sourceList, sourceShapeCount)
			#clean up the source list, if it was obtained from the shared library
			if sourceList != sourceShapeList:
				self.CSSL.cleanShapeList(sourceList, sourceShapeCount)
			#get the time taken in seconds, and round it
			time = datetime.now(timezone.utc).timestamp() - time
			if time < 1:
				time = round(time, 5)
			elif time < 10:
				time = round(time, 1)
			else:
				time = round(time)
			#create a shape archive for the descendents list
			self.saveArchive(descList, "shapes_" + str(cubeCountP), descCount)
			#set the new source shapes as the current descendents
			sourceList = descList
			sourceShapeCount = descCount
			#warn the user the file was created
			print("Created shape file for cube count " + str(cubeCountP) + " in " + str(time) + " seconds")
			cubeCountP += 1
		#clean up the descendent shape array
		self.CSSL.cleanShapeList(descList, descCount)

	def compareShapeArrays(self, firstShapeArr, secondShapeArr):
		#this function returns the list of shapes present in the second shape array and missing in the first
		firstLen = len(firstShapeArr)
		secondLen = len(secondShapeArr)
		missingShapeArr = (CubeShape * secondLen)() #the maximum number of missing shapes is the number of shapes in the second array
		maxBoxSize = (firstShapeArr[0].width + firstShapeArr[0].height + firstShapeArr[0].depth - 2) ** 3
		for i in range(secondLen):
			missingShapeArr[i].box = (c_char * maxBoxSize)()
		missingLen = self.CSSL.compareShapeLists(firstShapeArr, firstLen, secondShapeArr, secondLen, missingShapeArr)
		#check whether any shapes were missing
		if missingLen == 0:
			#if no shapes were missing return nothing
			return None
		#if there were missing shapes, build a final missing shape array and return it
		missingShapeArrFinal = (CubeShape * missingLen)()
		for i in range(missingLen):
			missingShapeArrFinal[i].value = missingShapeArr[i].value
			missingShapeArrFinal[i].edgeValue = missingShapeArr[i].edgeValue
			missingShapeArrFinal[i].connections = missingShapeArr[i].connections
			missingShapeArrFinal[i].width = missingShapeArr[i].width
			missingShapeArrFinal[i].height = missingShapeArr[i].height
			missingShapeArrFinal[i].depth = missingShapeArr[i].depth
			length = missingShapeArrFinal[i].width * missingShapeArrFinal[i].height * missingShapeArrFinal[i].depth
			missingShapeArrFinal[i].box = (c_char * length)()
			for j in range(length):
				missingShapeArrFinal[i].box[j] = missingShapeArr[i].box[j]
		return missingShapeArrFinal

	def getShapeDescendents(self, shape):
		#this function returns an array containing the descendents of the given shape
		descArr = POINTER(CubeShape)()
		descCount = self.CSSL.getDescendents(pointer(descArr), pointer(shape), 1)
		#check if the shape has any descendents
		if descCount == 0:
			return None
		#create a new descendent array
		descArrFinal = (CubeShape * descCount)()
		for i in range(descCount):
			descArrFinal[i].value = descArr[i].value
			descArrFinal[i].edgeValue = descArr[i].edgeValue
			descArrFinal[i].connections = descArr[i].connections
			descArrFinal[i].width = descArr[i].width
			descArrFinal[i].height = descArr[i].height
			descArrFinal[i].depth = descArr[i].depth
			length = descArrFinal[i].width * descArrFinal[i].height * descArrFinal[i].depth
			descArrFinal[i].box = (c_char * length)()
			for j in range(length):
				descArrFinal[i].box[j] = descArr[i].box[j]
		#clean up the descendent array
		self.CSSL.cleanShapeList(descArr, descCount)
		return descArrFinal

	def checkDistinct(self, firstShape, secondShape):
		#this function checks whether the fist shape is different from the second shape
		disVal = self.CSSL.checkDistinct(pointer(firstShape), pointer(secondShape))
		return (disVal == 1)

	def loadArchive(self, cubeCount, asCArray = True):
		#this function tries to find the text archive for the specified cube count,and returns a cube shape array with its data if found
		shapeFilePath = self.getArchivePath(cubeCount, self.shapeDirPath)
		if shapeFilePath is None:
			print("No archive file found for cube count " + str(cubeCount))
			return None
		return self.loadTextArchive(shapeFilePath, asCArray)

	def saveArchive(self, shapeList, fileName, shapeCount = 0):
		#this function saves the shape list in the archive folder, under the given file name
		shapeFilePath = os.path.join(self.shapeDirPath, fileName + ".txt")
		self.saveTextArchive(shapeList, shapeFilePath, shapeCount)

	def getArchivePath(cubeCount, shapeDirPath = None):
		#this function attempts to find the archive file path for the given cube count
		if shapeDirPath is None:
			shapeDirPath = os.path.join(pathlib.Path(__file__).parent.absolute(), "shapes/")
		#build the shape file path
		shapeFilePath = os.path.join(shapeDirPath, "shapes_" + str(cubeCount) + ".txt")
		if os.path.isfile(shapeFilePath):
			#return it if the file exists
			return shapeFilePath
		else:
			#otherwise return nothing
			return None

	def shapeArrayToList(shapeArray):
		#this function converts the provided shape array into a list
		shapeList = []
		for i in range(len(shapeArray)):
			shape = [0, 0, 0, 0, 0, 0, []]
			shape[0] = shapeArray[i].value
			shape[1] = shapeArray[i].edgeValue
			shape[2] = shapeArray[i].connections
			shape[3] = shapeArray[i].width
			shape[4] = shapeArray[i].height
			shape[5] = shapeArray[i].depth
			size = shapeArray[i].width * shapeArray[i].height * shapeArray[i].depth
			for j in range(size):
				shape[6].append(int.from_bytes(shapeArray[i].box[j], "little"))
			shapeList.append(shape)
		return shapeList

	def loadTextArchive(shapeFilePath, asCArray = True):
		#this function attempts to load the archive file at the specified path and returns a cube shape array if successful
		if not os.path.isfile(shapeFilePath):
			#if the shape file does not exist, exit immediately
			return None
		shapeList = []
		with open(shapeFilePath, "r") as file:
			lines = list(file)
			if len(lines[0]) > 0:
				for line in lines:
					#each line of the shape file represents a shape object
					text = line.strip().split(";") #split the line by semicolon
					if len(text) != 7:
						#if the line is not split in seven, this is an invalid file, exit
						return None
					shape = [0, 0, 0, 0, 0, 0, []]
					shape[0] = int(text[0]) #the first element of the line is the shape value
					shape[1] = int(text[1]) #the second element of the line is the shape edge value
					shape[2] = int(text[2]) #the third element of the line is the shape connections
					shape[3] = int(text[3]) #the fourth element is the width
					shape[4] = int(text[4]) #the fifth element is the height
					shape[5] = int(text[5]) #the sixth element is the depth
					#the fifth element is the actual representation of the shape, as a string of ones and zeros
					for c in text[6]:
						shape[6].append(int(c))
					shapeList.append(shape)
		#once we have loaded all the source shapes, either create an array of cube shapes structures and load them with the data or return the list
		if asCArray:
			sourceShapeCount = len(shapeList)
			sourceShapeArr = (CubeShape * sourceShapeCount)()
			for i in range(sourceShapeCount):
				sourceShapeArr[i].value = shapeList[i][0]
				sourceShapeArr[i].edgeValue = shapeList[i][1]
				sourceShapeArr[i].connections = shapeList[i][2]
				sourceShapeArr[i].width = shapeList[i][3]
				sourceShapeArr[i].height = shapeList[i][4]
				sourceShapeArr[i].depth = shapeList[i][5]
				length = len(shapeList[i][6])
				sourceShapeArr[i].box = (c_char * length)()
				shapeByte = bytes(shapeList[i][6])
				for j in range(length):
					sourceShapeArr[i].box[j] = shapeByte[j]
			return sourceShapeArr
		else:
			return shapeList

	def saveTextArchive(shapeList, shapeFilePath, shapeCount = 0):
		#this function creates a shape archive file at the specified path and copies the shape list information into it
		if (shapeCount == 0): shapeCount = len(shapeList)
		with open(shapeFilePath, "w") as file:
			for i in range(shapeCount):
				count = shapeList[i].width * shapeList[i].height * shapeList[i].depth
				shapeStr = ""
				for j in range(count):
					shapeStr += str(int.from_bytes(shapeList[i].box[j], "little"))
				file.write(str(shapeList[i].value) + ";" + str(shapeList[i].edgeValue) + ";" + str(shapeList[i].connections) + ";" + str(shapeList[i].width) + ";" + str(shapeList[i].height) + ";" + str(shapeList[i].depth) + ";" + shapeStr + "\n")

	def extract(shapeList, value = 0, width = 0, height = 0, depth = 0):
		#this function attempts to crate a list of shapes in the supplied shape list that match the specified characteristics
		matchValue = (value > 0)
		matchWidth = (width > 0)
		matchHeight = (height > 0)
		matchDepth = (depth > 0)
		#check that there is at least one extraction criterion
		if not (matchValue or matchWidth or matchHeight or matchDepth):
			print("Specify at least one extraction criterion")
			return None
		#go through the shape list and retrieve all shapes that match the criteria
		matchList = []
		shapeCount = len(shapeList)
		for i in range(shapeCount):
			if not ((matchValue and (shapeList[i].value != value)) or (matchWidth and (shapeList[i].width != width)) or (matchHeight and (shapeList[i].height != height)) or (matchDepth and (shapeList[i].depth != depth))):
				#if the shape does not differ from any criterion, add it to the match list
				matchList.append(shapeList[i])
		#check that there is at least one match
		matchCount = len(matchList)
		if matchCount == 0:
			print("There is no shape matching your criteria in this list")
			return None
		#build the extracted shape list and return it
		extractArr = (CubeShape * matchCount)()
		for i in range(matchCount):
			extractArr[i].value = matchList[i].value
			extractArr[i].edgeValue = matchList[i].edgeValue
			extractArr[i].connections = matchList[i].connections
			extractArr[i].width = matchList[i].width
			extractArr[i].height = matchList[i].height
			extractArr[i].depth = matchList[i].depth
			length = extractArr[i].width * extractArr[i].height * extractArr[i].depth
			extractArr[i].box = (c_char * length)()
			for j in range(length):
				extractArr[i].box[j] = matchList[i].box[j]
		return extractArr

if __name__ == '__main__':
	if (len(sys.argv) != 2) or (not sys.argv[1].isnumeric()):
		print("You must give as an argument the cube count of the shapes you want to find.")
	else:
		sf = ShapeFinder()
		sf.find(int(sys.argv[1]))