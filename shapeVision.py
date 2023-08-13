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
		infoStr = "There " + ("are " if shapeCount > 1 else "is ") + str(shapeCount) + " shapes with a cube count of " + str(cubeCount) + "\n"
		infoStr += "Type the number of the shape you want.\n"
		infoStr += "You can request a series of shapes by typing two numbers separated by a space.\n"
		infoStr += "A maximum of 30 shapes can be displayed at a time.\n"
		infoStr += "You can quit by typing 'quit' or 'q'\n"
		while True:
			#print the instructions and wait for a response
			print(infoStr)
			inpStr = input().strip().lower()
			#check if the user wants to quit
			if (inpStr == "quit") or (inpStr == "q"):
				break
			inpArr = inpStr.split()
			#check that the instructions are valid
			isRange = (len(inpArr) == 2)
			if (len(inpArr) > 2) or (not inpArr[0].isnumeric()) or (isRange and not inpArr[1].isnumeric()):
				print("Invalid Command\n")
				continue
			#check that the numbers are valid indexes
			start = int(inpArr[0])
			stop = int(inpArr[1]) if isRange else start
			if (start < 1) or (start > shapeCount) or (stop < 1) or (stop > shapeCount):
				print("Number outside the range of possible shapes\n")
				continue
			#check there are no more than thirty shapes
			if start - stop + 1 > 30:
				print("Too many shapes requests\n")
				continue
			#rearrange the indexes if need be
			if start > stop:
				buf = start
				start = stop
				stop = buf
			#print out the range of plotted shapes
			infStr = "Displaying shape"
			if isRange:
				infStr += "s " + str(start) + " to " + str(stop)
			else:
				infStr += " " + str(start)
			infStr += "\n"
			print(infStr)
			#create a plot for all requested shapes
			start -= 1
			figList = []
			for i in range(start, stop):
				#create the data and populate it with the specified shape
				axes = [shapeList[i][3], shapeList[i][2], shapeList[i][1]]
				data = empty(axes, dtype = bool)
				pos = 0
				for j in range(shapeList[i][3]):
					for k in range(shapeList[i][2]):
						for l in range(shapeList[i][1]):
							data[j][k][l] = (shapeList[i][4][pos] == 1)
							pos += 1
				data = data.transpose()
				#build and show the plot
				shapePlt = pyplot.figure().add_subplot(projection = "3d")
				shapePlt.voxels(data)
				shapePlt.set_aspect("equal", adjustable = "box")
				shapePlt.view_init(100, 0)
				figList.append(shapePlt)
			#show all the created plots
			pyplot.show()
			#clean up figures
			for fig in figList:
				fig.clear()