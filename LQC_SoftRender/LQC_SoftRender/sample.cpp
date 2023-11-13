#include "sample.h"
#include <stdlib.h>
#include <thread>

using namespace std;
 
//这个函数计算的是采样方向击中了贴图中的哪个面，同时求在这个面对应的纹理坐标
//原理就是首先立方体6个面都有各自对应的编号0-5，再一个就是根据方向的xyz大小可以知道击中哪个面
//如果是x值大于其他两个方向的值，说明方向是朝向x轴方向，再根据正负号判断击中的面
//方向确定后如何求纹理坐标
//由于最后要求的纹理坐标范围是0-1之间
//假如是x方向的值最大，那么就要把yz方向的值除以x转换到-1-1
//然后由于立方体的坐标在-1-1之间，所以还需要+1再除2

static int cal_cubemap_uv(vec3 direction, vec2& uv)
{
	int face_index = -1;
	float ma = 0, sc = 0, tc = 0;
	float abs_x = fabs(direction[0]), abs_y = fabs(direction[1]), abs_z = fabs(direction[2]);

	if (abs_x > abs_y && abs_x > abs_z)
	{
		ma = abs_x;
		if (direction[0] > 0)
		{
			face_index = 0;
			sc = direction[1];
			tc = direction[2];
		}
		else
		{
			face_index = 1;
			sc = -direction.z();
			tc = direction.y();
		}
	}
	else if (abs_y > abs_z)
	{
		ma = abs_y;
		if (direction.y() > 0)
		{
			face_index = 2;
			sc = direction.x();
			tc = direction.z();
		}
		else
		{
			face_index = 3;
			sc = direction.x();
			tc = -direction.z();
		}
	}
	else
	{
		ma = abs_z;
		if (direction.z() > 0)
		{
			face_index = 4;
			sc = -direction.x();
			tc = direction.y();
		}
		else
		{
			face_index = 5;
			sc = direction.x();
			tc = direction.z();
		}
	}

	uv[0] = (sc / ma + 1.0f) / 2.0f;

	uv[1] = (tc / ma + 1.0f) / 2.0f;

	return face_index;
}


vec3 texture_sample(vec2 uv, TGAImage* image)
{
	//对uv纹理坐标进行取模运算，限定范围在0-1之间
	uv[0] = fmod(uv[0], 1);
	uv[1] = fmod(uv[1], 1);

	int uv0 = uv[0] * image->get_width();
	int uv1 = uv[1] * image->get_height();

	TGAColor c = image->get(uv0, uv1);

	vec3 res;
	for (int i = 0;i < 3;i++)
	{
		res[2 - i] = (float)c[i] / 255.f;
	}
	return res;
 }


vec3 cubemap_sampling(vec3 direction, cubemap_t* cubemap)
{
	vec2 uv;
	vec3 color;
	int face_index = cal_cubemap_uv(direction, uv);
	color = texture_sample(uv, cubemap->faces[face_index]);

	return color;
}
//预计算基于图像的光照
//低差异序列用于重要性采样，该方法具有更快的收敛速度，且随机分布更均匀
float radicalInverse_VdC(unsigned int bits)
{
	bits = (bits << 16u) | (bits >> 16u);
	bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
	bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
	bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
	bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
	return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

vec2 hammersley2d(unsigned int i, unsigned int N) {
	return vec2(float(i) / float(N), radicalInverse_VdC(i));
}

//球面均匀采样
vec3 hemisphereSample_uniform(float u, float v)
{
	float phi = 2.0f * v * PI;
	float cosTheta = 1.0f - u;//随机半球方向的z轴向量
	float sinTheta = sqrt(1.0f - cosTheta * cosTheta);//再定一个随机角度
	return vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
}

vec3 hemisphereSample_cos(float u, float v)
{
	float phi = v * 2.0 * PI;
	float cosTheta = sqrt(1.0 - u);
	float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
	return vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
}

//重要性采样利用了GGX求采样方向，GGX是在PBR中的法线分布函数，表示的是
vec3 ImportanceSampleGGX(vec2 Xi, vec3 N, float roughness)
{
	//Xi是由低差异序列生成的随机向量，把这个向量转换为H
	float a = roughness * roughness;

	//为了将随机向量转换成H的笛卡尔坐标作的预先计算
	float phi = 2.0 * PI * Xi.x();
	float cosTheta = sqrt((1.0 - Xi.y())) / (1.0 + (a * a - 1.0) * Xi.y());
	float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

	vec3 H;
	H[0] = cos(phi) * sinTheta;
	H[1] = sin(phi) * sinTheta;
	H[2] = cosTheta;

	//将H从切线空间转换到世界坐标空间
	vec3 up = abs(N.z()) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
	vec3 tangent = unit_vector(cross(up, N));
	vec3 bitangent = cross(N, tangent);

	vec3 sampleVec = tangent * H.x() + bitangent * H.y() + N * H.z();

	return unit_vector(sampleVec);

}

//预计算BRDF
//计算几何遮蔽函数G
static float SchlickGGX_geometry(float n_dot_v, float roughness)
{
	float r = (1 + roughness);
	float k = r * r / 8.0;

	k = roughness * roughness / 2.0f;

	return n_dot_v / (n_dot_v * (1 - k) + k);

}

static float geometry_Smith(float n_dot_v, float n_dot_l, float roughness)
{
	float g1 = SchlickGGX_geometry(n_dot_v, roughness);
	float g2 = SchlickGGX_geometry(n_dot_l, roughness);
	return g1 * g2;
}

//为立方体不同的面设置法线向量
void set_normal_coord(int face_id, int x, int y, float& x_coord, float& y_coord, float& z_coord, float length = 255)
{
	switch (face_id)
	{
	case 0:   //positive x (right face)
		x_coord = 0.5f;
		y_coord = -0.5f + y / length;
		z_coord = -0.5f + x / length;
		break;
	case 1:   //negative x (left face)		
		x_coord = -0.5f;
		y_coord = -0.5f + y / length;
		z_coord = 0.5f - x / length;
		break;
	case 2:   //positive y (top face)
		x_coord = -0.5f + x / length;
		y_coord = 0.5f;
		z_coord = -0.5f + y / length;
		break;
	case 3:   //negative y (bottom face)
		x_coord = -0.5f + x / length;
		y_coord = -0.5f;
		z_coord = 0.5f - y / length;
		break;
	case 4:   //positive z (back face)
		x_coord = 0.5f - x / length;
		y_coord = -0.5f + y / length;
		z_coord = 0.5f;
		break;
	case 5:   //negative z (front face)
		x_coord = -0.5f + x / length;
		y_coord = -0.5f + y / length;
		z_coord = -0.5f;
		break;
	default:
		break;
	}
}


//------------------------镜面反射部分具体可以去看learnOpengl的IBL部分
//求预滤波环境贴图的原理就是，最主要就是获取一个方向，根据这个方向去采样环境贴图
//环境贴图是存储在模型中的，模型通过读取数据可以获取
//然后反向就采用重要性采样获取一个半程向量H，然后每个像素对应的法线又知道
//就可以求入射方向，入射方向就是打到环境贴图的采样方向
//将获取到的radiance再乘上法线和入射方向的余弦值就得到卷积结果
//再把这个卷积结果存到新的图中

void generate_prefilter_map(int thread_id, int face_id, int mip_level, TGAImage& image)
{
	int factor = 1;
	for (int temp = 0;temp < mip_level;temp++)
	{
		factor *= 2;
	}
	//求某一mip级别下的图像尺寸
	int width = 512 / factor;
	int height = 512 / factor;

	if (width < 64)
	{
		width = 64;
	}

	int x, y;
	const char* modelname5[] = {
		"obj/gun/Cerberus.obj",
		"obj/skybox4/box.obj",
	};

	//设置粗糙度系数0-1之间
	float roughness[10];
	for (int i = 0;i < 10;i++)
	{
		roughness[i] = i * (1.0 / 9.0);
	}
	roughness[0] = 0;roughness[9] = 1;

	Model* model[1];
	model[0] = new Model(modelname5[1], 1);

	payload_t p;
	p.model = model[0];

	vec3 prefilter_color(0, 0, 0);
	for (x = 0;x < height;x++)
	{
		for (y = 0;y < width;y++)
		{
			//求出每个像素对应的法线
			float x_coord, y_coord, z_coord;
			set_normal_coord(face_id, x, y, x_coord, y_coord, z_coord, float(width - 1));

			vec3 normal = vec3(x_coord, y_coord, z_coord);
			normal = unit_vector(normal);
			vec3 up = fabs(normal[1]) < 0.999f ? vec3(0.0f, 1.0f, 0.0f) : vec3(0.0f, 0.0f, 1.0f);
			vec3 right = unit_vector(cross(up, normal));
			up = cross(normal, right);

			vec3 r = normal;
			vec3 v = r;//法线方向也是视线方向

			prefilter_color = vec3(0, 0, 0);
			float total_weight = 0.0f;
			int numSamples = 1024;
			//开始随机采样
			for (int i = 0;i < numSamples;i++)
			{
				//先求采样方向
				vec2 Xi = hammersley2d(i, numSamples);
				//得到半程向量
				vec3 h = ImportanceSampleGGX(Xi, normal, roughness[mip_level]);
				//求入射方向
				vec3 l = unit_vector(2 * dot(v, h) * h - v);
				//通过入射方向对环境贴图进行采样
				vec3 radiance = cubemap_sampling(l, p.model->environment_map);
				float n_dot_l = float_max(dot(normal, l), 0.0);
				if (n_dot_l > 0)
				{
					prefilter_color += radiance * n_dot_l;
					total_weight += n_dot_l;
				}

			}

			prefilter_color = prefilter_color / total_weight;

			int red = float_min(prefilter_color.x() * 255.0f, 255);
			int green = float_min(prefilter_color.y() * 255.0f, 255);
			int blue = float_min(prefilter_color.z() * 255.0f, 255);

			TGAColor temp(red, green, blue);
			image.set(x, y, temp);

		}
		printf("%f% \n", x / 512.0f);
	}

}

//diffuse部分，注意辐照度图是立方体贴图，这里只是生成一个面
//生成单个面的辐照度图,计算过程同上，就是没有了mipmap级别和粗糙度，
//另外漫反射过程采用球面均匀采样环境贴图，环境贴图从模型上获取
//如何均匀采样就是将球面坐标均匀增加某个数值，然后范围是整个半球
//超过这个半球就停下；
//将采样得到的向量由球面坐标转换到笛卡尔坐标，再进行切线空间到世界空间的转换得到最后的采样向量
//有了采样向量就能采样环境贴图的color值，再乘上cos和sin值进行一个平衡
//cos用来平衡当角度较大时光比较弱，而sin值则用来平衡当靠近天顶采样时范围变小的贡献
//最后将得到的irradiance累加平均，将结果存储为辐照度图对应每个像素的卷积结果
void generate_irradiance_map(int thread_id, int face_id, TGAImage& image)
{
	int x, y;
	const char* modelname5[] =
	{
		"obj/gun/Cerberus.obj",
		"obj/skybox4/box.obj",
	};

	Model* model[1];
	model[0] = new Model(modelname5[1], 1);

	payload_t p;
	p.model = model[0];

	vec3 irradiance(0, 0, 0);
	for (x = 0;x < 256;x++)
	{
		for (y = 0;y < 256;y++)
		{
			float x_coord, y_coord, z_coord;
			set_normal_coord(face_id, x, y, x_coord, y_coord, z_coord);
			vec3 normal = unit_vector(vec3(x_coord, y_coord, z_coord));
			vec3 up = fabs(normal[1]) < 0.999f ? vec3(0.0f, 1.0f, 0.0f) : vec3(0.0f, 0.0f, 1.0f);
			vec3 right = unit_vector(cross(up, normal));
			up = cross(normal, right);

			irradiance = vec3(0, 0, 0);
			float sampleDelta = 0.025f;
			int numSamples = 0;
			//漫反射采用均匀采样
			for (float phi = 0.0f;phi < 2.0 * PI;phi += sampleDelta)
			{
				for (float theta = 0.0f;theta < 0.5 * PI;theta += sampleDelta)
				{
					//将球面空间坐标转换到笛卡尔坐标
					vec3 tangentSample = vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
					//再从切线空间转到世界空间
					vec3 sampleVec = tangentSample.x() * right + tangentSample.y() * up + tangentSample.z() * 
						normal;
					sampleVec = unit_vector(sampleVec);
					vec3 color = cubemap_sampling(sampleVec, p.model->environment_map);
					//再乘上余弦值得到最后结果
	/*				我们将采样的颜色值乘以系数 cos(θ) ，因为较大角度的光较弱，
					而系数 sin(θ) 则用于权衡较高半球区域的较小采样区域的贡献度。*/
					irradiance += color * sin(theta) * cos(theta);
					numSamples++;
				}
			}

			//求均值
			irradiance = PI * irradiance * (1.0f / numSamples);
			int red = float_min(irradiance.x() * 255.0f, 255);
			int green = float_min(irradiance.y() * 255.0f, 255);
			int blue = float_min(irradiance.z() * 255.0f, 255);

			TGAColor temp(red, green, blue);
			image.set(x, y, temp);
		}
		printf("%f% \n", x / 256.0f);
	}

}

//镜面反射的brdf的积分结果
//brdf积分式推导最后形成跟菲尼尔系数F0有关的比例和偏差，整个式子实际上
//和n和v的夹角以及roughness有关，同样可以利用卷积采样不同的夹角和粗糙度作预计算
//我们将角度 θ 和粗糙度作为输入，以重要性采样产生采样向量，
//在整个几何体上结合 BRDF 的菲涅耳项对向量进行处理，
//然后输出每个样本上 F0 的系数和偏差，最后取平均值。
vec3 IntegrateBRDF(float NdotV, float roughness)
{
	//由于各项同性，随意取一个视角方向V即可
	vec3 V;
	V[0] = 0;
	V[1] = sqrt(1.0 - NdotV * NdotV);
	V[2] = NdotV;

	float A = 0.0;
	float B = 0.0;
	float C = 0.0;

	vec3 N = vec3(0.0, 0.0, 1.0);

	const int SAMPLE_COUNT = 1024;
	for (int i = 0;i < SAMPLE_COUNT;i++)
	{
		vec2 Xi = hammersley2d(i, SAMPLE_COUNT);
		{
			vec3 H = ImportanceSampleGGX(Xi, N, roughness);
			vec3 L = unit_vector(2.0 * dot(V, H) * H - V);

			float NdotL = float_max(L.z(), 0.0);
			float NdotV = float_max(V.z(), 0.0);
			float NdotH = float_max(H.z(), 0.0);
			float VdotH = float_max(dot(V, H), 0.0);

			if (NdotL > 0.0)
			{
				float G = geometry_Smith(NdotV, NdotL, roughness);
				float G_Vis = (G * VdotH) / (NdotH * NdotV);
				float Fc = pow(1.0 - VdotH, 5.0);

				A += (1.0 - Fc) * G_Vis;
				B += Fc * G_Vis;
			}

		}
	}
	return vec3(A, B, C) / float(SAMPLE_COUNT);
}

//将brdf的积分结果存成lut图，取分辨率256x256进行存储

void calculate_BRDF_LUT(TGAImage& image)
{
	int i, j;
	for (i = 0;i < 256;i++)
	{
		for (j = 0;j < 256;j++)
		{
			vec3 color;
			if (i == 0)
			{
				color = IntegrateBRDF(0.002f, j / 256.0f);
			}
			else
			{
				color = IntegrateBRDF(i / 256.0f, j / 256.0f);
			}

			int red = float_min(color.x() * 255.0f, 255);
			int green = float_min(color.y() * 255.0f, 255);
			int blue = float_min(color.z() * 255.0f, 255);

			TGAColor temp(red, green, blue);
			image.set(i, j, temp);
		}
	}
}

//上述的预滤波贴图只是生成某个mipmap级别的，这里需要将所有mipmap级别的图整合
//同时预滤波贴图还是立方体贴图，所以这里要将6个面都要渲染处理
void foreach_prefilter_miplevel(TGAImage& image)
{
	const char* faces[6] = { "px", "nx", "py", "ny", "pz", "nz" };
	char paths[6][256];
	const int thread_num = 4;

	for (int mip_level = 8;mip_level < 10;mip_level++)
	{
		for (int j = 0;j < 6;j++)
		{
			sprintf_s(paths[j], "%s/m%d_%s.tga", "./obj/common2", mip_level, faces[j]);
		}
		int factor = 1;
		for (int temp = 0;temp < mip_level;temp++)
		{
			factor *= 2;
		}
		int w = 512 / factor;
		int h = 512 / factor;

		if (w < 64)
		{
			w = 64;
		}
		if (h < 64)
		{
			h = 64;
		}
		cout << w << h << endl;
		image = TGAImage(w, h, TGAImage::RGB);
		//多线程操作，每个线程生成一张预滤波贴图
		for (int face_id = 0;face_id < 6;face_id++)
		{
			thread thread[thread_num];
			for (int i = 0;i < thread_num;i++)
			{
				thread[i] = std::thread(generate_prefilter_map, i, face_id, mip_level, ref(image));
			}
			for (int i = 0; i< thread_num;i++)
			{
				thread[i].join();
			}
			//将原点放置在图像的左下角
			image.flip_vertically();
			image.write_tga_file(paths[face_id]);

		}

	}

}

//同理，辐照度图是立方体贴图，6个面都要渲染并保存
void foreach_irradiance_map(TGAImage& image)
{
	const char* faces[6] = { "px", "nx", "py", "ny", "pz", "nz" };
	char paths[6][256];
	const int thread_num = 4;


	for (int j = 0; j < 6; j++) {
		sprintf_s(paths[j], "%s/i_%s.tga", "./obj/common2", faces[j]);
	}
	image = TGAImage(256, 256, TGAImage::RGB);
	for (int face_id = 0; face_id < 6; face_id++)
	{
		std::thread thread[thread_num];
		for (int i = 0; i < thread_num; i++)
			thread[i] = std::thread(generate_irradiance_map, i, face_id, std::ref(image));
		for (int i = 0; i < thread_num; i++)
			thread[i].join();



		image.flip_vertically(); // to place the origin in the bottom left corner of the image
		image.write_tga_file(paths[face_id]);
	}

}

