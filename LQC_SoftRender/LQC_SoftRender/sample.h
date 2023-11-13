#pragma once
#include "shader.h"

//定义采样，分立方体贴图采样和纹理采样
//立方体贴图采样需要知道方向和是什么类型的立方体贴图，纹理同理
//
//生成预滤波环境贴图函数和辐照度贴图函数

vec3 cubemap_sampling(vec3 direction, cubemap_t* cubemap);
vec3 texture_sample(vec2 uv, TGAImage* image);

void generate_prefilter_map(int thread_id, int face_id, int mip_level, TGAImage& image);
void generate_irradiance_map(int thread_id, int face_id, TGAImage& image);
