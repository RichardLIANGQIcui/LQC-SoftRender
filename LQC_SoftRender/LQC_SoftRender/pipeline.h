#pragma once
#include "macro.h"
#include "maths.h"
#include"spainlock.hpp"
#include "shader.h"
#include "win32.h"

const int WINDOW_HEDIGHT = 600;
const int WINDOW_WIDTH = 800;

//π‚’§ªØ
void rasterize_singlethread(vec4* cliproord_attri, unsigned char* framebuffer, float* zbuffer, IShader& shader);
void rasterize_multithread(vec4* clipcoord_attri, unsigned char* framebuffer, float* zbuffer, IShader& shader);
void draw_triangles(unsigned char* framebuffer, float* zbuffer, IShader& shader, int nface);

