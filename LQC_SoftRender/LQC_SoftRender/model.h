#pragma once
#include <string>
#include <vector>

#include "maths.h"
#include "tgaimage.h"

using namespace std;

typedef struct cubemap cubemap_t;

//ģ�͵Ķ���
//�ж������飬�����飬�������飬������������
//������������ͼ����
//������ͼ��������ͼ�Ͱ��������䡢��ȡ����ߡ��ֲڶȡ������ȵȵ�
//����������

class Model {
private:
	vector<vec3> verts;//ģ�Ͷ���
	vector<vector<int>> faces;//ģ����
	vector<vec3> norms;//ģ�ͷ���
	vector<vec2> uvs;//ģ����������

	void load_cubemap(const char* filename);//��ȡģ�����ݣ�������������ͼ
	void create_map(const char* filename);//����ģ�����ݴ�����ͼ
	//��������ĳ��ͼ����
	void load_texture(string filename, const char* suffix, TGAImage& img);
	void load_texture(string filename, const char* suffix, TGAImage* img);

public:
	Model(const char* filename, int is_skybox = 0, int is_form_mmd = 0);
	~Model();

	//��պ�
	cubemap_t* environment_map;
	int is_skybox;

	//map������ͼ
	int is_from_mmd;
	TGAImage* diffusemap;
	TGAImage* normalmap;
	TGAImage* specularmap;
	TGAImage* roughnessmap;
	TGAImage* metalnessmap;
	TGAImage* occlusion_map;
	TGAImage* emision_map;

	int nverts();//������
	int nfaces();//����
	vec3 normal(int iface, int nthvert);//��ȡĳ����ķ���
	vec3 normal(vec2 uv);
	vec3 vert(int i);//��ȡĳ��������Ӧ�Ķ���
	vec3 vert(int iface, int nthvert);//��ȡÿ�����ϵ�ĳ������

	vec2 uv(int iface, int nthvert);//��ȡĳ�����ϵ�ĳ�������������
	vec3 diffuse(vec2 uv);//�������������ȡ������ϵ��
	float roughness(vec2 uv);
	float metalness(vec2 uv);
	vec3 emission(vec2 uv);
	float occlusion(vec2 uv);
	float specular(vec2 uv);

	vector<int> face(int idx);//��ȡĳ��������Ķ�����


};