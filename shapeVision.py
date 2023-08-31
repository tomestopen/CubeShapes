import os
import pathlib
import numpy
import sys
from CSAdapt import CubeShape, CubeShapesSL
from ctypes import c_char
from shapeFinder import ShapeFinder
from matplotlib import pyplot

class ShapeVision:
	def __init__(self):
		#get the cube shapes library
		self.CSSL = CubeShapesSL()
		#add class function shortcuts
		self.view = ShapeVision.view
		#find current directory path
		sourcePath = pathlib.Path(__file__).parent.absolute()
		#check that the shapes directory exists
		self.shapeDirPath = os.path.join(sourcePath, "shapes/")
		self.isValid = os.path.isdir(self.shapeDirPath)
		if not self.isValid:
			print("shape folder not present. impossible to find shapes")

	def viewArchive(self, cubeCount):
		#this function is used to view cube shapes from the archive of the specified cube count
		if not self.isValid:
			print("shape folder not present. impossible to find shapes")
			return
		#try to find the shape file for the specified cube count
		shapeFilePath = ShapeFinder.getArchivePath(cubeCount, self.shapeDirPath)
		if shapeFilePath is None:
			print("No shape file for the specified cube count")
			return
		#load the archive into a shape list
		shapeList = ShapeFinder.loadTextArchive(shapeFilePath, False)
		#launch the view function
		self.view(shapeList)

	def loadNumpyArchive(self, archivePath, asCArray = True):
		#this function loads a numpy file and converts it into a shape list or array
		if not os.path.isfile(archivePath):
			#if the numpy file does not exist, exit immediately
			return None
		#otherwise load the file
		npShapeData = numpy.load(archivePath, allow_pickle = True)
		#get the number of shapes in the archive
		shapeCount = len(npShapeData)
		#create a cube shape array and load it with the numpy data
		shapeArr = (CubeShape * shapeCount)()
		#go through the data and load each data point into a cube shape
		for i in range(shapeCount):
			#if necessary rearrange the axes so that the width (deepest level of the array) is bigger than the height (mid level of the array) and the height is bigger than the depth (top level)
			checkSwap = True
			while checkSwap:
				checkSwap = False
				if len(npShapeData[i]) > len(npShapeData[i][0]):
					npShapeData[i] = numpy.swapaxes(npShapeData[i],0,1)
					checkSwap = True
				if len(npShapeData[i][0]) > len(npShapeData[i][0][0]):
					npShapeData[i] = numpy.swapaxes(npShapeData[i],1,2)
					checkSwap = True
			#copy the data into its corresponding shape array
			shapeArr[i].depth = len(npShapeData[i])
			shapeArr[i].height = len(npShapeData[i][0])
			shapeArr[i].width = len(npShapeData[i][0][0])
			size = shapeArr[i].width * shapeArr[i].height * shapeArr[i].depth
			shapeArr[i].box = (c_char * size)()
			pos = 0
			for j in range(shapeArr[i].depth):
				for k in range(shapeArr[i].height):
					for l in range(shapeArr[i].width):
						shapeArr[i].box[pos] = int(npShapeData[i][j][k][l])
						pos += 1
		#set the shape values
		self.CSSL.setShapeListValues(shapeArr, shapeCount)
		#return either the array or a list
		if asCArray:
			return shapeArr
		else:
			return ShapeFinder.shapeArrayToList(shapeArr)

	def view(shapeList):
		#this function is used to view cube shapes for the specified cube count
		shapeCount = len(shapeList)
		#inform the user of the number of shapes and ask for instructions
		infoStr = "-------------------------------------\n"
		infoStr += "There " + ("are " if shapeCount > 1 else "is ") + str(shapeCount) + " shapes\n"
		infoStr += "-------------------------------------\n"
		infoStr += "Type the number of the shape you want.\n"
		infoStr += "You can request several shapes by typing their numbers separated by spaces.\n"
		infoStr += "You can request a range of shapes by typing two numbers separated by a minus sign.\n"
		infoStr += "A maximum of 30 shapes can be displayed at a time.\n"
		infoStr += "-------------------------------------\n"
		infoStr += "You can quit by typing 'quit' or 'q'\n"
		infoStr += "-------------------------------------\n"
		outBoundStr = "Index out of range. Valid shapes indexes are 1 to " + str(shapeCount) + "\n"
		invalidStr = "Invalid Command\n"
		tooManyStr = "Too many shapes requests\n"
		while True:
			#print the instructions and wait for a response
			print(infoStr)
			inpStr = input().strip().lower()
			#check if the user wants to quit
			if (inpStr == "quit") or (inpStr == "q"):
				break
			#check that the request was a series
			inpArr = inpStr.split()
			isSeries = (len(inpArr) > 1)
			if isSeries:
				#if it is, get the list of shape indexes requested
				indexList = []
				valid = True
				for inp in inpArr:
					#check that this input is numeric
					if (inp.isnumeric()):
						#if the input is numeric, check that it is in the correct range
						index = int(inp)
						if (index < 1) or (index > shapeCount):
							#if it is not, exit
							print(outBoundStr)
							valid = False
							break
						else:
							#otherwise add it to the index list
							indexList.append(index - 1)
					else:
						#if the input is not numeric, this is an invalid command
						print(invalidStr)
						valid = False
						break
				#check there are no more than thirty shapes
				if len(indexList) > 30:
					print(tooManyStr)
					continue
				#if the command was invalid return to the prompt
				if not valid:
					continue
			#check that the request was a range
			inpArr = inpStr.split("-")
			isRange = (not isSeries) and (len(inpArr) > 1)
			if isRange:
				#if it is, get the range of shape indexes requested
				if (len(inpArr) > 2) or not (inpArr[0].isnumeric() and inpArr[1].isnumeric()):
					# a range consist of two numbers, return to the prompt if it isn't
					print(invalidStr)
					continue
				#check that the indexes are in the correct range
				start = int(inpArr[0])
				stop = int(inpArr[1])
				if (start < 1) or (start > shapeCount) or (stop < 1) or (stop > shapeCount):
					#return to the prompt if they are not
					print(outBoundStr)
					continue
				#rearrange the indexes if need be
				if start > stop:
					buf = start
					start = stop
					stop = buf
				#check there are no more than thirty shapes
				if start - stop + 1 > 30:
					print(tooManyStr)
					continue
				#build the index list if all is good
				indexList = list(range(start - 1, stop))
			#if the instruction was neither a series or a range, check that it is a valid single
			if not (isSeries or isRange):
				if not inpStr.isnumeric():
					print(invalidStr)
					continue
				index = int(inpStr)
				if (index < 1) or (index > shapeCount):
					print(outBoundStr)
					continue
				indexList = [index - 1]
			#create a plot for all requested shapes
			figList = []
			for index in indexList:
				#create the data and populate it with the specified shape
				axes = [shapeList[index][5], shapeList[index][4], shapeList[index][3]]
				data = numpy.empty(axes, dtype = bool)
				pos = 0
				for j in range(shapeList[index][5]):
					for k in range(shapeList[index][4]):
						for l in range(shapeList[index][3]):
							data[j][k][l] = (shapeList[index][6][pos] == 1)
							pos += 1
				data = data.transpose()
				#build and show the plot
				fig = pyplot.figure()
				fig.canvas.manager.set_window_title("Shape " + str(index + 1))
				shapePlt = fig.add_subplot(projection = "3d")
				shapePlt.voxels(data)
				shapePlt.set_aspect("equal", adjustable = "box")
				shapePlt.view_init(100, 0)
				figList.append(shapePlt)
			#show all the created plots
			pyplot.show()
			#clean up figures
			for fig in figList:
				fig.clear()

if __name__ == '__main__':
	if (len(sys.argv) != 2) or (not sys.argv[1].isnumeric()):
		print("You must give as an argument the cube count of the shapes you want to see.")
	else:
		sv = ShapeVision()
		sv.viewArchive(int(sys.argv[1]))