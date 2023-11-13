#include "scene.h"

using namespace std;

//在scene中调用图像函数读取图像数据
TGAImage* texture_from_file(const char* file_name)
{
	TGAImage* texture = new TGAImage();
	texture->read_tga_file(file_name);
	texture->flip_vertically();
	return texture;

}

cubemap_t* cubemap_from_files(const char* positive_x, const char* negative_x,
	const char* positive_y, const char* negative_y,
	const char* positive_z, const char* negative_z)//6个面的路径
{
	cubemap_t* cubemap = new cubemap_t();
	cubemap->faces[0] = texture_from_file(positive_x);
	cubemap->faces[1] = texture_from_file(negative_x);
	cubemap->faces[2] = texture_from_file(positive_y);
	cubemap->faces[3] = texture_from_file(negative_y);
	cubemap->faces[4] = texture_from_file(positive_z);
	cubemap->faces[5] = texture_from_file(negative_z);

	return cubemap;
}

//从场景中加载IBL环境贴图,其中辐照度图和预滤波环境贴图都是立方体贴图，而LUT则是单张贴图
void load_ibl_map(payload_t& p, const char* env_path)
{
	int i, j;
	iblmap_t* iblmap = new iblmap_t();
	const char* faces[6] = { "px","nx","py","ny","pz","nz" };
	char paths[6][256];

	iblmap->mip_levels = 10;

	//漫反射环境贴图
	for (j = 0;j < 6;j++)
	{
		sprintf_s(paths[j], "%s/i_%s.tga", env_path, faces[j]);//将格式化的数据写入字符串paths中
	}
	//再从path中读取内容
	iblmap->irradiance_map = cubemap_from_files(paths[0], paths[1], paths[2], paths[3]
		, paths[5], paths[5]);

	//镜面反射环境贴图,把不同级别的mipmap和粗糙度对应的立方体贴图都生成
	for (i = 0;i < iblmap->mip_levels;i++)
	{
		for (j = 0;j < 6;j++)
		{
			sprintf_s(paths[j], "%s/m%d_%s.tga", env_path, i, faces[j]);
		}
		iblmap->prefilter_maps[i] = cubemap_from_files(paths[0], paths[1], paths[2], paths[3]
			, paths[5], paths[5]);
	}

	//lut图生成
	iblmap->brdf_lut = texture_from_file("E:/SRender/obj/common/BRDF_LUT.tga");

	p.iblmap = iblmap;

}


//下面6个场景的生成逻辑
//1、通过实例化模型时传入obj文件路径，从而读取顶点数据到model中存储
//2、然后创立着色器，根据模型的不同创立不同的着色器
//设置并传入着色器中的一些参数，比如顶点数、面数、相机、矩阵、ibl环境贴图的读取，
//也就是说这整个过程实际上就是将场景中的数据读取然后生成已经计算好的shader
//后续直接调用shader中的值绘制即可



//场景一
void build_fuhua_scene(Model** model, int& m, IShader** shader_use, IShader** shader_skybox,
	mat4 perspective, Camera* camera)
{
	m = 4;
	//读取模型的路径
	const char* modelname[] = {
		"E:/SRender/obj/fuhua/fuhuabody.obj",
		"E:/SRender/obj/fuhua/fuhuahair.obj",
		"E:/SRender/obj/fuhua/fuhuaface.obj",
		"E:/SRender/obj/fuhua/fuhuacloak.obj",
	};

	int vertex = 0, face = 0;
	const char* scene_name = "fuhua";
	PhongShader* shader_phong = new PhongShader();

	for (int i = 0;i < m;i++)
	{
		//在实例化model的过程中就已经将顶点数和面数存到了model中
		model[i] = new Model(modelname[i], 0, 1);
		vertex += model[i]->nverts();
		face += model[i]->nfaces();
	}

	shader_phong->payload.camera_perp_matrix = perspective;
	shader_phong->payload.camera = camera;

	*shader_use = shader_phong;
	*shader_skybox = NULL;


	printf("scene name:%s\n", scene_name);
	printf("model number:%d\n", m);
	printf("vertex:%d faces:%d\n", vertex, face);

}

//场景2
void build_qiyana_scene(Model** model, int& m, IShader** shader_use, IShader** shader_skybox,
	mat4 perspective, Camera* camera)
{
	m = 3;
	//读取模型的路径
	const char* modelname[] = {
		"E:/SRender/obj/qiyana/qiyanabody.obj",
		"E:/SRender/obj/qiyana/qiyanabody.obj",
		"E:/SRender/obj/qiyana/qiyanabody.obj",
	};

	int vertex = 0, face = 0;
	const char* scene_name = "qiyana";
	PhongShader* shader_phong = new PhongShader();

	for (int i = 0;i < m;i++)
	{
		model[i] = new Model(modelname[i], 0, 1);
		vertex += model[i]->nverts();
		face += model[i]->nfaces();
	}

	shader_phong->payload.camera_perp_matrix = perspective;
	shader_phong->payload.camera = camera;

	*shader_use = shader_phong;
	*shader_skybox = NULL;


	printf("scene name:%s\n", scene_name);
	printf("model number:%d\n", m);
	printf("vertex:%d faces:%d\n", vertex, face);

}


//场景3
void build_xier_scene(Model** model, int& m, IShader** shader_use, IShader** shader_skybox, mat4 perspective, Camera* camera)
{
	m = 5;
	const char* modelname[] =
	{
		"E:/SRender/obj/xier/xierbody.obj",
		"E:/SRender/obj/xier/xierhair.obj",
		"E:/SRender/obj/xier/xierface.obj",
		"E:/SRender/obj/xier/xiercloth.obj",
		"E:/SRender/obj/xier/xierarm.obj",
	};

	int vertex = 0, face = 0;
	const char* scene_name = "xier";
	PhongShader* shader_phong = new PhongShader();

	for (int i = 0; i < m; i++)
	{
		model[i] = new Model(modelname[i], 0, 1);
		vertex += model[i]->nverts();
		face += model[i]->nfaces();
	}

	shader_phong->payload.camera_perp_matrix = perspective;
	shader_phong->payload.camera = camera;

	*shader_use = shader_phong;
	*shader_skybox = NULL;

	printf("scene name:%s\n", scene_name);
	printf("model number:%d\n", m);
	printf("vertex:%d faces:%d\n", vertex, face);
}


//场景4
void build_yayi_scene(Model** model, int& m, IShader** shader_use, IShader** shader_skybox, mat4 perspective, Camera* camera)
{
	m = 7;
	const char* modelname[] = {
		"E:/SRender/obj/yayi/yayiface.obj",
		"E:/SRender/obj/yayi/yayibody.obj",
		"E:/SRender/obj/yayi/yayihair.obj",
		"E:/SRender/obj/yayi/yayiarmour1.obj",
		"E:/SRender/obj/yayi/yayiarmour2.obj",
		"E:/SRender/obj/yayi/yayidecoration.obj",
		"E:/SRender/obj/yayi/yayisword.obj"
	};

	int vertex = 0, face = 0;
	const char* scene_name = "yayi";
	PhongShader* shader_phong = new PhongShader();

	for (int i = 0; i < m; i++)
	{
		model[i] = new Model(modelname[i], 0, 1);
		vertex += model[i]->nverts();
		face += model[i]->nfaces();
	}

	shader_phong->payload.camera_perp_matrix = perspective;
	shader_phong->payload.camera = camera;

	*shader_use = shader_phong;
	*shader_skybox = NULL;

	printf("scene name:%s\n", scene_name);
	printf("model number:%d\n", m);
	printf("vertex:%d faces:%d\n", vertex, face);
}


//场景5，这里用到了pbr
void build_helmet_scene(Model** model, int& m, IShader** shader_use, IShader** shader_skybox, mat4 perspective, Camera* camera)
{
	m = 2;
	const char* modelname[] =
	{
		"E:/SRender/obj/helmet/helmet.obj",
		"E:/SRender/obj/skybox4/box.obj",
	};

	PBRShader* shader_pbr = new PBRShader();
	SkyboxShader* shader_sky = new SkyboxShader();

	int vertex = 0, face = 0;
	const char* scene_name = "helmet";
	model[0] = new Model(modelname[0], 0, 0); vertex += model[0]->nverts(); face += model[0]->nfaces();
	model[1] = new Model(modelname[1], 1, 0); vertex += model[1]->nverts(); face += model[1]->nfaces();

	shader_pbr->payload.camera_perp_matrix = perspective;
	shader_pbr->payload.camera = camera;
	shader_sky->payload.camera_perp_matrix = perspective;
	shader_sky->payload.camera = camera;

	load_ibl_map(shader_pbr->payload, "E:/SRender/obj/common2");

	*shader_use = shader_pbr;
	*shader_skybox = shader_sky;

	printf("scene name:%s\n", scene_name);
	printf("model number:%d\n", m);
	printf("vertex:%d faces:%d\n", vertex, face);
}

//场景6，这里用到了pbr
void build_gun_scene(Model** model, int& m, IShader** shader_use, IShader** shader_skybox, mat4 perspective, Camera* camera)
{
	m = 2;
	const char* modelname[] =
	{
		"E:/SRender/obj/gun/Cerberus.obj",
		"E:/SRender/obj/skybox4/box.obj",
	};

	PBRShader* shader_pbr = new PBRShader();
	SkyboxShader* shader_sky = new SkyboxShader();

	int vertex = 0, face = 0;
	const char* scene_name = "gun";
	model[0] = new Model(modelname[0], 0, 0); vertex += model[0]->nverts(); face += model[0]->nfaces();
	model[1] = new Model(modelname[1], 1, 0); vertex += model[1]->nverts(); face += model[1]->nfaces();

	shader_pbr->payload.camera_perp_matrix = perspective;
	shader_pbr->payload.camera = camera;
	shader_sky->payload.camera_perp_matrix = perspective;
	shader_sky->payload.camera = camera;

	load_ibl_map(shader_pbr->payload, "E:/SRender/obj/common2");

	*shader_use = shader_pbr;
	*shader_skybox = shader_sky;

	printf("scene name:%s\n", scene_name);
	printf("model number:%d\n", m);
	printf("vertex:%d faces:%d\n", vertex, face);
}

