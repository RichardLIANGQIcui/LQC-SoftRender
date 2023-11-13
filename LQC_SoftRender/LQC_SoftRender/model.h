#pragma once
#include <string>
#include <vector>

#include "maths.h"
#include "tgaimage.h"

using namespace std;

typedef struct cubemap cubemap_t;

//模型的定义
//有顶点数组，面数组，法线数组，纹理坐标数组
//加载立方体贴图函数
//创建贴图函数，贴图就包括漫反射、深度、法线、粗糙度、金属度等等
//加载纹理函数

class Model {
private:
	vector<vec3> verts;//模型顶点
	vector<vector<int>> faces;//模型面
	vector<vec3> norms;//模型法线
	vector<vec2> uvs;//模型纹理坐标

	void load_cubemap(const char* filename);//读取模型数据，加载立方体贴图
	void create_map(const char* filename);//利用模型数据创建贴图
	//加载纹理到某个图像上
	void load_texture(string filename, const char* suffix, TGAImage& img);
	void load_texture(string filename, const char* suffix, TGAImage* img);

public:
	Model(const char* filename, int is_skybox = 0, int is_form_mmd = 0);
	~Model();

	//天空盒
	cubemap_t* environment_map;
	int is_skybox;

	//map各种贴图
	int is_from_mmd;
	TGAImage* diffusemap;
	TGAImage* normalmap;
	TGAImage* specularmap;
	TGAImage* roughnessmap;
	TGAImage* metalnessmap;
	TGAImage* occlusion_map;
	TGAImage* emision_map;

	int nverts();//顶点数
	int nfaces();//面数
	vec3 normal(int iface, int nthvert);//获取某个面的法线
	vec3 normal(vec2 uv);
	vec3 vert(int i);//获取某个索引对应的顶点
	vec3 vert(int iface, int nthvert);//获取每个面上的某个顶点

	vec2 uv(int iface, int nthvert);//获取某个面上的某个点的纹理坐标
	vec3 diffuse(vec2 uv);//根据纹理坐标获取漫反射系数
	float roughness(vec2 uv);
	float metalness(vec2 uv);
	vec3 emission(vec2 uv);
	float occlusion(vec2 uv);
	float specular(vec2 uv);

	vector<int> face(int idx);//获取某个面包含的顶点数


};