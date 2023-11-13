#pragma once
#include "macro.h"
#include "maths.h"
#include "model.h"
#include "camera.h"

//�ṹ�嶨��ƹ�:λ�ú�ǿ��
struct light
{
	vec3 pos;
	vec3 intensity;
};


//��ν��������ͼ������6�����ͼ��
typedef struct cubemap
{
	TGAImage* faces[6];
}cubemap_t;


//IBL����ͼ��Ĺ���
typedef struct iblmap
{
	int mip_levels;//���ڶ���mipmap����
	cubemap_t* irradiance_map;//���ڴ洢��ͬ�����ϵ�Ԥ�ȼ���õ�irradiance��Ҳ�Ƿ��ն�ͼ
	cubemap_t* prefilter_maps[15];//���ھ�����յ�Ԥ�˲�������ͼ��15��ʾ��ͬ�ֲڶ��´洢�Ľ��
	TGAImage* brdf_lut;//���ھ�����յĴ洢brdf�ļ�����
}iblmap_t;

//��һ���ṹ��payload�洢shader�е�uniform��������������ṹ����в�������
typedef struct
{
	//����һЩ������Ӱ��ͼ����Ĺ��վ���
	mat4 model_matrix;
	mat4 camera_view_matrix;//��ͼ����
	mat4 light_view_matrix;
	mat4 camera_perp_matrix;
	mat4 light_perp_matrix;
	mat4 mvp_matrix;

	Camera* camera;
	Model* model;

	//��������
	vec3 normal_attri[3];
	vec2 uv_attri[3];
	vec3 worldcoord_attri[3];
	vec4 clipcoord_attri[3];

	//������βü�
	vec3 in_normal[MAX_VERTEX];
	vec2 in_uv[MAX_VERTEX];
	vec3 in_worldcoord[MAX_VERTEX];
	vec4 in_clipcoord[MAX_VERTEX];
	vec3 out_normal[MAX_VERTEX];
	vec2 out_uv[MAX_VERTEX];
	vec3 out_worldcoord[MAX_VERTEX];
	vec4 out_clipcoord[MAX_VERTEX];

	/*bool isPBR = false;*///��Ϊû�����ù�Դ����������PBRֻ�м�ӹ���

	//���ڻ���ͼ��Ĺ���
	iblmap_t* iblmap;
}payload_t;

//��ɫ���Ķ���
//�ɶ�����һЩҪ���������payload
//���ж����Ƭ����ɫ��
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
