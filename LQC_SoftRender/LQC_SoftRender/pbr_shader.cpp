#include "shader.h"
#include "tgaimage.h"
#include "sample.h"
#include <cmath>



//���߷ֲ�����
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

//�����ڵ�������������۱����Ӱ�죬�����Ǻ�۷���
static float geometry_Smith(float n_dot_v, float n_dot_l, float roughness)
{
	float g1 = SchlickGGX_geometry(n_dot_v, roughness);
	float g2 = SchlickGGX_geometry(n_dot_l, roughness);

	return g1 * g2;
}


//�������������Ƿ����ռ�ȣ��������ߺ�ƽ�淨�߼н��й�
//����Խ�����ӽǣ��н�90�ȣ��������Խǿ��F0Ϊ����������

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

//Reinhard_mapping ��һ������ͼ��ɫ��ӳ����㷨�����ڽ�һ��ͼ�����ɫӳ�䵽��һ��ͼ���ϡ�
//���㷨��Ҫ��Բ�ͬ�ع������µ�ͼ�������ɫУ����ʹ�����Ǿ������Ƶ�ɫ���ͶԱȶȡ�

//Reinhard_mapping �㷨��ԭ���ǻ��ڶ�ͼ��ĻҶ�ֵ����ͳ�Ʒ�����Ȼ�������Щͳ�����ݶ�ͼ����е�����������ԣ����㷨ͨ��������ͼ
//���е�ÿ������ӳ�䵽���ͼ���еĶ�Ӧ���أ�����������ֵ���е�������ʵ����ɫ��У����

//ACES tone mapping�㷨��٤�����
//���������ʾ��LDR��HDR��ͻ�����⣬
//��Ϊ���ڴ󲿷ִ��͵���Ϸ����һЩӰ����Ʒ����ʹ��HDR��Ⱦ��Ҳ���Ǹ߶�̬��Χ��
//������ʾ���еĻ���LDR�ģ����������һ��ɫ��ӳ��Ļ���
//�ͻ�ʹ����Ⱦ�����Ľ��Ҫô����Ҫô������

//�ں����У�����һ��������ֵ value���ú���ͨ������ֵ����Ԥ�����һЩ���� (a��b��c��d��e)��
//������һЩ�򵥵���ѧ����������ӳ����ֵ�����ս��ͨ������ float_clamp ������ȷ������ 0 �� 1 ֮�䡣
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

//gamma����
//gamma��������Ϊ���۾�������������ʾ���������⣬
//ʹ����ʾ�����Ļ�������һ�������Կռ䣬��Ҫ����gamma�����󣬵������Կռ䣬
//���ӽ����ۿ����������������������ɫȡһ�������㣬0.45������1 / 2.2���ݡ�
static vec3 Reinhard_mapping(vec3& color)
{
	int i;
	for (i = 0;i < 3;i++)
	{
		color[i] = float_aces(color[i]);//�Ƚ���ɫ�����������������������
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

//���������������Ҫ�Ǵ����α任�����ѱ任������Դ洢���ṹ���У��Ա�Ƭ����ɫ��ʹ��
//�洢�����ԾͰ����������ꡢ�ü����ꡢ���ߡ���������
void PBRShader::vertex_shader(int nfaces, int nvertex)
{
	int i = 0;
	vec4 temp_vert = to_vec4(payload.model->vert(nfaces, nvertex), 1.0f);
	vec4 temp_normal = to_vec4(payload.model->normal(nfaces, nvertex), 1.0f);

	payload.uv_attri[nvertex] = payload.model->uv(nfaces, nvertex);
	payload.in_uv[nvertex] = payload.uv_attri[nvertex];
	payload.clipcoord_attri[nvertex] = payload.mvp_matrix * temp_vert;
	payload.in_clipcoord[nvertex] = payload.clipcoord_attri[nvertex];

	//Ϊʲô���ﲻֱ�Ӹ�ֵ����Ϊtemp_vert��˼ά����worldcoord_attri����ά
	for (int i = 0;i < 3;i++)
	{
		payload.worldcoord_attri[nvertex][i] = temp_vert[i];
		payload.in_worldcoord[nvertex][i] = temp_vert[i];
		payload.normal_attri[nvertex][i] = temp_normal[i];
		payload.in_normal[nvertex][i] = temp_normal[i];

	}
}

//PBR��Ϊֱ�ӹ��ղ��ֺͼ�ӹ��ղ��֣��ⲿ����ֱ�ӹ��գ���ӹ���ʹ��IBL�㷨
//ֱ�ӹ��ղ��ֵ�Li�ͼн�����ֵ���ǹ̶�����radiance��n_dot_l
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

		CookTorrance_brdf = NDF*G*F  / (4 * n_dot_l * n_dot_v + 0.0001);//��ֹ��ĸΪ0+һ����С��

		vec3 Lo = (kD * albedo / PI + CookTorrance_brdf) * radiance * n_dot_l;

		vec3 ambient = 0.05 * albedo;
		color = Lo + ambient;//��������ķ���+������

		//�ٶ����������ɫ����һ��acesmapping�����gammaУ��
		Reinhard_mapping(color);

	}

	return color * 255.f;
}


//PBR�ļ�ӹ��ղ��֣�ʹ��IBL�㷨,Li����Ǻ�ֱ�ӹ�����������
//����˵�������̷�Ϊ�����䲿�ֺ;��淴�䲿��
//PBR�ļ�ӹ��� = �������+���淴���+�Է���
vec3 PBRShader::fragment_shader(float alpha, float beta, float gamma)
{
	vec3 CookTorrance_brdf;
	vec3 light_pos = vec3(2, 1.5, 5);
	vec3 radiance = vec3(3, 3, 3);

	vec4* clip_coords = payload.clipcoord_attri;
	vec3* world_coords = payload.worldcoord_attri;
	vec3* normals = payload.normal_attri;
	vec2* uvs = payload.uv_attri;

	//͸��У����ֵ
	float Z = 1.0 / (alpha / clip_coords[0].w() + beta / clip_coords[1].w() + gamma / clip_coords[2].w());
	vec3 normal = (alpha * normals[0] / clip_coords[0].w() + beta * normals[1] / clip_coords[1].w() +
		gamma * normals[2] / clip_coords[2].w()) * Z;
	vec2 uv = (alpha * uvs[0] / clip_coords[0].w() + beta * uvs[1] / clip_coords[1].w() +
		gamma * uvs[2] / clip_coords[2].w()) * Z;
	vec3 worldpos = (alpha * world_coords[0] / clip_coords[0].w() + beta * world_coords[1] / clip_coords[1].w() +
		gamma * world_coords[2] / clip_coords[2].w()) * Z;

	//���ģ�ʹ��ڷ�����ͼ���ʹӷ�����ͼ�л�ȡ����
	if (payload.model->normalmap)
	{
		normal = cal_normal(normal, world_coords, uvs, uv,payload.model->normalmap);
	}

	//��һ��
	vec3 n = unit_vector(normal);
	vec3 v = unit_vector(payload.camera->eye - worldpos);
	float n_dot_v = float_max(dot(n, v), 0);
	vec3 l = unit_vector(light_pos - worldpos);
	float n_dot_l = float_max(dot(n, l), 0);

	vec3 color(0.0f, 0.0f, 0.0f);
	if (n_dot_l > 0)
	{
		float roughness = payload.model->roughness(uv);
		float metalness = payload.model->metalness(uv);//��������Ϊ�˾�������ϵ��kd�ã��Լ�����������
		float occlusion = payload.model->occlusion(uv);
		vec3 emission = payload.model->emission(uv);

		//����������ϵ��kd
		vec3 albedo = payload.model->diffuse(uv);//��������ͼ����ɫֵ
		vec3 temp = vec3(0.4, 0.4, 0.4);//���ﶨ��һ����͵Ľ�����
		//����������f0ͨ��temp��albedo֮���ֵ����ֵϵ�����ǽ�����
		/*�����Ϳ��Ը���metalness��Ȩ�أ�ƽ���ع��ɻ��������ʵ�ֵ��
		��metalness�ӽ�0ʱ����ֵ������ӽ�temp����metalness�ӽ�1ʱ��
		��ֵ������ӽ�albedo��*/
		vec3 f0 = vec3_lerp(temp, albedo, metalness);
		vec3 kD = (vec3(1.0, 1.0, 1.0) - f0) * (1 - (double)metalness);//�����Ǳ�ʾ��������Ϊ1ʱ��������ϵ��Ϊ0������û�����䣬ȫ�Ƿ���

		//�����䲿�֣�Li * kD * lambrtBRDF
		//����Li��ֱ�Ӵӷ��ն�ͼ����
		cubemap_t* irradiance_map = payload.iblmap->irradiance_map;
		//��Ϊ�������������һ�������Ծ���n��Ϊ��������
		vec3 irradiance = cubemap_sampling(n, irradiance_map);

		//�Ի������ս���ƽ��������ɫ��ӳ�䣬��ÿ��������Ҫƽ��
		for (int i = 0;i < 3;i++)
		{
			irradiance[i] = pow(irradiance[i], 2.0f);
		}

		vec3 diffuse = irradiance * kD * albedo / PI;

		//--------------------���淴��ⲿ��
		// Li * BRDF
		// Li����ͨ��Ԥ����Ԥ�˲�������ͼ���
		//BRDF���ָ��ݷָ���ͽ��Ʒ����ⲿ�����ղ�ֳ�
		//f0*A+B,AΪf0�ı�����BΪƫ�A��BΪLUT���������x��yֵ

		//���Ȳ���BRDF
		//LUT�����������ɼнǺʹֲڶ����
		vec2 lut_uv = vec2(n_dot_v, roughness);
		vec3 lut_sample = texture_sample(lut_uv, payload.iblmap->brdf_lut);
		float specular_scale = lut_sample.x();
		float specular_bias = lut_sample.y();

		vec3 specular = f0 * specular_scale + vec3(specular_bias, specular_bias, specular_bias);

		//���Ų���Li
		//ȷ��mipmap�����ٲ�
		float max_mip_level = (float)(payload.iblmap->mip_levels - 1);

		//���ݴֲڶȶ�λ����һ��mipmap������ֱ���׹�ʽ
		int specular_miplevel = (int)(roughness * max_mip_level + 0.5f);

		//ȷ�������������ӽǷ���ͺͷ��߷������
		vec3 r = unit_vector(2.0 * dot(n,v) * n - v);

		vec3 prefilter_color = cubemap_sampling(r, payload.iblmap->prefilter_maps[specular_miplevel]);

		for (int i = 0;i < 3;i++)
		{
			prefilter_color[i] = pow(prefilter_color[i], 2.0f);
		}

		specular = cwise_product(prefilter_color, specular);


		//���ս��
		color = (diffuse + specular) + emission;
	}

	//����ɫ��ӳ�䣬�����Ҫ��LDR��Ļ��չʾ,Ȼ���ٽ���gammaУ��
	Reinhard_mapping(color);

	return  color * 255.f;
}
