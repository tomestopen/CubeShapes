//*** Cube Shapes Header ***//

#ifndef CUBE_SHAPES
	#define CUBE_SHAPES
	#include <stdlib.h>
	#include <stdint.h>

	typedef struct sCubeShape {
			int value;
			int width;
			int height;
			int depth;
			char *shape;
		} CubeShape;

	#ifdef BUILD_DLL
		#include <windows.h>
		#define DLL_EXPORT __declspec(dllexport)
		//***** DLL function prototypes *****//
		DLL_EXPORT int GetDescendents(CubeShape **descendents, CubeShape *source, int sourceCount);
		DLL_EXPORT void CleanShapeList(CubeShape *shapeList, int shapeCount);
		DLL_EXPORT int GetDescendentsMulti(CubeShape **descendents, CubeShape *source, int sourceCount, int maxThreads);
	#else
		//***** Shared object function prototypes *****//
		int GetDescendents(CubeShape **descendents, CubeShape *source, int sourceCount);
		void CleanShapeList(CubeShape *shapeList, int shapeCount);
	#endif
#endif
