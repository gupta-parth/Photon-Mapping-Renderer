#pragma once
#define _CRT_SECURE_NO_WARNINGS
#define NOMINMAX


// OpenGL
#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>


// image loader and writer
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_write.h"


// linear algebra 
#include "linalg.h"
using namespace linalg::aliases;



// misc
#include <algorithm>
#include <cfloat>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>


// window size and resolution
// (do not make it too large - will be slow!)
constexpr int globalWidth = 512;
constexpr int globalHeight = 384;


// degree and radian
constexpr float PI = 3.14159265358979f;
constexpr float DegToRad = PI / 180.0f;
constexpr float RadToDeg = 180.0f / PI;


// for ray tracing
constexpr float Epsilon = 5e-5f;


// amount the camera moves with a mouse and a keyboard
constexpr float ANGFACT = 0.2f;
constexpr float SCLFACT = 0.1f;


// fixed camera parameters
constexpr float globalAspectRatio = float(globalWidth / float(globalHeight));
constexpr float globalFOV = 45.0f; // vertical field of view
constexpr float globalDepthMin = Epsilon; // for rasterization
constexpr float globalDepthMax = 100.0f; // for rasterization
constexpr float globalFilmSize = 0.032f; //for ray tracing
const float globalDistanceToFilm = globalFilmSize / (2.0f * tan(globalFOV * DegToRad * 0.5f)); // for ray tracing

