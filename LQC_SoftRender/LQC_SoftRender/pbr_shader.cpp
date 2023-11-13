#include "shader.h"
#include "tgaimage.h"
#include "sample.h"
#include <cmath>



//法线分布函数
static float GGX_distribution(float n_dot_h, float roughness)
{
	float alpha = roughness * roughness;
	float alpha2 = alpha * alpha;

	float n_dot_h2 = n_dot_h * n_dot_h;
	float factor = n_dot_h2 * (alpha2 - 1) + 1;

	return alpha2 / (PI * factor * factor);

}

static float SchlickGGX_geometry(float n_dot_v, float roughness)
{
	float r = (1 + roughness);
	float k = r * r / 8.0;

	return n_dot_v / (n_dot_v * (1 - k) + k);

}

static float SchlickGGX_geometry_ibl(float n_dot_v, float roughness)
{
	float r = roughness;
	float k = r * r / 2.0;

	return n_dot_v / (n_dot_v * (1 - k) + k);

}

//几何遮挡计算的是物体宏观表面的影响，所以是宏观法向
static float geometry_Smith(float n_dot_v, float n_dot_l, float roughness)
{
	float g1 = SchlickGGX_geometry(n_dot_v, roughness);
	float g2 = SchlickGGX_geometry(n_dot_l, roughness);

	return g1 * g2;
}


//菲涅尔项计算的是反射光占比，它跟视线和平面法线夹角有关
//视线越靠近掠角（夹角90度），反射光越强，F0为基础反射率

static vec3 fresenlschlick(float h_dot_v, vec3& f0)
{
	return f0 + (vec3(1.0, 1.0, 1.0) - f0) * pow(1 - h_dot_v, 5.0);
}

vec3 fresenlschlick_roughness(float h_dot_v, vec3& f0, float roughness)
{
	float r1 = 1.0f - roughness;
	if (r1 < f0[0])
	{
		r1 = f0[0];
	}

	return  f0 + (vec3(r1, r1, r1) - f0) * pow(1 - h_dot_v, 5.0f);
}

//Reinhard_mapping 是一种用于图像色彩映射的算法，用于将一个图像的颜色映射到另一个图像上。
//该算法主要针对不同曝光条件下的图像进行颜色校正，使得它们具有相似的色调和对比度。

//Reinhard_mapping 算法的原理是基于对图像的灰度值进行统计分析，然后根据这些统计数据对图像进行调整。具体而言，该算法通过将输入图
//像中的每个像素映射到输出图像中的对应像素，并根据亮度值进行调整，以实现颜色的校正。

//ACES tone mapping算法和伽马矫正
//用来解决显示器LDR和HDR冲突的问题，
//因为现在大部分大型的游戏或者一些影视作品都是使用HDR渲染，也就是高动态范围。
//但是显示器有的还是LDR的，如果不进行一个色域映射的话，
//就会使得渲染出来的结果要么过亮要么过暗。

//在函数中，给定一个浮点数值 value，该函数通过将该值代入预定义的一些常量 (a、b、c、d、e)，
//并进行一些简单的数学计算来计算映射后的值。最终结果通过调用 float_clamp 函数来确保它在 0 到 1 之间。
static float float_aces(float value)
{
	float a = 2.51f;
	float b = 0.03f;
	float c = 2.43f;
	float d = 0.59f;
	float e = 0.14f;
	value = (value * (a * value + b)) / (value * (c * value + d) + e);
	return float_clamp(value, 0, 1);
}

//gamma矫正
//gamma矫正是因为人眼睛成像和显像管显示的特性问题，
//使得显示出来的画面是在一个非线性空间，需要进行gamma矫正后，到达线性空间，
//更接近人眼看到的这个世界的情况。给颜色取一个幂运算，0.45或者是1 / 2.2次幂。
static vec3 Reinhard_mapping(vec3& color)
{
	int i;
	for (i = 0;i < 3;i++)
	{
		color[i] = float_aces(color[i]);//先进行色域调整，解决过亮或过暗问题
		color[i] = pow(color[i], 1.0 / 2.2);
	}

	return color;
}

static vec3 cal_normal(vec3& normal, vec3* world_coords, const vec2* uvs, 
	const vec2& uv, TGAImage* normal_map)
{
	float x1 = uvs[1][0] - uvs[0][0];
	float y1 = uvs[1][1] - uvs[0][1];
	float x2 = uvs[2][0] - uvs[0][0];
	float y2 = uvs[2][1] - uvs[0][1];
	float det(x1 * y1 - x2 * y2);

	vec3 e1 = world_coords[1] - world_coords[0];
	vec3 e2 = world_coords[2] - world_coords[0];

	vec3 t = e1 * y2 + e2 * (-y1);
	vec3 b = e1 * (-x2) + e2 * x1;
	t /= det;
	b /= det;

	normal = unit_vector(normal);
	t = unit_vector(t - dot(t, normal) * normal);
	b = unit_vector(b - dot(b, normal) * normal - dot(b, t) * t);

	vec3 sample = texture_sample(uv, normal_map);

	sample = vec3(sample[0] * 2 - 1, sample[1] * 2 - 1, sample[2] * 2 - 1);

	vec3 normal_new = t * sample[0] + b * sample[1] + normal * sample[2];

	return normal_new;
}

//顶点坐标的作用主要是处理几何变换，并把变换后的属性存储到结构体中，以便片段着色器使用
//存储的属性就包括世界坐标、裁剪坐标、法线、纹理坐标
void PBRShader::vertex_shader(int nfaces, int nvertex)
{
	int i = 0;
	vec4 temp_vert = to_vec4(payload.model->vert(nfaces, nvertex), 1.0f);
	vec4 temp_normal = to_vec4(payload.model->normal(nfaces, nvertex), 1.0f);

	payload.uv_attri[nvertex] = payload.model->uv(nfaces, nvertex);
	payload.in_uv[nvertex] = payload.uv_attri[nvertex];
	payload.clipcoord_attri[nvertex] = payload.mvp_matrix * temp_vert;
	payload.in_clipcoord[nvertex] = payload.clipcoord_attri[nvertex];

	//为什么这里不直接赋值，因为temp_vert是思维，而worldcoord_attri是三维
	for (int i = 0;i < 3;i++)
	{
		payload.worldcoord_attri[nvertex][i] = temp_vert[i];
		payload.in_worldcoord[nvertex][i] = temp_vert[i];
		payload.normal_attri[nvertex][i] = temp_normal[i];
		payload.in_normal[nvertex][i] = temp_normal[i];

	}
}

//PBR分为直接光照部分和间接光照部分，这部分是直接光照，间接光照使用IBL算法
//直接光照部分的Li和夹角余弦值都是固定，即radiance和n_dot_l
vec3 PBRShader::direct_fragment_shader(float alpha, float beta, float gamma)
{
	vec3 CookTorrance_brdf;
	vec3 light_pos = vec3(2, 1.5, 5);
	vec3 radiance = vec3(3, 3, 3);

	vec4* clip_coords = payload.clipcoord_attri;
	vec3* world_coords = payload.worldcoord_attri;
	vec3* normals = payload.normal_attri;
	vec2* uvs = payload.uv_attri;

	float Z = 1.0 / (alpha * clip_coords[0].w() + beta * clip_coords[1].w() + gamma * clip_coords[2].w());
	vec3 normal = (alpha * normals[0] / clip_coords[0].w() + beta * normals[1] / clip_coords[1].w()
		+ gamma * normals[2] / clip_coords[2].w())*Z;
	vec2 uv = (alpha * uvs[0] / clip_coords[0].w() + beta * uvs[1] / clip_coords[1].w()
		+ gamma * uvs[2] / clip_coords[2].w()) * Z;
	vec3 worldpos = (alpha * world_coords[0] / clip_coords[0].w() + beta * world_coords[1] / clip_coords[1].w()
		+ gamma * world_coords[2] / clip_coords[2].w()) * Z;

	vec3 l = unit_vector(light_pos - worldpos);
	vec3 n = unit_vector(normal);
	vec3 v = unit_vector(payload.camera->eye - worldpos);
	vec3 h = unit_vector(l + v);

	float n_dot_l = float_max(dot(n, l), 0);
	vec3 color(0, 0, 0);
	if (n_dot_l > 0)
	{
		float n_dot_v = float_max(dot(n, v), 0);
		float n_dot_h = float_max(dot(n, h), 0);
		float h_dot_v = float_max(dot(h, v), 0);

		float roughness = payload.model->roughness(uv);
		float metalness = payload.model->metalness(uv);

		float NDF = GGX_distribution(n_dot_h, roughness);
		float G = geometry_Smith(n_dot_v, n_dot_l, roughness);
		
		vec3 albedo = payload.model->diffuse(uv);
		vec3 temp = vec3(0.04, 0.04, 0.04);
		vec3 f0 = vec3_lerp(temp, albedo, metalness);

		vec3 F = fresenlschlick(h_dot_v, f0);
		vec3 kD = (vec3(1.0, 1.0, 1.0) - F) * (1 - metalness);

		CookTorrance_brdf = NDF*G*F  / (4 * n_dot_l * n_dot_v + 0.0001);//防止分母为0+一个极小数

		vec3 Lo = (kD * albedo / PI + CookTorrance_brdf) * radiance * n_dot_l;

		vec3 ambient = 0.05 * albedo;
		color = Lo + ambient;//基于物理的反射+环境光

		//再对最后的输出颜色进行一个acesmapping处理和gamma校正
		Reinhard_mapping(color);

	}

	return color * 255.f;
}


//PBR的间接光照部分，使用IBL算法,Li这块是和直接光照区别最大的
//或者说整个方程分为漫反射部分和镜面反射部分
//PBR的间接光照 = 漫反射光+镜面反射光+自发光
vec3 PBRShader::fragment_shader(float alpha, float beta, float gamma)
{
	vec3 CookTorrance_brdf;
	vec3 light_pos = vec3(2, 1.5, 5);
	vec3 radiance = vec3(3, 3, 3);

	vec4* clip_coords = payload.clipcoord_attri;
	vec3* world_coords = payload.worldcoord_attri;
	vec3* normals = payload.normal_attri;
	vec2* uvs = payload.uv_attri;

	//透视校正插值
	float Z = 1.0 / (alpha / clip_coords[0].w() + beta / clip_coords[1].w() + gamma / clip_coords[2].w());
	vec3 normal = (alpha * normals[0] / clip_coords[0].w() + beta * normals[1] / clip_coords[1].w() +
		gamma * normals[2] / clip_coords[2].w()) * Z;
	vec2 uv = (alpha * uvs[0] / clip_coords[0].w() + beta * uvs[1] / clip_coords[1].w() +
		gamma * uvs[2] / clip_coords[2].w()) * Z;
	vec3 worldpos = (alpha * world_coords[0] / clip_coords[0].w() + beta * world_coords[1] / clip_coords[1].w() +
		gamma * world_coords[2] / clip_coords[2].w()) * Z;

	//如果模型存在法线贴图，就从法线贴图中获取法线
	if (payload.model->normalmap)
	{
		normal = cal_normal(normal, world_coords, uvs, uv,payload.model->normalmap);
	}

	//归一化
	vec3 n = unit_vector(normal);
	vec3 v = unit_vector(payload.camera->eye - worldpos);
	float n_dot_v = float_max(dot(n, v), 0);
	vec3 l = unit_vector(light_pos - worldpos);
	float n_dot_l = float_max(dot(n, l), 0);

	vec3 color(0.0f, 0.0f, 0.0f);
	if (n_dot_l > 0)
	{
		float roughness = payload.model->roughness(uv);
		float metalness = payload.model->metalness(uv);//金属度是为了就漫反射系数kd用，以及基础反射率
		float occlusion = payload.model->occlusion(uv);
		vec3 emission = payload.model->emission(uv);

		//计算漫反射系数kd
		vec3 albedo = payload.model->diffuse(uv);//漫反射贴图的颜色值
		vec3 temp = vec3(0.4, 0.4, 0.4);//这里定义一个最低的金属度
		//基础反射率f0通过temp、albedo之间插值，插值系数就是金属度
		/*这样就可以根据metalness的权重，平滑地过渡基础反射率的值。
		当metalness接近0时，插值结果更接近temp；当metalness接近1时，
		插值结果更接近albedo。*/
		vec3 f0 = vec3_lerp(temp, albedo, metalness);
		vec3 kD = (vec3(1.0, 1.0, 1.0) - f0) * (1 - (double)metalness);//后面是表示当金属度为1时，漫反射系数为0，就是没有折射，全是反射

		//漫反射部分：Li * kD * lambrtBRDF
		//先求Li，直接从辐照度图采样
		cubemap_t* irradiance_map = payload.iblmap->irradiance_map;
		//因为漫反射各个方向都一样，所以就用n作为采样方向
		vec3 irradiance = cubemap_sampling(n, irradiance_map);

		//对环境光照进行平方处理，即色彩映射，即每个分量都要平方
		for (int i = 0;i < 3;i++)
		{
			irradiance[i] = pow(irradiance[i], 2.0f);
		}

		vec3 diffuse = irradiance * kD * albedo / PI;

		//--------------------求镜面反射光部分
		// Li * BRDF
		// Li部分通过预计算预滤波环境贴图求解
		//BRDF部分根据分割求和近似法将这部分最终拆分成
		//f0*A+B,A为f0的比例，B为偏差，A和B为LUT采样结果的x和y值

		//首先采样BRDF
		//LUT的纹理坐标由夹角和粗糙度组成
		vec2 lut_uv = vec2(n_dot_v, roughness);
		vec3 lut_sample = texture_sample(lut_uv, payload.iblmap->brdf_lut);
		float specular_scale = lut_sample.x();
		float specular_bias = lut_sample.y();

		vec3 specular = f0 * specular_scale + vec3(specular_bias, specular_bias, specular_bias);

		//接着采样Li
		//确定mipmap最多多少层
		float max_mip_level = (float)(payload.iblmap->mip_levels - 1);

		//根据粗糙度定位在哪一层mipmap采样，直接套公式
		int specular_miplevel = (int)(roughness * max_mip_level + 0.5f);

		//确定采样方向，由视角方向和和法线方向决定
		vec3 r = unit_vector(2.0 * dot(n,v) * n - v);

		vec3 prefilter_color = cubemap_sampling(r, payload.iblmap->prefilter_maps[specular_miplevel]);

		for (int i = 0;i < 3;i++)
		{
			prefilter_color[i] = pow(prefilter_color[i], 2.0f);
		}

		specular = cwise_product(prefilter_color, specular);


		//最终结果
		color = (diffuse + specular) + emission;
	}

	//进行色域映射，最后是要在LDR屏幕上展示,然后再进行gamma校正
	Reinhard_mapping(color);

	return  color * 255.f;
}
