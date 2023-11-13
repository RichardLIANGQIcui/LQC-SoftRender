#pragma once
#include "macro.h"
#include "maths.h"
#include "model.h"
#include "camera.h"

//结构体定义灯光:位置和强度
struct light
{
	vec3 pos;
	vec3 intensity;
};


//所谓立方体贴图就是有6个面的图像
typedef struct cubemap
{
	TGAImage* faces[6];
}cubemap_t;


//IBL基于图像的光照
typedef struct iblmap
{
	int mip_levels;//用于定义mipmap级别
	cubemap_t* irradiance_map;//用于存储不同方向上的预先计算好的irradiance，也是辐照度图
	cubemap_t* prefilter_maps[15];//用于镜面光照的预滤波环境贴图，15表示不同粗糙度下存储的结果
	TGAImage* brdf_lut;//用于镜面光照的存储brdf的计算结果
}iblmap_t;

//用一个结构体payload存储shader中的uniform参数，利用这个结构体进行参数传递
typedef struct
{
	//定义一些用于阴影贴图计算的光照矩阵
	mat4 model_matrix;
	mat4 camera_view_matrix;//视图矩阵
	mat4 light_view_matrix;
	mat4 camera_perp_matrix;
	mat4 light_perp_matrix;
	mat4 mvp_matrix;

	Camera* camera;
	Model* model;

	//顶点属性
	vec3 normal_attri[3];
	vec2 uv_attri[3];
	vec3 worldcoord_attri[3];
	vec4 clipcoord_attri[3];

	//用于齐次裁剪
	vec3 in_normal[MAX_VERTEX];
	vec2 in_uv[MAX_VERTEX];
	vec3 in_worldcoord[MAX_VERTEX];
	vec4 in_clipcoord[MAX_VERTEX];
	vec3 out_normal[MAX_VERTEX];
	vec2 out_uv[MAX_VERTEX];
	vec3 out_worldcoord[MAX_VERTEX];
	vec4 out_clipcoord[MAX_VERTEX];

	/*bool isPBR = false;*///因为没有设置光源，所以这里PBR只有间接光照

	//用于基于图像的光照
	iblmap_t* iblmap;
}payload_t;

//着色器的定义
//可定包含一些要计算的属性payload
//还有顶点和片段着色器
class IShader
{
public:
	payload_t payload;
	virtual void vertex_shader(int nfaces,int nvertex){}
	virtual vec3 fragment_shader(float alpha, float beta, float gamma) { return vec3(0, 0, 0); }
};

class PhongShader :public IShader
{
public:
	void vertex_shader(int nfaces, int nvertex);
	vec3 fragment_shader(float alpha, float beta, float gamma);
};

class PBRShader :public IShader
{
public:
	void vertex_shader(int nfaces, int nvertex);
	vec3 direct_fragment_shader(float alpha, float beta, float gamma);
	vec3 fragment_shader(float alpha, float beta, float gamma);
};

class SkyboxShader :public IShader
{
public:
	void vertex_shader(int nfaces, int nvertex);
	vec3 fragment_shader(float alpha, float beta, float gamma);
};
