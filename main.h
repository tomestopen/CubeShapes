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
	#else
		//***** Shared object function prototypes *****//
	#endif
#endif
