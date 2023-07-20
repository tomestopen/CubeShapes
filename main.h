//*** Cube Shapes Header ***//

#ifndef CUBE_SHAPES
	#define CUBE_SHAPES
	#include <stdlib.h>
	#include <stdint.h>
	#ifdef BUILD_DLL
		#include <windows.h>
		#define DLL_EXPORT __declspec(dllexport)
		//***** DLL function prototypes *****//
	#else
		//***** Shared object function prototypes *****//
	#endif
#endif
