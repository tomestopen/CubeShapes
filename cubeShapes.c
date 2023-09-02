#include "main.h"

//definitions
#define MAXDICTLEN 10000000 //the maximum dictionary length is ten million entries
#define MINBATCHSIZE 100 //the minimum batch size for the multi-threaded function
//external variables
static int cornerDir[8][3] = {{1, 1, 1}, {-1, 1, 1}, {1, -1, 1}, {-1, -1, 1}, {1, 1, -1}, {-1, 1, -1}, {1, -1, -1}, {-1, -1, -1}};
static int dimOrder[6][3] = {{0, 1, 2}, {0, 2, 1}, {1, 0, 2}, {1, 2, 0}, {2, 0, 1}, {2, 1, 0}};
static int dimCorner[6][4] = {{0, 3, 5, 6}, {1, 2, 4, 7}, {1, 2, 4, 7}, {0, 3, 5, 6}, {0, 3, 5, 6}, {1, 2, 4, 7}};
static int dimCmpN[] = {0}, dimCmpWH[] = {0, 2}, dimCmpWHD[] = {0, 1, 2, 3, 4, 5}, dimCmpHD[] = {0, 1};
//function declarations
static int GetCubeCount(CubeShape *source);
static int GetShapeDictionarySize(int cubeCount);
static void SetShapeConnections(CubeShape *shape);
static void SetShapeValue(CubeShape *shape);
static int AddUniqueShape(void **shapeDictionary, CubeShape *newShape);
static int ShapeMatch(CubeShape *firstShape, CubeShape *secondShape, int *corner, int *dimCmp, int dimCmpCount);
static CubeShape *ShapeDictionaryToArray(void **shapeDictionary, int dictionarySize, int shapeCount);
static void CleanShapeDictionary(void **shapeDictionary, int dictionarySize);

int GetDescendents(CubeShape **descendents, CubeShape *source, int sourceCount){
	//this function returns all unique cube shapes descended from all the source cubes shapes
	int bwidth, bheight, bdepth, bsize, blx, bly, blz, maxSize;
	int midptW, midptH, midptD, adjW, adjH, adjD, modW, modH, modD, valH, valD, valT, val;
	int dictLen;
	int zMove, dblLine;
	int pos, posC, posS, posF, posL, scPos;
	int frameStart, frameEnd, lineStart, lineEnd;
	int connectCount;
	int dimChanged;
	int posCon[6];
	int cubeCount;
	int dimMov[3];
	int i, j, k, l, m, n;
	char *box;
	int shapeCount;
	CubeShape bufShape;
	void **shapeDict;
	CubeShape *shape;
	//first check that there is a source list, and return the cube shape for one cube if there isn't
	if (!source || !sourceCount){
		shape = (CubeShape *) malloc(sizeof(CubeShape));
		shape->value = shape->edgeValue = 21;
		shape->connections = 0;
		shape->width = 1;
		shape->height = 1;
		shape->depth = 1;
		shape->box = (char *) malloc(1);
		shape->box[0] = 1;
		*descendents = shape;
		return 1;
	}
	//if there is a source list, we take each member and try to find all possible shapes that can be created by adding one cube
	shapeCount = 0;
	//the new cube count is the source + 1
	cubeCount = GetCubeCount(source) + 1;
	//create the shape dictionary and set all entries to 0
	dictLen = GetShapeDictionarySize(cubeCount);
	shapeDict = (void **) malloc(sizeof(void *) * dictLen);
	for (i = 0; i < dictLen; i++){
		shapeDict[i] = 0;
	}
	//get the maximum possible size of the array representing a new shape, and assign an array of that size as the creation box and buffer shape box
	val = (cubeCount > 2)? (cubeCount / 3) + 3 : 64;
	maxSize = val * val * val;
	box = (char *) malloc(maxSize);
	bufShape.box = (char *) malloc(maxSize);
	//go through every source shape and get all their unique descendents
	for (scPos = 0; scPos < sourceCount; scPos++){
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
		//copy the source shape into the creation box, while zeroing all spaces on the edge
		pos = 0;
		posC = 0;
		//null all spaces in the first frame, first line of the second frame, and first space of the second line
		val = zMove + bwidth + 1;
		for (i = 0; i < val; i++){
			box[pos++] = 0;
		}
		//start copying the source shape
		for (i = 0; i < source[scPos].depth; i++){
			for (j = 0; j < source[scPos].height; j++){
				for (k = 0; k < source[scPos].width; k++){
					box[pos++] = source[scPos].box[posC++];
				}
				//on line change, null the last space of the current line and the first space of the next one
				for (k = 0; k < 2; k++){
					box[pos++] = 0;
				}
			}
			//on frame change, null all spaces in the last line of the current frame and the first line of the next one
			for (j = 0; j < dblLine; j++){
				box[pos++] = 0;
			}
		}
		//null the remaining spaces
		val = zMove - bwidth - 1;
		for (i = 0; i < val; i++){
			box[pos++] = 0;
		}
		//calculate the midpoint and distance adjusters for the width, height and depth
		midptW = (bwidth / 2) + (bwidth % 2);
		midptH = (bheight / 2) + (bheight % 2);
		midptD = (bdepth / 2) + (bdepth % 2);
		adjW = source[scPos].width + 6;
		adjH = source[scPos].height + 6;
		adjD = source[scPos].depth + 6;
		modW = bwidth - 1;
		modH = bheight - 1;
		modD = bdepth - 1;
		//now we go through each cube of the creation box and try to find if one of its neighbour is not empty, at which point we create a new shape by adding a cube at the current position
		posS = -1;
		frameStart = 0;
		frameEnd = zMove;
		for (i = 0; i < bdepth; i++){
			lineStart = frameStart;
			lineEnd = lineStart + bwidth;
			//calculate the depth edge value (adjusted distance to the closest edge in the z axis) for cubes on this frame
			valD = ((i < midptD)? i : (modD - i)) * adjD;
			for (j = 0; j < bheight; j++){
				//calculate the height edge value (adjusted distance to the closest edge in the y axis) for cubes on this line
				valH = ((j < midptH)? j : (modH - j)) * adjH;
				for (k = 0; k < bwidth; k++){
					posS++;
					//if there is already a cube at the current position, move on to the next
					if (box[posS]) continue;
					//if there is no cube at the current position check if there is a cube in any of the six neighbouring positions, and record how many connections the new cube would have
					connectCount = ((((posCon[0] = posS - 1) >= lineStart) && (box[posCon[0]])) + (((posCon[1] = posS + 1) < lineEnd) && (box[posCon[1]])) + (((posCon[2] = posS - bwidth) >= frameStart) && (box[posCon[2]])) + (((posCon[3] = posS + bwidth) < frameEnd) && (box[posCon[3]])) + (((posCon[4] = posS - zMove) >= 0) && (box[posCon[4]])) + (((posCon[5] = posS  + zMove) < bsize) && (box[posCon[5]])));
					//if there is at least one connection  (a neighbouring cube) create a new shape
					if (connectCount){
						//if there is, create a new shape
						//*******************
						//shape creation
						//*******************
						//set the connection score for the new shape
						bufShape.connections = source[scPos].connections + connectCount;
						//get the new shape dimensions
						bufShape.width = source[scPos].width + ((k == 0) || (k == blx));
						bufShape.height = source[scPos].height + ((j == 0) || (j == bly));
						bufShape.depth = source[scPos].depth + ((i == 0) || (i == blz));
						//check that any dimension has changed
						dimChanged = ((bufShape.width != source[scPos].width) || (bufShape.height != source[scPos].height) || (bufShape.depth != source[scPos].depth));
						//get the position of the starting frame for copying, depending on the position of the new cube
						if (dimChanged){
							//if any dimension changed, the starting point might have changed
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
							//organise the shape so that the longest dimension is the width, the second longest is the height, and the shortest is depth, and define the dimension moves accordingly
							if (bufShape.height > bufShape.width){
								dimMov[0] = bwidth;
								dimMov[1] = -1;
								dimMov[2] = zMove;
								posF += (bufShape.width - 1);
								val = bufShape.width;
								bufShape.width = bufShape.height;
								bufShape.height = val;
							}
							else if (bufShape.depth > bufShape.width){
								dimMov[0] = zMove;
								dimMov[1] = 1;
								dimMov[2] = bwidth;
								val = bufShape.width;
								bufShape.width = bufShape.depth;
								bufShape.depth = val;
							}
							else if (bufShape.depth > bufShape.height){
								dimMov[0] = -1;
								dimMov[1] = zMove;
								dimMov[2] = bwidth;
								posF += (bufShape.width - 1);
								val = bufShape.height;
								bufShape.height = bufShape.depth;
								bufShape.depth = val;
							}
							else {
								dimMov[0] = 1;
								dimMov[1] = bwidth;
								dimMov[2] = zMove;
							}
						}
						else {
							//if no dimension has changed, the starting point is always one space, one line and one frame from teh start of the creation box
							posF = zMove + bwidth + 1;
							//and the dimension moves are the same as the source
							dimMov[0] = 1;
							dimMov[1] = bwidth;
							dimMov[2] = zMove;
						}
						//now copy the shape from the creation box into the shape object box array, while rotating it so that the width is the longest dimension, heights second longest and depth shortest
						posC = posL = posF;
						pos = 0;
						box[posS] = 1; //add the new cube to the creation box for the copy
						for (l = 0; l < bufShape.depth; l++){
							for (m = 0; m < bufShape.height; m++){
								for (n = 0; n < bufShape.width; n++){
									bufShape.box[pos++] = box[posC];
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
						if (dimChanged){
							//if a dimension changed, we must recalculate all cube edge values, meaning the source shape value is useless
							SetShapeValue(&bufShape);
						}
						else {
							//if no dimension has changed, we only need to calculate the edge value for the new cube and add it to the source edge value to get the new one
							//calculate the width edge value (adjusted distance to the closest edge in the x axis) for this cube and the total edge value by adding up all edges values
							valT = valD + valH + (((k < midptW)? k : (modW - k)) * adjW);
							//the new shape edge value is the same as the source plus the edge value of the new cube
							bufShape.edgeValue = source[scPos].edgeValue + valT;
							//the new shape value is the same as the source plus the edge value and connection count of the new cube
							bufShape.value = source[scPos].value + connectCount + valT;
						}
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
	}
	//clean up the creation box and buffer shape array
	free(box);
	free(bufShape.box);
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
		cubeCount += source->box[i];
	}
	return cubeCount;
}

int GetShapeDictionarySize(int cubeCount){
	//this function returns the size of the dictionary required to store shapes made with the specified cube count
	int maxEdgeValue, count, dictLen;
	//get the maximum possible edge value for a cube
	maxEdgeValue = (cubeCount + 6) * cubeCount; //this is above any possible edge value
	//get the size of the biggest box possible with this cube count
	count = ((cubeCount / 3) + 1); //get the side length of the shape with the biggest possible box, which is three lines perpendicular to each other in each dimension. we add 1 to make sure to be larger
	count = (count > 1)? count * count * count : 8; //cube it (as there are three dimensions) to get the maximum box size (minimum 8)
	//the length of the dictionary is determined by the largest possible shape value, function of the maximum edge value, maximum connections and maximum dimension difference
	dictLen = ((maxEdgeValue + 6) * count * 3) + (cubeCount * 1600);
	dictLen = (dictLen > MAXDICTLEN)? MAXDICTLEN : dictLen; //limit the size of the dictionary to the predefined maximum
	return dictLen;
}

void SetShapeConnections(CubeShape *shape){
	//this function sets the connections value for the given shape
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
				if (shape->box[pos]){
					//if there is , add the sum of connections to all surrounding cubes to the connection value
					connectValue += ((((posCon[0] = pos - 1) >= lineStart) && (shape->box[posCon[0]])) + (((posCon[1] = pos + 1) < lineEnd) && (shape->box[posCon[1]])) + (((posCon[2] = pos - shape->width) >= frameStart) && (shape->box[posCon[2]])) + (((posCon[3] = pos + shape->width) < frameEnd) && (shape->box[posCon[3]])) + (((posCon[4] = pos - zMove) >= 0) && (shape->box[posCon[4]])) + (((posCon[5] = pos  + zMove) < size) && (shape->box[posCon[5]])));
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
	shape->connections = connectValue / 2; // as a connection always involve two cubes, divide by two to get the number of unique connections
}

void SetShapeValue(CubeShape *shape){
	//this function calculates ans sets the shape value for the supplied shape
	int midptW, midptH, midptD, adjW, adjH, adjD, valH, valD;
	int pos;
	int i, j, k;
	//calculate the midpoint and distance adjusters for the width, height and depth
	midptW = (shape->width / 2) + (shape->width % 2);
	midptH = (shape->height / 2) + (shape->height % 2);
	midptD = (shape->depth / 2) + (shape->depth % 2);
	adjW = shape->width + 6;
	adjH = shape->height + 6;
	adjD = shape->depth + 6;
	//if the shape connections is 0, calculate them
	if (!shape->connections)
		SetShapeConnections(shape);
	//the cube edge values are calculated by taking the adjusted distance to the closest edge and squaring it. we do this for all three dimensions
	pos = 0;
	shape->edgeValue = 0;
	for (i = 0; i < shape->depth; i++){
		//calculate the depth edge value (adjusted distance to the closest edge in the z axis) for cubes on this frame
		valD = ((i < midptD)? (i + 1) : (shape->depth - i)) * adjD;
		for (j = 0; j < shape->height; j++){
			//calculate the height edge value (adjusted distance to the closest edge in the y axis) for cubes on this line
			valH = ((j < midptH)? (j + 1) : (shape->height - j)) * adjH;
			for (k = 0; k < shape->width; k++){
				//check that there is a cube present
				if (shape->box[pos++]){
					//if there is, calculate the width edge value (adjusted distance to the closest edge in the x axis) and add it and the other edge values top the shape edge value total
					shape->edgeValue += ((((k < midptW)? (k + 1) : (shape->width - k)) * adjW) + valH + valD);
				}
			}
		}
	}
	//the shape value is a combination of its connection value, the weighted difference between its dimensions and the cube edge value total
	shape->value = shape->connections + shape->edgeValue + (shape->width - shape->height) * 1000 + (shape->width - shape->depth) * 600;
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
			shape->edgeValue = newShape->edgeValue;
			shape->connections = newShape->connections;
			shape->width = newShape->width;
			shape->height = newShape->height;
			shape->depth = newShape->depth;
			count = newShape->width * newShape->height * newShape->depth;
			shape->box = (char *) malloc(count);
			for (i = 0; i < count; i++){
				shape->box[i] = newShape->box[i];
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
		shape->edgeValue = newShape->edgeValue;
		shape->connections = newShape->connections;
		shape->width = newShape->width;
		shape->height = newShape->height;
		shape->depth = newShape->depth;
		count = newShape->width * newShape->height * newShape->depth;
		shape->box = (char *) malloc(count);
		for (i = 0; i < count; i++){
			shape->box[i] = newShape->box[i];
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
	//check that the first an second shape have the same characteristics
	if ((firstShape->value == secondShape->value) && (firstShape->edgeValue == secondShape->edgeValue) && (firstShape->connections == secondShape->connections) && (firstShape->width == secondShape->width) && (firstShape->height == secondShape->height) && (firstShape->depth == secondShape->depth)){
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
							if (firstShape->box[posC] ^ secondShape->box[pos++]){
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
				shapeArr[pos].edgeValue = shapeC->edgeValue;
				shapeArr[pos].connections = shapeC->connections;
				shapeArr[pos].width = shapeC->width;
				shapeArr[pos].height = shapeC->height;
				shapeArr[pos].depth = shapeC->depth;
				shapeArr[pos].box = shapeC->box;
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
		free((void *) shapeList[i].box);
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
	int dictLen, missingCount, count;
	void **shapeDict;
	int i, j;
	//create the shape dictionary and set all entries to 0
	dictLen = GetShapeDictionarySize(GetCubeCount(sourceList));
	shapeDict = (void **) malloc(sizeof(void *) * dictLen);
	for (i = 0; i < dictLen; i++){
		shapeDict[i] = 0;
	}
	//add all source list entries
	for (i = 0; i < sourceLength; i++){
		if (!sourceList[i].value) SetShapeValue(&sourceList[i]); //calculate the shape value if it hasn't one
		AddUniqueShape(shapeDict, &sourceList[i]);
	}
	//go through the target list and try to add each shape to the dictionary
	missingCount = 0;
	for (i = 0; i < targetLength; i++){
		if (!targetList[i].value) SetShapeValue(&targetList[i]); //calculate the shape value if it hasn't one
		if (AddUniqueShape(shapeDict, &targetList[i])){
			//if the shape was successfully added to the dictionary, it wasn't present in the source list, copy it to the missing list
			missingList[missingCount].value = targetList[i].value;
			missingList[missingCount].edgeValue = targetList[i].edgeValue;
			missingList[missingCount].connections = targetList[i].connections;
			missingList[missingCount].width = targetList[i].width;
			missingList[missingCount].height = targetList[i].height;
			missingList[missingCount].depth = targetList[i].depth;
			count = missingList[missingCount].width * missingList[missingCount].height * missingList[missingCount].depth;
			for (j = 0; j < count; j++){
				missingList[missingCount].box[j] = targetList[i].box[j];
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
	//set the shape value for each shape
	for (int i = 0; i < listLength; i++){
		SetShapeValue(&shapeList[i]);
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

int GetDescendentsMulti(CubeShape **descendents, CubeShape *source, int sourceCount, int maxThreads, int batchSize){
	//this function returns the descendents of the source shapes, splitting the workload between multiple threads
	TP_CALLBACK_ENVIRON callBackEnv;
	PTP_POOL pool;
	PTP_WORK *workArr;
	PTP_CLEANUP_GROUP cleanupgroup;
	int cleanStep;
	int workCount, hasPartialBatch, workDone;
	CubeShape *shapeArr;
	WorkData *workDataArr;
	int dictLen, maxBatchSize;
	void **shapeDict;
	int shapeCount;
	int i, j;
	//check that the source has shape count greater than the minimum batch size
	if (sourceCount <= MINBATCHSIZE){
		//if does not, simply return the results from the single thread function
		return GetDescendents(descendents, source, sourceCount);
	}
	//if there are enough source shapes, use multiple threads
	pool = 0;
	cleanStep = 0;
	shapeCount = 0;
	//check that the batch size is sensible
	maxBatchSize = sourceCount / maxThreads;
	if (maxBatchSize < MINBATCHSIZE) maxBatchSize = MINBATCHSIZE;
	if (batchSize < 1){
		//if the batch size is zero or less, divide the work into equal parts for each thread
		batchSize = maxBatchSize;
	}
	else if (batchSize > maxBatchSize){
		//if the batch size is too big with respect to the number of threads, set it to the maximum allowed
		batchSize = maxBatchSize;
	}
	//get the number of work items and initialize the work arrays
	hasPartialBatch = ((sourceCount % batchSize) > 0);
	workCount = sourceCount / batchSize;
	workDataArr = (WorkData *) malloc(sizeof(WorkData) * workCount);
	//add the work data for all work groups
	shapeArr = source;
	for (i = 0; i < workCount; i++){
		workDataArr[i].sourceCount = batchSize; //the source count is always the batch size except for the last work batch
		workDataArr[i].source = shapeArr;
		workDataArr[i].descCount = -1;
		shapeArr += batchSize;
	}
	if (hasPartialBatch)
		workDataArr[workCount - 1].sourceCount += (sourceCount % batchSize); //if the there is a partial work batch, add it to the last one
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
			free((void *) shapeArr[j].box);
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