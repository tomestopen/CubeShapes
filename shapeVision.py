import os
import pathlib
from numpy import empty
from matplotlib import pyplot

class ShapeVision:
	def __init__(self):
		#find current directory path
		sourcePath = pathlib.Path(__file__).parent.absolute()
		#check that the shapes directory exists
		self.shapeDirPath = os.path.join(sourcePath, "shapes/")
		self.isValid = os.path.isdir(self.shapeDirPath)
		if not self.isValid:
			print("shape folder not present. impossible to find shapes")

	def view(self, cubeCount):
		#this function is used to view cube shapes for the specified cube count
		if not self.isValid:
			print("shape folder not present. impossible to find shapes")
			return
		#try to find the shape file for the specified cube count
		shapeFilePath = os.path.join(self.shapeDirPath, "shapes_" + str(cubeCount) + ".txt")
		if not os.path.isfile(shapeFilePath):
			print("No shape file for the specified cube count")
			return
		#if a shape file was found load it into a list
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
		shapeCount = len(shapeList)
		#inform the user of the number of shapes and ask for instructions
		infoStr = "-------------------------------------\n"
		infoStr += "There " + ("are " if shapeCount > 1 else "is ") + str(shapeCount) + " shapes with a cube count of " + str(cubeCount) + "\n"
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
				axes = [shapeList[index][3], shapeList[index][2], shapeList[index][1]]
				data = empty(axes, dtype = bool)
				pos = 0
				for j in range(shapeList[index][3]):
					for k in range(shapeList[index][2]):
						for l in range(shapeList[index][1]):
							data[j][k][l] = (shapeList[index][4][pos] == 1)
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