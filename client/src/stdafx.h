// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#ifdef _WIN64
	static_assert(false, "This program must be compiled in x86.");
#endif

#ifndef _WIN32
	static_assert(false, "This program must be compiled for Windows.");	
#endif

#define NOMINMAX

#include "targetver.h"

#include <stdio.h>
#include <tchar.h>
#include <windows.h>

// <assert.h> doesn't compile in release
#define ASSERT(assertion) { if (!(assertion)) { assert(0); fprintf( stderr, "\n%s:%i in %s ASSERTION FAILED2:\n  %s\n", __FILE__, __LINE__,__FUNCTION__,  #assertion); int* ptr = nullptr; *ptr = 0; } }

// TODO: reference additional headers your program requires here

#include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>

using namespace std;

#include "SfExtensions.h"
#include "../Logger.h"