#include "./model.h"

#include <io.h> 
#include <iostream>
#include <fstream>
#include <sstream>

#include "shader.h"

using namespace std;

//这个类的构造函数接受三个参数：文件名、一个布尔值is_skybox和另一个布尔值is_from_mmd。
//前两个参数用于确定是否加载天空盒纹理以及是否从MikuMikuDance软件中导入模型
//后者会影响纹理坐标的垂直翻转，同时mmd的角色模型我们采取blinn - phong渲染，
//非mmd角色模型我们采取PBR渲染。


Model::Model(const char* filename, int is_skybox, int is_from_mmd):
	is_skybox(is_skybox),is_from_mmd(is_from_mmd)
{
	ifstream in;
	in.open(filename, ifstream::in);
	if (in.fail())
	{
		cout << "load model failed" << endl;
		return;
	}

	string line;
	while (!in.eof())//一直读到文件末尾
	{
		getline(in, line);
		//istringstream可以根据空格分割string,这就为什么要从ifstream先读出数据再转到istringstream
		istringstream iss(line.c_str());
		char trash;
		if (!line.compare(0, 2, "v"))//(对应标签)依次来区别数据属性
		{
			iss >> trash;
			vec3 v;
			for (int i = 0;i < 3;i++)
			{
				iss >> v[i];
			}
			verts.push_back(v);//将模型顶点数据读入到数组中保存
		}
		else if (!line.compare(0, 3, "vn"))
		{
			//在读取行时，将字符串line和指定的开头字符串进行比较，
			//从而确定这是一个顶点坐标行还是法线行。但是，在处理法线行时，
			//"vn"后面还有一个" "空格，所以需要将iss流多提取一个"trash"变量，
			//以便将流指针移动到接下来的三个浮点数。因此，在读取法线行时需要多
			/*提取一个"trash"变量，而在读取顶点坐标行时，"v"后面没有空格，
				所以不需要多提取。*/
		iss >> trash >> trash;
			vec3 n;
			for (int i = 0;i < 3;i++)
			{
				iss >> n[i];
			}
			norms.push_back(n);
		}
		else if (!line.compare(0, 3, "vt"))
		{
			iss >> trash >> trash;
			vec2 uv;
			for (int i = 0;i < 2;i++)
			{
				iss >> uv[i];
			}
			if (is_from_mmd)
			{
				uv[1] = 1 - uv[1];
			}

			uvs.push_back(uv);
		}
		else if (!line.compare(0, 2, "f"))//读入对应面的顶点数
		{
			vector<int> f;
			int tmp[3];
			iss >> trash;
			while (iss >> tmp[0] >> trash >> tmp[1] >> trash >> tmp[2])
			{
				for (int i = 0;i < 3;i++)
				{
					tmp[i]--;//把索引从0开始设置，模型中是从1开始
				}
				f.push_back(tmp[0]);
				f.push_back(tmp[1]);
				f.push_back(tmp[2]);
			}
			faces.push_back(f);
		}
	}
	std::cerr << "# v# " << verts.size() << " f# " << faces.size() << " vt# " << uvs.size() << " vn# " << norms.size() << std::endl;

	create_map(filename);

	environment_map = NULL;
	if (is_skybox)
	{
		environment_map = new cubemap_t();
		load_cubemap(filename);
	}

}

Model::~Model()
{
	if (diffusemap)
	{
		delete diffusemap;
		diffusemap = nullptr;
	}
	if (normalmap)
	{
		delete normalmap;
		normalmap = nullptr;
	}
	if (specularmap)
	{
		delete specularmap;
		specularmap = nullptr;
	}
	if (roughnessmap)
	{
		delete roughnessmap;
		roughnessmap = nullptr;
	}
	if (metalnessmap)
	{
		delete metalnessmap;
		metalnessmap = nullptr;
	}
	if (occlusion_map)
	{
		delete occlusion_map;
		occlusion_map = nullptr;
	}
	if (emision_map)
	{
		delete emision_map;
		emision_map = nullptr;
	}

	if (environment_map)
	{
		for (int i = 0;i < 6;i++)
		{
			delete environment_map->faces[i];
		}
		delete environment_map;
	}
}

//初始化各种贴图
void Model::create_map(const char* filename)
{
	diffusemap = NULL;
	normalmap = NULL;
	specularmap = NULL;
	roughnessmap = NULL;
	metalnessmap = NULL;
	occlusion_map = NULL;
	emision_map = NULL;

	string texfile(filename);
	size_t dot = texfile.find_last_of(".");
	// _access(texfile.data(), 0)是一个windows特有的函数，用于检查文件是否存在。
	//如果返回值为 - 1, 则表示文件不存在
	texfile = texfile.substr(0, dot) + string("_diffuse.tga");
	if (_access(texfile.data(), 0) != -1)
	{
		diffusemap = new TGAImage();
		load_texture(filename, "_diffuse.tga", diffusemap);
	}
	texfile = texfile.substr(0, dot) + string("_normal.tga");
	if (_access(texfile.data(), 0) != -1)
	{
		normalmap = new TGAImage();
		load_texture(filename, "_normal.tga", normalmap);
	}
	texfile = texfile.substr(0, dot) + string("_spec.tga");
	if (_access(texfile.data(), 0) != -1)
	{
		specularmap = new TGAImage();
		load_texture(filename, "_spec.tga", specularmap);
	}

	texfile = texfile.substr(0, dot) + string("_roughness.tga");
	if (_access(texfile.data(), 0) != -1)
	{
		roughnessmap = new TGAImage();
		load_texture(filename, "_roughness.tga", roughnessmap);
	}

	texfile = texfile.substr(0, dot) + string("_metalness.tga");
	if (_access(texfile.data(), 0) != -1)
	{
		metalnessmap = new TGAImage();
		load_texture(filename, "_metalness.tga", metalnessmap);
	}

	texfile = texfile.substr(0, dot) + string("_emission.tga");
	if (_access(texfile.data(), 0) != -1)
	{
		emision_map = new TGAImage();
		load_texture(filename, "_emission.tga", emision_map);
	}

	texfile = texfile.substr(0, dot) + string("_occlusion.tga");
	if (_access(texfile.data(), 0) != -1)
	{
		occlusion_map = new TGAImage();
		load_texture(filename, "_occlusion.tga", occlusion_map);
	}

}

//立方体贴图6个面都是一个TGAImage指针，这个加载和上面一样
void Model::load_cubemap(const char* filename)
{
	environment_map->faces[0] = new TGAImage();
	load_texture(filename, "_right.tga", environment_map->faces[0]);
	environment_map->faces[1] = new TGAImage();
	load_texture(filename, "_left.tga", environment_map->faces[1]);
	environment_map->faces[2] = new TGAImage();
	load_texture(filename, "_top.tga", environment_map->faces[2]);
	environment_map->faces[3] = new TGAImage();
	load_texture(filename, "_bottom.tga", environment_map->faces[3]);
	environment_map->faces[4] = new TGAImage();
	load_texture(filename, "_back.tga", environment_map->faces[4]);
	environment_map->faces[5] = new TGAImage();
	load_texture(filename, "_front.tga", environment_map->faces[5]);
}


int Model::nverts()
{
	return (int)verts.size();
}

int Model::nfaces()
{
	return (int)faces.size();
}


std::vector<int> Model::face(int idx)
{
	vector<int> face;
	for (int i = 0;i < 3;i++)
	{
		face.push_back(faces[idx][i * 3]);
	}
	return face;
}

vec3 Model::vert(int i)
{
	return verts[i];
}

vec3 Model::vert(int iface, int nthvert)
{
	return verts[faces[iface][nthvert * 3]];//所以face里面存的是各个属性的索引值
}

vec2 Model::uv(int iface, int nthvert)
{
	return uvs[faces[iface][nthvert * 3]];
}

vec3 Model::normal(int iface, int nthvert)
{
	int idx = faces[iface][nthvert * 3 + 2];
	return unit_vector(norms[idx]);
}

void Model::load_texture(std::string filename, const char* suffix, TGAImage& img)
{
	string texfile(filename);
	size_t dot = texfile.find_last_of(".");
	if (dot != string::npos)
	{
		texfile = texfile.substr(0, dot) + string(suffix);
		img.read_tga_file(texfile.c_str());
		img.flip_vertically();
	}

}
//这两个函数的区别主要在于参数传递方式的不同，一个通过引用传递，另一个通过指针传递。
void Model::load_texture(std::string filename, const char* suffix, TGAImage* img)
{
	string texfile(filename);
	size_t dot = texfile.find_last_of(".");
	if (dot != string::npos)
	{
		texfile = texfile.substr(0, dot) + string(suffix);
		img->read_tga_file(texfile.c_str());
		img->flip_vertically();
	}
}

vec3 Model::diffuse(vec2 uv)
{
	//先对UV纹理坐标取模运算保证其在0-1范围内
	uv[0] = fmod(uv[0], 1);
	uv[1] = fmod(uv[1], 1);
	//乘上图像宽高映射回像素具体位置
	int uv0 = uv[0] * diffusemap->get_width();
	int uv1 = uv[1] * diffusemap->get_height();
	TGAColor c = diffusemap->get(uv0, uv1);//获取像素值，范围为0-255
	
	//将获取到的值进行归一化处理，将其从0-255转换到0-1之间
	//又因为存储顺序不一样，所以需要反转，因为贴图存的是BGR
	vec3 res;
	for (int i = 0;i < 3;i++)
	{
		res[2 - i] = (float)c[i] / 255.f;
	}

	return res;

}

vec3 Model::normal(vec2 uv)
{
	uv[0] = fmod(uv[0], 1);
	uv[1] = fmod(uv[1], 1);
	int uv0 = uv[0] * normalmap->get_width();
	int uv1 = uv[1] * normalmap->get_height();

	TGAColor c = normalmap->get(uv0, uv1);
	vec3 res;
	for (int i = 0;i < 3;i++)
	{
		res[2 - i] = (float)c[i] / 255.f * 2.f - 1.f;
	}
	return res;
}

float Model::roughness(vec2 uv)
{
	uv[0] = fmod(uv[0], 1);
	uv[1] = fmod(uv[1], 1);
	int uv0 = uv[0] * roughnessmap->get_width();
	int uv1 = uv[1] * roughnessmap->get_height();

	return roughnessmap->get(uv0, uv1)[0] / 255.f;
}

float Model::metalness(vec2 uv)
{
	uv[0] = fmod(uv[0], 1);
	uv[1] = fmod(uv[1], 1);
	int uv0 = uv[0] * metalnessmap->get_width();
	int uv1 = uv[1] * metalnessmap->get_height();

	return metalnessmap->get(uv0, uv1)[0] / 255.f;

}

float Model::specular(vec2 uv)
{
	uv[0] = fmod(uv[0], 1);
	uv[1] = fmod(uv[1], 1);
	int uv0 = uv[0] * specularmap->get_width();
	int uv1 = uv[1] *specularmap->get_height();

	return specularmap->get(uv0, uv1)[0] / 255.f;
}

float Model::occlusion(vec2 uv)
{
	uv[0] = fmod(uv[0], 1);
	uv[1] = fmod(uv[1], 1);
	int uv0 = uv[0] * occlusion_map->get_width();
	int uv1 = uv[1] * occlusion_map->get_height();

	return occlusion_map->get(uv0, uv1)[0] / 255.f;
}

vec3 Model::emission(vec2 uv)
{
	uv[0] = fmod(uv[0], 1);
	uv[1] = fmod(uv[1], 1);
	int uv0 = uv[0] * emision_map->get_width();
	int uv1 = uv[1] * emision_map->get_height();

	TGAColor c = emision_map->get(uv0, uv1);

	vec3 res;
	for (int i = 0;i < 3;i++)
	{
		res[2 - i] = (float)c[i] / 255.f;
	}
	return res;
}
