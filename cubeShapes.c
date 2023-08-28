#include "main.h"

//definitions
#define MAXDICTLEN 10000000 //the maximum dictionary length is ten million entries
//external variables
static int cornerDir[8][3] = {{1, 1, 1}, {-1, 1, 1}, {1, -1, 1}, {-1, -1, 1}, {1, 1, -1}, {-1, 1, -1}, {1, -1, -1}, {-1, -1, -1}};
static int dimOrder[6][3] = {{0, 1, 2}, {0, 2, 1}, {1, 0, 2}, {1, 2, 0}, {2, 0, 1}, {2, 1, 0}};
static int dimCorner[6][4] = {{0, 3, 5, 6}, {1, 2, 4, 7}, {1, 2, 4, 7}, {0, 3, 5, 6}, {0, 3, 5, 6}, {1, 2, 4, 7}};
static int dimCmpN[] = {0}, dimCmpWH[] = {0, 2}, dimCmpWHD[] = {0, 1, 2, 3, 4, 5}, dimCmpHD[] = {0, 1};
//function declarations
static int GetCubeCount(CubeShape *source);
static int GetShapeDictionarySize(int cubeCount);
static int GetShapeConnectionValue(CubeShape *shape);
static void SetShapeValue(CubeShape *shape, int cubeCount, int connectionValue);
static int AddUniqueShape(void **shapeDictionary, CubeShape *newShape);
static int ShapeMatch(CubeShape *firstShape, CubeShape *secondShape, int *corner, int *dimCmp, int dimCmpCount);
static CubeShape *ShapeDictionaryToArray(void **shapeDictionary, int dictionarySize, int shapeCount);
static void CleanShapeDictionary(void **shapeDictionary, int dictionarySize);

int GetDescendents(CubeShape **descendents, CubeShape *source, int sourceCount){
	//this function returns all unique cube shapes descended from all the source cubes shapes
	CubeShape *descArr;
	//first check that there is a source list, and return the cube shape for one cube if there isn't
	if (!source || !sourceCount){
		descArr = (CubeShape *) malloc(sizeof(CubeShape));
		descArr->value = 7;
		descArr->width = 1;
		descArr->height = 1;
		descArr->depth = 1;
		descArr->shape = (char *) malloc(1);
		descArr->shape[0] = 1;
		*descendents = descArr;
		return 1;
	}
	//if there is a source list, we take each member and try to find all possible shapes that can be created by adding one cube
	int bwidth, bheight, bdepth, bsize, blx, bly, blz;
	int buf;
	int dictLen;
	int zMove, dblLine;
	int pos, posC, posS, posF, posL;
	int frameStart, frameEnd, lineStart, lineEnd;
	int sourceConnectValue, connectValue;
	int posCon[6];
	int cubeCount;
	int dimMov[3];
	int i, j, k, l, m, n;
	char *box;
	int shapeCount = 0;
	CubeShape bufShape;
	void **shapeDict;
	//the new cube count is the source + 1
	cubeCount = GetCubeCount(source) + 1;
	//create the shape dictionary and set all entries to 0
	dictLen = GetShapeDictionarySize(cubeCount);
	shapeDict = (void **) malloc(sizeof(void *) * dictLen);
	for (i = 0; i < dictLen; i++){
		shapeDict[i] = 0;
	}
	//go through every source shape and get all their unique descendents
	for (int scPos = 0; scPos < sourceCount; scPos++){
		//first we need to define the creation box depending on the dimensions of the source shape
		bwidth = source[scPos].width + 2;
		bheight = source[scPos].height + 2;
		bdepth = source[scPos].depth + 2;
		bsize = bwidth * bheight * bdepth;
		blx = bwidth - 1;
		bly = bheight - 1;
		blz = bdepth - 1;
		dblLine = bwidth * 2;
		zMove = bwidth * bheight;
		//build the creation box for the current shape and copy the shape into it
		box = (char *) malloc(bsize);
		for (i = 0; i < bsize; i++){
			box[i] = 0;	//null all cubes in the creation box
		}
		pos = zMove + bwidth + 1; //the starting position for the copy is one frame, one line and one cube from the beginning
		posC = 0;
		for (i = 0; i < source[scPos].depth; i++){
			for (j = 0; j < source[scPos].height; j++){
				for (k = 0; k < source[scPos].width; k++){
					box[pos++] = source[scPos].shape[posC++];
				}
				//on line change, increment the box position by 2 (to skip the last cube of the current line and the first one of the next)
				pos += 2;
			}
			//on frame change, increment the box position by 2 lines (to skip the last line of the current frame and the first one of the next)
			pos += dblLine;
		}
		//calculate the source connection value
		bufShape.width = bwidth;
		bufShape.height = bheight;
		bufShape.depth = bdepth;
		bufShape.shape = box;
		sourceConnectValue = GetShapeConnectionValue(&bufShape);
		//build the shape array for the potential shape
		bufShape.shape = (char *) malloc(bsize);
		//now we go through each cube of the creation box and try to find if one of its neighbour is not empty, at which point we create a new shape by adding a cube at the current position
		posS = -1;
		frameStart = 0;
		frameEnd = zMove;
		for (i = 0; i < bdepth; i++){
			lineStart = frameStart;
			lineEnd = lineStart + bwidth;
			for (j = 0; j < bheight; j++){
				for (k = 0; k < bwidth; k++){
					posS++;
					//if there is already a cube at the current position, move on to the next
					if (box[posS]) continue;
					//if there is no cube at the current position check if there is a cube in any of the six neighbouring positions, and record how many connections the new cube would have
					connectValue = ((((posCon[0] = posS - 1) >= lineStart) && (box[posCon[0]])) + (((posCon[1] = posS + 1) < lineEnd) && (box[posCon[1]])) + (((posCon[2] = posS - bwidth) >= frameStart) && (box[posCon[2]])) + (((posCon[3] = posS + bwidth) < frameEnd) && (box[posCon[3]])) + (((posCon[4] = posS - zMove) >= 0) && (box[posCon[4]])) + (((posCon[5] = posS  + zMove) < bsize) && (box[posCon[5]])));
					//if there is at least one connection  (a neighbouring cube) create a new shape
					if (connectValue){
						//if there is, create a new shape
						//*******************
						//shape creation
						//*******************
						//get the new shape dimensions
						bufShape.width = source[scPos].width + ((k == 0) || (k == blx));
						bufShape.height = source[scPos].height + ((j == 0) || (j == bly));
						bufShape.depth = source[scPos].depth + ((i == 0) || (i == blz));
						//get the position of the starting frame for copying, depending on the position of the new cube
						if (k == 0){ //the new cube is in the first column
							posF = zMove + bwidth;
						}
						else if (j == 0){ //the new cube is in the first line
							posF = zMove + 1;
						}
						else if (i == 0){ //the new cube is in the first frame
							posF = bwidth + 1;
						}
						else { //the new cube is inside the source shape box, or "after" it
							posF = zMove + bwidth + 1;
						}
						//reorganise the shape so that the longest dimension becomes the width, the second longest becomes the height, and the shortest becomes depth, and define the dimension moves accordingly
						if (bufShape.height > bufShape.width){
							dimMov[0] = bwidth;
							dimMov[1] = 1;
							dimMov[2] = zMove;
							buf = bufShape.width;
							bufShape.width = bufShape.height;
							bufShape.height = buf;
						}
						else if (bufShape.depth > bufShape.width){
							dimMov[0] = zMove;
							dimMov[1] = 1;
							dimMov[2] = bwidth;
							buf = bufShape.width;
							bufShape.width = bufShape.depth;
							bufShape.depth = buf;
						}
						else if (bufShape.depth > bufShape.height){
							dimMov[0] = 1;
							dimMov[1] = zMove;
							dimMov[2] = bwidth;
							buf = bufShape.height;
							bufShape.height = bufShape.depth;
							bufShape.depth = buf;
						}
						else {
							dimMov[0] = 1;
							dimMov[1] = bwidth;
							dimMov[2] = zMove;
						}
						//now copy the shape from the creation box into the shape object shape array, while rotating it so that the width is the longest dimension, heights second longest and depth shortest
						posC = posL = posF;
						pos = 0;
						box[posS] = 1; //add the new cube to the creation box for the copy
						for (l = 0; l < bufShape.depth; l++){
							for (m = 0; m < bufShape.height; m++){
								for (n = 0; n < bufShape.width; n++){
									bufShape.shape[pos++] = box[posC];
									posC += dimMov[0];
								}
								//get the new "line" position in the creation box
								posL += dimMov[1];
								posC = posL;
							}
							//get the new "frame" position in the creation box
							posF += dimMov[2];
							posC = posL = posF;
						}
						box[posS] = 0; //remove the new cube from the creation box
						//once the new shape is created, calculate its value
						//*******************
						//shape value calculation
						//*******************
						SetShapeValue(&bufShape, cubeCount, sourceConnectValue + connectValue);
						//now that we have the shape value, check if it is unique
						//*******************
						//shape uniqueness check
						//*******************
						//increase the shape count if the new shape was added to the dictionary (and was therefore unique)
						shapeCount += AddUniqueShape(shapeDict, &bufShape);
					}
				}
				//increment line start and end
				lineStart += bwidth;
				lineEnd += bwidth;
			}
			//increment frame start and end
			frameStart += zMove;
			frameEnd += zMove;
		}
		//clean up the creation box and potential shape array
		free(box);
		free(bufShape.shape);
	}
	//return the final shape array and shape count
	*descendents = ShapeDictionaryToArray(shapeDict, dictLen, shapeCount);
	return shapeCount;
}

int GetCubeCount(CubeShape *source){
	//this function get the cube count for the specified cube shape
	int i, boxSize, cubeCount;
	cubeCount = 0;
	boxSize = source->width * source->height * source->depth;
	//go through the entire shape box and count the number of cubes
	for (i = 0; i < boxSize; i++){
		cubeCount += source->shape[i];
	}
	return cubeCount;
}

int GetShapeDictionarySize(int cubeCount){
	//this function returns the size of the dictionary required to store shapes made with the specified cube count
	int shellWeight, count, dictLen;
	shellWeight = cubeCount + 7; //the maximum possible shell weight is 7 (the base shell weight) + the number of cubes in the new shape
	count = ((cubeCount / 3) + 1); //get the side length of the shape with the biggest possible box, which is three lines perpendicular to each other in each dimension. we add 1 to make sure to be larger
	count = (count > 1)? count * count * count : 8; //cube it (as there are three dimensions) to get the maximum box size (minimum 8)
	dictLen = ((shellWeight + 6) * count * 3) + ((cubeCount - 1) * 150); //the length of the dictionary is determined by the largest possible shape value
	dictLen = (dictLen > MAXDICTLEN)? MAXDICTLEN : dictLen; //limit the size of the dictionary to the predefined maximum
	return dictLen;
}

int GetShapeConnectionValue(CubeShape *shape){
	//this function returns the connection value for the given shape
	int posCon[6];
	int connectValue, size, zMove, pos;
	int frameStart, frameEnd, lineStart, lineEnd;
	int i, j, k;
	pos = 0;
	zMove = shape->width * shape->height;
	size = zMove * shape->depth;
	connectValue = 0;
	frameStart = 0;
	frameEnd = zMove;
	for (i = 0; i < shape->depth; i++){
		lineStart = frameStart;
		lineEnd = lineStart + shape->width;
		for (j = 0; j < shape->height; j++){
			for (k = 0; k < shape->width; k++){
				//for each space in the shape box, check if there is a cube present
				if (shape->shape[pos]){
					//if there is , add the sum of connections to all surrounding cubes to the connection value
					connectValue += ((((posCon[0] = pos - 1) >= lineStart) && (shape->shape[posCon[0]])) + (((posCon[1] = pos + 1) < lineEnd) && (shape->shape[posCon[1]])) + (((posCon[2] = pos - shape->width) >= frameStart) && (shape->shape[posCon[2]])) + (((posCon[3] = pos + shape->width) < frameEnd) && (shape->shape[posCon[3]])) + (((posCon[4] = pos - zMove) >= 0) && (shape->shape[posCon[4]])) + (((posCon[5] = pos  + zMove) < size) && (shape->shape[posCon[5]])));
				}
				pos++;
			}
			//increment line start and end
			lineStart += shape->width;
			lineEnd += shape->width;
		}
		//increment frame start and end
		frameStart += zMove;
		frameEnd += zMove;
	}
	connectValue = connectValue / 2; // as a connection always involve two cubes, divide by two to get the number of unique connections
	return connectValue;
}

void SetShapeValue(CubeShape *shape, int cubeCount, int connectionValue){
	//this function calculates ans sets the shape value for the supplied shape
	int dimMov[3], dimLenC[3];
	int pos, posC, posF, posL, vmove;
	int shellWeight, shellCount, shellCountNext;
	int i, j, k, l;
	//get the dimensions and moves of the new shape
	dimMov[0] = 1; //width move
	dimMov[1] = shape->width; //height move
	dimMov[2] = shape->width * shape->height; //depth move
	//if the cube count is 0, calculate it
	if (!cubeCount)
		cubeCount = GetCubeCount(shape);
	//if the connection value is 0, calculate it
	if (!connectionValue)
		connectionValue = GetShapeConnectionValue(shape);
	//the shape value is a combination of its connection value, the weighted difference between its dimensions and the cube shell sum
	shape->value = connectionValue + (shape->width - shape->height) * 100 + (shape->width - shape->depth) * 50;
	//the cube shell sum is the sum by dimension of all cubes in successive shells multiplied by the corresponding shell weight
	for (i = 0; i < 3; i++){
		//get the initial shell dimensions
		dimLenC[0] = shape->width;
		dimLenC[1] = shape->height;
		dimLenC[2] = shape->depth;
		//build successive shells within the box, and check how many cubes are in each shell
		vmove = 1;
		shellCount = cubeCount;
		shellWeight = 7;
		pos = 0;
		while (vmove){
			//check if the dimension length is greater than 2 if it is, subtract two to the dimension length and move the starting position by the dimension move value
			vmove = 0;
			if (dimLenC[i] > 2){
				vmove += dimMov[i];
				dimLenC[i] -= 2;
			}
			//if the dimension length was greater than 2, we can create at least one shell inside the current one
			shellCountNext = 0;
			if (vmove){
				//go through the next shell and make a cube count
				pos += vmove;
				posC = posL = posF = pos;
				for (j = 0; j < dimLenC[2]; j++){
					for (k = 0; k < dimLenC[1]; k++){
						for (l = 0; l < dimLenC[0]; l++){
							shellCountNext += shape->shape[posC];
							posC += dimMov[0];
						}
						//get the new "line" position in the shape box
						posL += dimMov[1];
						posC = posL;
					}
					//get the new "frame" position in the shape box
					posF += dimMov[2];
					posC = posL = posF;
				}
			}
			//increase the shape value by the final cube count in the current shell (the number of cubes in the current shell minus the number of cubes in the next shell) times the shell weight
			shellCount = shellCount - shellCountNext;
			shape->value += (shellCount * shellWeight);
			//increase the shell weight
			shellWeight += 2;
			//update the shell count for the next shell
			shellCount = shellCountNext;
		}
	}
}

int AddUniqueShape(void **shapeDictionary, CubeShape *newShape){
	//this function checks whether the new shape already exists in the provided dictionary, and adds it if it doesn't
	int buf, count, match, adjValue;
	CubeShape *shape;
	void **link, **linkP;
	int corner[8];
	int *dimCmp;
	int i;
	//check that the new shape value is already present in the dictionary
	adjValue = newShape->value % MAXDICTLEN;
	if (shapeDictionary[adjValue]){
		//if there is a match, we must look through all shapes with the same value to determine if they are identical to the new shape
		//calculate the corners of the new shape box
		buf = newShape->width * (newShape->height - 1);
		corner[0] = 0;
		corner[1] = corner[0] + newShape->width - 1; //upper right front corner
		corner[2] = corner[0] + buf; //lower left front corner
		corner[3] = corner[1] + buf; //lower right front corner
		buf = (newShape->width * newShape->height) * (newShape->depth - 1);
		corner[4] = corner[0] + buf; //upper left back corner
		corner[5] = corner[1] + buf; //upper right back corner
		corner[6] = corner[2] + buf; //lower left back corner
		corner[7] = corner[3] + buf; //lower right back corner
		//the checks we must do depend on how many dimensions are of the same length
		if (newShape->width == newShape->height){
			//if the width is equal to the height, the number of comparisons depend on whether the width is also equal to the depth
			if (newShape->width == newShape->depth){
				//if the width is equal to the depth as well, all three dimensions are equal, and we must make a comparison for all dimension orders
				dimCmp = dimCmpWHD;
				count = 6;
			}
			else {
				//if they are not equal, we only need to check the dimension orders where width = width and width = height
				dimCmp = dimCmpWH;
				count = 2;
			}
		}
		else if (newShape->height == newShape->depth){
			//if the width is not equal to the height, but the height is equal to the depth, check the dimension orders where height = height and height = depth
			dimCmp = dimCmpHD;
			count = 2;
		}
		else {
			//otherwise, simply compare them when the dimension order is the same
			dimCmp = dimCmpN;
			count = 1;
		}
		//now compare the new shape to each shape in the linked list found at the dictionary index corresponding to the shape value
		link = (void **) shapeDictionary[adjValue];
		while (link){ //keep going until the end of the linked list
			if ((match = ShapeMatch(newShape, (CubeShape *) link[0], corner, dimCmp, count)))
				break; //exit as soon as we find a match
			//move to the next link in the list
			linkP = link;
			link = (void **) link[1];
		}
		//check whether there was any match
		if (match){
			//if there was a match the shape is not new, and we exit
			return 0;
		}
		else{
			//if the shape did not match any of the previous ones, it is new and we can add it as the last link in the list
			//create a new copy of the shape object
			shape = (CubeShape *) malloc(sizeof(CubeShape));
			shape->value = newShape->value;
			shape->width = newShape->width;
			shape->height = newShape->height;
			shape->depth = newShape->depth;
			count = newShape->width * newShape->height * newShape->depth;
			shape->shape = (char *) malloc(count);
			for (i = 0; i < count; i++){
				shape->shape[i] = newShape->shape[i];
			}
			//create the linked list that will contain the shape pointer
			link = (void **) malloc(sizeof(void *) * 2);
			link[0] = (void *) shape;
			link[1] = 0;
			//add the linked list as the next link in the current list
			linkP[1] = (void *) link;
		}
	}
	else {
		//if there is no match, the shape is unique. add it to the dictionary
		//create a new copy of the shape object
		shape = (CubeShape *) malloc(sizeof(CubeShape));
		shape->value = newShape->value;
		shape->width = newShape->width;
		shape->height = newShape->height;
		shape->depth = newShape->depth;
		count = newShape->width * newShape->height * newShape->depth;
		shape->shape = (char *) malloc(count);
		for (i = 0; i < count; i++){
			shape->shape[i] = newShape->shape[i];
		}
		//create the linked list that will contain the shape pointer
		link = (void **) malloc(sizeof(void *) * 2);
		link[0] = (void *) shape;
		link[1] = 0;
		//add the linked list to the dictionary
		shapeDictionary[adjValue] = (void *) link;
	}
	return 1;
}

int ShapeMatch(CubeShape *firstShape, CubeShape *secondShape, int *corner, int *dimCmp, int dimCmpCount){
	//this function checks that the two provided cube shapes are identical
	int i, j, k, l, m;
	int dimLen[3], dimMov[3], dimLenC[3], dimMovC[3];
	int pos, posC, posF, posL;
	int match;
	//check that the first an second shape have the same value and dimensions
	if ((firstShape->value == secondShape->value) && (firstShape->width == secondShape->width) && (firstShape->height == secondShape->height) && (firstShape->depth == secondShape->depth)){
		//if the dimensions and value are the same, we must check whether the cube positions match exactly
		//get the dimensions and moves of the both shapes (since we know they have the same dimensions)
		dimLen[0] = firstShape->width;
		dimLen[1] = firstShape->height;
		dimLen[2] = firstShape->depth;
		dimMov[0] = 1; //width move
		dimMov[1] = firstShape->width; //height move
		dimMov[2] = firstShape->width * firstShape->height; //depth move
		//there are six possible dimension orders (XYZ, YXZ, XZY, ZYX, YZX, ZXY) and each have four associated corners to start the check
		for (i = 0; i < dimCmpCount; i++){ //for each dimension order in the comparison list
			//define the dimension length for the current dimension order
			dimLenC[0] = dimLen[dimOrder[dimCmp[i]][0]];
			dimLenC[1] = dimLen[dimOrder[dimCmp[i]][1]];
			dimLenC[2] = dimLen[dimOrder[dimCmp[i]][2]];
			for (j = 0; j < 4; j++){ //for each box corner associated with that dimension order
				//define the dimension moves for the current dimension order and box corner
				dimMovC[0] = dimMov[dimOrder[dimCmp[i]][0]] * cornerDir[dimCorner[i][j]][dimOrder[dimCmp[i]][0]];
				dimMovC[1] = dimMov[dimOrder[dimCmp[i]][1]] * cornerDir[dimCorner[i][j]][dimOrder[dimCmp[i]][1]];
				dimMovC[2] = dimMov[dimOrder[dimCmp[i]][2]] * cornerDir[dimCorner[i][j]][dimOrder[dimCmp[i]][2]];
				//go through the new shape in the direction of the current comparison, and the current link shape linearly, and check if they have cubes in matching positions
				pos = 0;
				posC = posL = posF = corner[dimCorner[i][j]];
				match = 1;
				for (k = 0; k < dimLenC[2]; k++){
					for (l = 0; l < dimLenC[1]; l++){
						for (m = 0; m < dimLenC[0]; m++){
							//we compare the two positions with an exclusive or. they differ if the result is 1
							if (firstShape->shape[posC] ^ secondShape->shape[pos++]){
								//if the positions don't match, we, know the shapes are not the same and can stop the comparison in this orientation
								match = 0;
								goto checkMatch;
							}
							posC += dimMovC[0];
						}
						//get the new "line" position in the shape box
						posL += dimMovC[1];
						posC = posL;
					}
					//get the new "frame" position in the shape box
					posF += dimMovC[2];
					posC = posL = posF;
				}
				//check if all the positions matched
				checkMatch: if (match){
					//if all the positions match, the shapes are identical, we can stop the comparisons and return the match signal
					return 1;
				}
			}
		}
	}
	//return the no match signal
	return 0;
}

CubeShape *ShapeDictionaryToArray(void **shapeDictionary, int dictionarySize, int shapeCount){
	//this function return a shape array created from a dictionary, and cleans up the dictionary
	CubeShape *shapeC, *shapeArr;
	void **link, **linkP;
	int i, pos;
	shapeArr = (CubeShape *) malloc(sizeof(CubeShape) * shapeCount);
	pos = 0;
	for (i = 0; i < dictionarySize; i++){
		if (shapeDictionary[i]){
			link = (void **) shapeDictionary[i];
			while (link){
				shapeC = (CubeShape *) link[0];
				shapeArr[pos].value = shapeC->value;
				shapeArr[pos].width = shapeC->width;
				shapeArr[pos].height = shapeC->height;
				shapeArr[pos].depth = shapeC->depth;
				shapeArr[pos].shape = shapeC->shape;
				pos++;
				free((void *) shapeC);
				linkP = link;
				link = (void **) link[1];
				free((void *) linkP);
			}
		}
	}
	free((void *) shapeDictionary);
	return shapeArr;
}

void CleanShapeList(CubeShape *shapeList, int shapeCount){
	//this function is used to clean up the shape arrays in the provided cube shape list, and then to clean it
	for (int i = 0; i < shapeCount; i++){
		free((void *) shapeList[i].shape);
	}
	//clean the shape list
	free((void *) shapeList);
}

void CleanShapeDictionary(void **shapeDictionary, int dictionarySize){
	//this function goes through the provided dictionary and frees up all the links
	void **link, **linkP;
	for (int i = 0; i < dictionarySize; i++){
		if (shapeDictionary[i]){
			link = (void **) shapeDictionary[i];
			while (link){
				free(link[0]);
				linkP = link;
				link = (void **) link[1];
				free((void *) linkP);
			}
		}
	}
	free((void *) shapeDictionary);
}

int CompareShapeLists(CubeShape *sourceList, int sourceLength, CubeShape *targetList, int targetLength, CubeShape *missingList){
	//this function finds all the shapes from the target list that are missing in the source list
	int dictLen, missingCount, cubeCount, count;
	void **shapeDict;
	int i, j;
	//create the shape dictionary and set all entries to 0
	cubeCount = GetCubeCount(sourceList);
	dictLen = GetShapeDictionarySize(cubeCount);
	shapeDict = (void **) malloc(sizeof(void *) * dictLen);
	for (i = 0; i < dictLen; i++){
		shapeDict[i] = 0;
	}
	//add all source list entries
	for (i = 0; i < sourceLength; i++){
		if (!sourceList[i].value) SetShapeValue(&sourceList[i], cubeCount, 0); //calculate the shape value if it hasn't one
		AddUniqueShape(shapeDict, &sourceList[i]);
	}
	//go through the target list and try to add each shape to the dictionary
	missingCount = 0;
	for (i = 0; i < targetLength; i++){
		if (!targetList[i].value) SetShapeValue(&targetList[i], cubeCount, 0); //calculate the shape value if it hasn't one
		if (AddUniqueShape(shapeDict, &targetList[i])){
			//if the shape was successfully added to the dictionary, it wasn't present in the source list, copy it to the missing list
			missingList[missingCount].value = targetList[i].value;
			missingList[missingCount].width = targetList[i].width;
			missingList[missingCount].height = targetList[i].height;
			missingList[missingCount].depth = targetList[i].depth;
			count = missingList[missingCount].width * missingList[missingCount].height * missingList[missingCount].depth;
			for (j = 0; j < count; j++){
				missingList[missingCount].shape[j] = targetList[i].shape[j];
			}
			missingCount++;
		}
	}
	//clean up the shape dictionary
	CleanShapeDictionary(shapeDict, dictLen);
	//return the missing count
	return missingCount;
}

void SetShapeListValues(CubeShape *shapeList, int listLength){
	//this function calculates and sets the shape value for each shape in the shape list
	int cubeCount;
	//get the cube count for the shapes
	cubeCount = GetCubeCount(shapeList);
	//set the shape value for each shape
	for (int i = 0; i < listLength; i++){
		SetShapeValue(&shapeList[i], cubeCount, 0);
	}
}

int CheckDistinct(CubeShape *firstShape, CubeShape *secondShape){
	//this function checks whether the first and second shape are distinct
	int corner[8];
	int *dimCmp;
	int count;
	//check that the characteristics of both shapes are the same
	if (!((firstShape->value == secondShape->value) && (firstShape->width == secondShape->width) && (firstShape->height == secondShape->height) && (firstShape->depth == secondShape->depth)))
		return 1; //if they don't, immediately return that they are distinct
	//otherwise calculate the corners of the first shape box
	count = firstShape->width * (firstShape->height - 1);
	corner[0] = 0;
	corner[1] = corner[0] + firstShape->width - 1; //upper right front corner
	corner[2] = corner[0] + count; //lower left front corner
	corner[3] = corner[1] + count; //lower right front corner
	count = (firstShape->width * firstShape->height) * (firstShape->depth - 1);
	corner[4] = corner[0] + count; //upper left back corner
	corner[5] = corner[1] + count; //upper right back corner
	corner[6] = corner[2] + count; //lower left back corner
	corner[7] = corner[3] + count; //lower right back corner
	//the checks we must do depend on how many dimensions are of the same length
	if (firstShape->width == firstShape->height){
		//if the width is equal to the height, the number of comparisons depend on whether the width is also equal to the depth
		if (firstShape->width == firstShape->depth){
			//if the width is equal to the depth as well, all three dimensions are equal, and we must make a comparison for all dimension orders
			dimCmp = dimCmpWHD;
			count = 6;
		}
		else {
			//if they are not equal, we only need to check the dimension orders where width = width and width = height
			dimCmp = dimCmpWH;
			count = 2;
		}
	}
	else if (firstShape->height == firstShape->depth){
		//if the width is not equal to the height, but the height is equal to the depth, check the dimension orders where height = height and height = depth
		dimCmp = dimCmpHD;
		count = 2;
	}
	else {
		//otherwise, simply compare them when the dimension order is the same
		dimCmp = dimCmpN;
		count = 1;
	}
	//finally return the inverse of the match function
	return !ShapeMatch(firstShape, secondShape, corner, dimCmp, count);
}

#ifdef BUILD_DLL

typedef struct sWorkData {
	CubeShape *descendentArr;
	CubeShape *source;
	int descCount;
	int sourceCount;
} WorkData;

VOID CALLBACK DescendentWorkCallback(PTP_CALLBACK_INSTANCE instance, PVOID parameters, PTP_WORK work){
	//this is the work callback function used by the worker threads
	//retrieve the work parameters
	WorkData *workData = (WorkData *) parameters;
	//launch the get descendents function with the given parameters
	workData->descCount = GetDescendents(&workData->descendentArr, workData->source, workData->sourceCount);
}

int GetDescendentsMulti(CubeShape **descendents, CubeShape *source, int sourceCount, int maxThreads){
	//this function returns the descendents of the source shapes, splitting the workload between multiple threads
	//check that the source has at least ten shapes
	if (sourceCount <= 100){
		//if there are less than a hundred source shapes, simply return the results from the single thread function
		return GetDescendents(descendents, source, sourceCount);
	}
	//if there are more than ten sources, use multiple threads
	TP_CALLBACK_ENVIRON callBackEnv;
	PTP_POOL pool = 0;
	PTP_WORK *workArr;
	PTP_CLEANUP_GROUP cleanupgroup;
	int cleanStep = 0;
	int workCount, hasPartialBatch, workDone;
	CubeShape *shapeArr;
	WorkData *workDataArr;
	int dictLen;
	void **shapeDict;
	int shapeCount = 0;
	int i, j;
	//get the number of work items and initialize the work arrays
	hasPartialBatch = ((sourceCount % 100) > 0);
	workCount = (sourceCount / 100) + hasPartialBatch;
	workDataArr = (WorkData *) malloc(sizeof(WorkData) * workCount);
	//add the work data for all work groups
	shapeArr = source;
	for (i = 0; i < workCount; i++){
		workDataArr[i].sourceCount = 100; //the source count is always 100 except for the last work batch
		workDataArr[i].source = shapeArr;
		workDataArr[i].descCount = -1;
		shapeArr += 100;
	}
	if (hasPartialBatch)
		workDataArr[workCount - 1].sourceCount = (sourceCount % 100); //if the last work batch is partial (not 100 shapes) store the correct source count
	//create the thread pool work array
	workArr = malloc(sizeof(PTP_WORK) * workCount);
	//set up thread pool
	InitializeThreadpoolEnvironment(&callBackEnv);
	if (!(pool = CreateThreadpool(0))) goto poolCleanup;
	cleanStep = 1;
	//assign number of threads
	maxThreads = (maxThreads > 1)? maxThreads : 1;
	SetThreadpoolThreadMaximum(pool, maxThreads);
    if (!SetThreadpoolThreadMinimum(pool, 1)) goto poolCleanup;
	//create pool clean-up group
	if (!(cleanupgroup = CreateThreadpoolCleanupGroup())) goto poolCleanup;
	cleanStep = 2;
	//associate callback environment and clean-up group to thread pool
    SetThreadpoolCallbackPool(&callBackEnv, pool);
    SetThreadpoolCallbackCleanupGroup(&callBackEnv, cleanupgroup, 0);
	//build the thread pool work
	for (i = 0; i < workCount; i++){
		if (!(workArr[i] = CreateThreadpoolWork(DescendentWorkCallback, (void *) &workDataArr[i], &callBackEnv))) goto poolCleanup;
	}
	cleanStep = 3;
	//once all the thread pool work has been successfully created, submit it all
	for (i = 0; i < workCount; i++){
		SubmitThreadpoolWork(workArr[i]);
	}
	//create a dictionary to store the combined shapes
	dictLen = GetShapeDictionarySize(GetCubeCount(source) + 1);
	shapeDict = (void **) malloc(sizeof(void *) * dictLen);
	for (i = 0; i < dictLen; i++){
		shapeDict[i] = 0;
	}
	//wait for all work batches to be completed
	while(1){
		//check that the work is done
		workDone = 1;
		for (i = 0; i < workCount; i++){
			if (workDataArr[i].descCount == -1){
				//if one of the descendent counts is still at -1 (the initial value), at least one of the work batches is not done
				workDone = 0;
				break;
			}
		}
		//if the work is done, we exit
		if (workDone) break;
		//otherwise sleep for 10 milliseconds
		Sleep(10);
	}
	//once all the partial descendents lists have been completed, combine them in the dictionary
	for (i = 0; i < workCount; i++){
		shapeArr = workDataArr[i].descendentArr;
		for (j = 0; j < workDataArr[i].descCount; j++){
			//try to add the shape to the dictionary
			shapeCount += AddUniqueShape(shapeDict, &shapeArr[j]);
			//clean up the shape
			free((void *) shapeArr[j].shape);
		}
		//clean up the shape array
		free((void *) shapeArr);
	}
	//return the final shape array
	*descendents = ShapeDictionaryToArray(shapeDict, dictLen, shapeCount);
	//clean up all the thread pool resources
	poolCleanup:
	switch (cleanStep) {
        case 3:
            //clean up the clean-up group members
            CloseThreadpoolCleanupGroupMembers(cleanupgroup, 0, 0);
        case 2:
            //clean up the clean-up group
            CloseThreadpoolCleanupGroup(cleanupgroup);
        case 1:
            //clean up the thread pool
            CloseThreadpool(pool);
        default:
            break;
    }
	//clean up other function resources
	free((void *) workDataArr);
	free((void *) workArr);
	//return the shape count
	return shapeCount;
}
#endif