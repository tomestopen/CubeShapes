import os
import pathlib
import time
from CSAdapt import CubeShape, CubeShapesSL
from ctypes import POINTER, pointer, c_char, cast
from datetime import datetime, timezone

class ShapeFinder:
	
	def __init__(self):
		#get the cube shapes library
		self.CSSL = CubeShapesSL()
		#find current directory path
		sourcePath = pathlib.Path(__file__).parent.absolute()
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
			shapeFilePath = os.path.join(self.shapeDirPath, "shapes_" + str(cubeCountP) + ".txt")
			if os.path.isfile(shapeFilePath):
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
				shapeList = []
				with open(shapeFilePath, "r") as file:
					lines = list(file)
					if len(lines[0]) > 0:
						for line in lines:
							#each line of the shape file represents a shape object
							text = line.strip().split(";") #split the line by semicolon
							shape = [0, 0, 0, 0, []]
							shape[0] = int(text[0]) #the first element of the line is the shape value
							shape[1] = int(text[1]) #the second element is the width
							shape[2] = int(text[2]) #the third element is the height
							shape[3] = int(text[3]) #the fourth element is the depth
							#the fifth element is the actual representation of the shape, as a string of ones and zeros
							for c in text[4]:
								shape[4].append(int(c))
							shapeList.append(shape)
				#once we have loaded all the source shapes, create an array of cube shapes structures and load them with the data
				sourceShapeCount = len(shapeList)
				sourceShapeList = (CubeShape * sourceShapeCount)()
				for i in range(sourceShapeCount):
					sourceShapeList[i].value = shapeList[i][0]
					sourceShapeList[i].width = shapeList[i][1]
					sourceShapeList[i].height = shapeList[i][2]
					sourceShapeList[i].depth = shapeList[i][3]
					length = len(shapeList[i][4])
					sourceShapeList[i].shape = (c_char * length)()
					shapeByte = bytes(shapeList[i][4])
					for j in range(length):
						sourceShapeList[i].shape[j] = shapeByte[j]
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
			#get the descendents shapes for the current source shapes
			time = datetime.now(timezone.utc).timestamp()
			descList = POINTER(CubeShape)()
			if (threadCount > 1) and (callable(getattr(self.CSSL, "getDescendentsMulti", None))):
				#use the multi threaded get descendents function if it is available
				descCount = self.CSSL.getDescendentsMulti(pointer(descList), sourceList, sourceShapeCount, threadCount)
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
			#create a file and copy the descendent shapes into it
			shapeFilePath = os.path.join(self.shapeDirPath, "shapes_" + str(cubeCountP) + ".txt")
			with open(shapeFilePath, "w") as file:
				for i in range(descCount):
					count = descList[i].width * descList[i].height * descList[i].depth
					shapeStr = ""
					for j in range(count):
						shapeStr += str(int.from_bytes(descList[i].shape[j], "little"))
					file.write(str(descList[i].value) + ";" + str(descList[i].width) + ";" + str(descList[i].height) + ";" + str(descList[i].depth) + ";" + shapeStr + "\n")
			#set the new source shapes as the current descendents
			sourceList = descList
			sourceShapeCount = descCount
			#warn the user the file was created
			print("Created shape file for cube count " + str(cubeCountP) + " at " + shapeFilePath + " in " + str(time) + " seconds")
			cubeCountP += 1
		#clean up the descendent shape array
		self.CSSL.cleanShapeList(descList, descCount)