//*** Cube Shapes Header ***//

#ifndef CUBE_SHAPES
	#define CUBE_SHAPES
	#include <stdlib.h>
	#include <stdint.h>

	typedef struct sCubeShape {
			char *box;
			int value;
			int edgeValue;
			int connections;
			int width;
			int height;
			int depth;
		} CubeShape;

	#ifdef BUILD_DLL
		#include <windows.h>
		#define DLL_EXPORT __declspec(dllexport)
		//***** DLL function prototypes *****//
		DLL_EXPORT int GetDescendents(CubeShape **descendents, CubeShape *source, int sourceCount);
		DLL_EXPORT void CleanShapeList(CubeShape *shapeList, int shapeCount);
		DLL_EXPORT int CompareShapeLists(CubeShape *firstList, int firstLength, CubeShape *secondList, int secondLength, CubeShape *missingList);
		DLL_EXPORT void SetShapeListValues(CubeShape *shapeList, int listLength);
		DLL_EXPORT int CheckDistinct(CubeShape *firstShape, CubeShape *secondShape);
		DLL_EXPORT int GetDescendentsMulti(CubeShape **descendents, CubeShape *source, int sourceCount, int maxThreads);
	#else
		//***** Shared object function prototypes *****//
		int GetDescendents(CubeShape **descendents, CubeShape *source, int sourceCount);
		void CleanShapeList(CubeShape *shapeList, int shapeCount);
		int CompareShapeLists(CubeShape *firstList, int firstLength, CubeShape *secondList, int secondLength, CubeShape *missingList);
		void SetShapeListValues(CubeShape *shapeList, int listLength);
		int CheckDistinct(CubeShape *firstShape, CubeShape *secondShape);
	#endif
#endif
