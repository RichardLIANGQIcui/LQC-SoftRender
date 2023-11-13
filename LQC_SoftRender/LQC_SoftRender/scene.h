#pragma once
#include "tgaimage.h"
#include "shader.h"

typedef struct
{
	const char* scene_name;
	//声明一个函数指针
	void (*build_scene)(Model** model, int& m, IShader** shader_use, IShader** shader_sky_box, mat4 perspective, Camera* camera_);

}scene_t;

TGAImage* texture_from_file(const char* file_name);
void load_ibl_map(payload_t& p, const char* env_path);

void build_fuhua_scene(Model** model, int& m, IShader** shader_use, IShader** shader_skybox, mat4 perspective, Camera* camera);
void build_yayi_scene(Model** model, int& m, IShader** shader_use, IShader** shader_skybox, mat4 perspective, Camera* camera);
void build_qiyana_scene(Model** model, int& m, IShader** shader_use, IShader** shader_skybox, mat4 perspective, Camera* camera);
void build_xier_scene(Model** model, int& m, IShader** shader_use, IShader** shader_skybox, mat4 perspective, Camera* camera);
void build_helmet_scene(Model** model, int& m, IShader** shader_use, IShader** shader_skybox, mat4 perspective, Camera* camera);
void build_gun_scene(Model** model, int& m, IShader** shader_use, IShader** shader_skybox, mat4 perspective, Camera* camera);
