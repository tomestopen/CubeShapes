#include "main.h"

int GetDescendents(CubeShape **descendents, CubeShape *source, int sourceCount){
	//this function returns all unique cube shapes descended from all the source cubes shapes
	CubeShape *descArr;
	char *shape;
	//first check that there is a source list, and return the cube shape for one cube if there isn't
	if (!source || !sourceCount){
		descArr = (CubeShape *) malloc(sizeof(CubeShape));
		shape = (char *) malloc(1);
		shape[0] = 1;
		descArr->value = 48;
		descArr->width = 1;
		descArr->height = 1;
		descArr->depth = 1;
		descArr->shape = shape;
		*descendents = descArr;
		return 1;
	}
	//if there is a source list, we take each member and try to find all possible shapes that can be created by adding one cube
	int bwidth, bheight, bdepth, bsize, blx, bly, blz;
	int buf, count, isNew, match;
	int dictLen;
	int zMove, dblLine;
	int pos, posC, posS, posF, posL;
	int dimLen[] = {0, 0, 0}, dimMov[] = {0, 0, 0}, dimLenC[] = {0, 0, 0}, dimMovC[] = {0, 0, 0};
	int corner[] = {0, 0, 0, 0, 0, 0, 0, 0};
	int cornerDir[8][3] = {{1, 1, 1}, {-1, 1, 1}, {1, -1, 1}, {-1, -1, 1}, {1, 1, -1}, {-1, 1, -1}, {1, -1, -1}, {-1, -1, -1}};
	int dimOrder[6][3] = {{0, 1, 2}, {0, 2, 1}, {1, 0, 2}, {1, 2, 0}, {2, 0, 1}, {2, 1, 0}};
	int dimCorner[6][4] = {{0, 3, 5, 6}, {1, 2, 4, 7}, {1, 2, 4, 7}, {0, 3, 4, 6}, {0, 1, 5, 6}, {1, 2, 4, 7}};
	int *dimCmp, dimCmpN[] = {0}, dimCmpWH[] = {0, 2}, dimCmpWHD[] = {0, 1, 2, 3, 4, 5}, dimCmpHD[] = {0, 1};
	int cmpCount;
	int adjValue;
	int i, j, k, l, m, n, o, p;
	char *box;
	int shapeCount = 0;
	CubeShape bufShape;
	CubeShape *shapeC, *shapeAll;
	void **shapeDict, **link, **linkP;
	//get the highest dimension in the source shapes, which is equivalent to the number of cubes
	buf = 0;
	for (i = 0; i < sourceCount; i++){
		if (source[i].width > buf)
			buf = source[i].width;
	}
	//create the shape dictionary and set all entries to 0
	count = (((buf + 1) / 3) + 1); //get the side length of the shape with the biggest possible box, which is three lines perpendicular to each other in each dimension. we add 1 to make sure to be larger
	count = (count > 1)? count * count * count : 8; //cube it (as there are three dimensions) to get the maximum box size (minimum 8)
	count = ((count * (count + 1) / 2) + 1) * 48; //this is the summation formula for the box size, multiplied by the number of dimension configurations (6) times the number of box corners (8)
	dictLen = (count > 10000000)? 10000000 : count; //limit the size of the dictionary to 10 million entries maximum
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
		//build the shape array for the potential shape
		bufShape.shape = (char *) malloc(bsize);
		//now we go through each cube of the creation box and try to find if one of its neighbour is not empty, at which point we create a new shape by adding a cube at the current position
		posS = -1;
		for (i = 0; i < bdepth; i++){
			for (j = 0; j < bheight; j++){
				for (k = 0; k < bwidth; k++){
					posS++;
					//if there is already a cube at the current position, move on to the next
					if (box[posS]) continue;
					//check that there is a cube at any of the six neighbouring positions
					if ((((posC = posS - 1) >= 0) && (box[posC])) || (((posC = posS + 1) < bsize) && (box[posC])) || (((posC = posS - bwidth) >= 0) && (box[posC])) || (((posC = posS + bwidth) < bsize) && (box[posC])) || (((posC = posS - zMove) >= 0) && (box[posC])) || (((posC = posS  + zMove) < bsize) && (box[posC]))){
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
						//get the dimensions and moves of the new shape
						dimLen[0] = bufShape.width;
						dimLen[1] = bufShape.height;
						dimLen[2] = bufShape.depth;
						dimMov[0] = 1; //width move
						dimMov[1] = bufShape.width; //height move
						dimMov[2] = bufShape.width * bufShape.height; //depth move
						//calculate the corners of the new shape box
						buf = bufShape.width * (bufShape.height - 1);
						corner[1] = corner[0] + bufShape.width - 1; //upper right front corner
						corner[2] = corner[0] + buf; //lower left front corner
						corner[3] = corner[1] + buf; //lower right front corner
						buf = dimMov[2] * (bufShape.depth - 1);
						corner[4] = corner[0] + buf; //upper left back corner
						corner[5] = corner[1] + buf; //upper right back corner
						corner[6] = corner[2] + buf; //lower left back corner
						corner[7] = corner[3] + buf; //lower right back corner
						//to calculate the value of a shape, we multiply a position counter by 1 if there is a cube in the position and 0 otherwise, and add it to the shape value.
						bufShape.value = 0;
						for (l = 0; l < 6; l++){ //for each dimension order
							//define the dimension length for the current dimension order
							dimLenC[0] = dimLen[dimOrder[l][0]];
							dimLenC[1] = dimLen[dimOrder[l][1]];
							dimLenC[2] = dimLen[dimOrder[l][2]];
							for (m = 0; m < 8; m++){ //for each box corner
								//define the dimension moves for the current dimension order and box corner
								dimMovC[0] = dimMov[dimOrder[l][0]] * cornerDir[m][dimOrder[l][0]];
								dimMovC[1] = dimMov[dimOrder[l][1]] * cornerDir[m][dimOrder[l][1]];
								dimMovC[2] = dimMov[dimOrder[l][2]] * cornerDir[m][dimOrder[l][2]];
								//now calculate
								count = 1;
								posC = posL = posF = corner[m];
								for (n = 0; n < dimLenC[2]; n++){
									for (o = 0; o < dimLenC[1]; o++){
										for (p = 0; p < dimLenC[0]; p++){
											bufShape.value += (bufShape.shape[posC] * count++);
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
							}
						}
						//now that we have the shape value, check if it is unique
						//*******************
						//shape uniqueness check
						//*******************
						//check in the shape "dictionary" if there is an adjusted value match
						adjValue = bufShape.value % 10000000;
						if (shapeDict[adjValue]){
							//if there is a match, we must look through all shapes with the same value to determine if they are identical to the new shape
							//the checks we must do depend on how many dimensions are of the same length
							if (bufShape.width == bufShape.height){
								//if the width is equal to the height, the number of comparisons depend on whether the width is also equal to the depth
								if (bufShape.width == bufShape.depth){
									//if the width is equal to the depth as well, all three dimensions are equal, and we must make a comparison for all dimension orders
									dimCmp = dimCmpWHD;
									cmpCount = 5;
								}
								else {
									//if they are not equal, we only need to check the dimension orders where width = width and width = height
									dimCmp = dimCmpWH;
									cmpCount = 2;
								}
							}
							else if (bufShape.height == bufShape.depth){
								//if the width is not equal to the height, but the height is equal to the depth, check the dimension orders where height = height and height = depth
								dimCmp = dimCmpHD;
								cmpCount = 2;
							}
							else {
								//otherwise, simply compare them when the dimension order is the same
								dimCmp = dimCmpN;
								cmpCount = 1;
							}
							//now compare the new shape to each shape in the linked list found at the dictionary index corresponding to the shape value
							link = (void **) shapeDict[adjValue];
							isNew = 1;
							while (link){ //keep going until the end of the linked list
								shapeC = (CubeShape *) link[0];
								//compare that the new shape and this shape have the same value and box dimensions
								if ((shapeC->value == bufShape.value) && (shapeC->width == bufShape.width) && (shapeC->height == bufShape.height) && (shapeC->depth == bufShape.depth)){
									//if the dimensions and value are the same, we must check whether the cube positions match exactly
									//there are six possible dimension orders (XYZ, YXZ, XZY, ZYX, YZX, ZXY) and each have four associated corners to start the check
									for (l = 0; l < cmpCount; l++){ //for each dimension order in the comparison list
										//define the dimension length for the current dimension order
										dimLenC[0] = dimLen[dimOrder[dimCmp[l]][0]];
										dimLenC[1] = dimLen[dimOrder[dimCmp[l]][1]];
										dimLenC[2] = dimLen[dimOrder[dimCmp[l]][2]];
										for (m = 0; m < 4; m++){ //for each box corner associated with that dimension order
											//define the dimension moves for the current dimension order and box corner
											dimMovC[0] = dimMov[dimOrder[dimCmp[l]][0]] * cornerDir[dimCorner[l][m]][dimOrder[dimCmp[l]][0]];
											dimMovC[1] = dimMov[dimOrder[dimCmp[l]][1]] * cornerDir[dimCorner[l][m]][dimOrder[dimCmp[l]][1]];
											dimMovC[2] = dimMov[dimOrder[dimCmp[l]][2]] * cornerDir[dimCorner[l][m]][dimOrder[dimCmp[l]][2]];
											//go through the new shape in the direction of the current comparison, and the current link shape linearly, and check if they have cubes in matching positions
											pos = 0;
											posC = posL = posF = corner[dimCorner[l][m]];
											match = 1;
											for (n = 0; n < dimLenC[2]; n++){
												for (o = 0; o < dimLenC[1]; o++){
													for (p = 0; p < dimLenC[0]; p++){
														//we compare the two positions with an exclusive or. they differ if the result is 1
														if (bufShape.shape[posC] ^ shapeC->shape[pos++]){
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
												//if all the positions match, the shapes are identical, we can stop the comparisons
												isNew = 0;
												goto checkNew;
											}
										}
									}
								}
								//move to the next link in the list
								linkP = link;
								link = (void **) link[1];
							}
							//if the shape is new add it as the last link in the list
							checkNew: if (isNew){
								//create the permanent shape object
								shapeC = (CubeShape *) malloc(sizeof(CubeShape));
								shapeC->value = bufShape.value;
								shapeC->width = bufShape.width;
								shapeC->height = bufShape.height;
								shapeC->depth = bufShape.depth;
								count = bufShape.width * bufShape.height * bufShape.depth;
								shapeC->shape = (char *) malloc(count);
								for (l = 0; l < count; l++){
									shapeC->shape[l] = bufShape.shape[l];
								}
								//create the linked list that will contain the shape pointer
								link = (void **) malloc(sizeof(void *) * 2);
								link[0] = (void *) shapeC;
								link[1] = 0;
								//add the linked list as the next link in the current list
								linkP[1] = (void *) link;
								shapeCount++;
							}
						}
						else {
							//if there is no match, the shape is unique. add it to the dictionary
							//create the permanent shape object
							shapeC = (CubeShape *) malloc(sizeof(CubeShape));
							shapeC->value = bufShape.value;
							shapeC->width = bufShape.width;
							shapeC->height = bufShape.height;
							shapeC->depth = bufShape.depth;
							count = bufShape.width * bufShape.height * bufShape.depth;
							shapeC->shape = (char *) malloc(count);
							for (l = 0; l < count; l++){
								shapeC->shape[l] = bufShape.shape[l];
							}
							//create the linked list that will contain the shape pointer
							link = (void **) malloc(sizeof(void *) * 2);
							link[0] = (void *) shapeC;
							link[1] = 0;
							//add the linked list to the dictionary
							shapeDict[adjValue] = (void *) link;
							shapeCount++;
						}
					}
				}
			}
		}
		//clean up the creation box and potential shape array
		free(box);
		free(bufShape.shape);
	}
	//go through the shape dictionary, copy the shapes into the final shape array, and clean up the linked lists and shape objects
	shapeAll = (CubeShape *) malloc(sizeof(CubeShape) * shapeCount);
	pos = 0;
	for (i = 0; i < dictLen; i++){
		if (shapeDict[i]){
			link = (void **) shapeDict[i];
			while (link){
				shapeC = (CubeShape *) link[0];
				shapeAll[pos].value = shapeC->value;
				shapeAll[pos].width = shapeC->width;
				shapeAll[pos].height = shapeC->height;
				shapeAll[pos].depth = shapeC->depth;
				shapeAll[pos].shape = shapeC->shape;
				pos++;
				free((void *) shapeC);
				linkP = link;
				link = (void **) link[1];
				free((void *) linkP);
			}
		}
	}
	//return the final shape array and clean up the dictionary
	*descendents = shapeAll;
	free((void *) shapeDict);
	return shapeCount;
}