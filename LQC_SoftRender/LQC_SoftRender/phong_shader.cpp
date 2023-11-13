#include "shader.h"
#include "sample.h"

//将法线由切线空间转换为世界坐标空间
//传入的参数中是模型数据的法线
static vec3 cal_normal(vec3& normal, vec3* world_coords, 
	const vec2* uvs, const vec2& uv, TGAImage* normal_map)
{
	//计算纹理坐标之间的差值
	float x1 = uvs[1][0] - uvs[0][0];
	float y1 = uvs[1][1] - uvs[0][1];
	float x2 = uvs[2][0] - uvs[0][0];
	float y2 = uvs[2][1] - uvs[0][1];
	float det = (x1 * y2 - x2 * y1);

	//计算世界坐标之间的差值
	vec3 e1 = world_coords[1] - world_coords[0];
	vec3 e2 = world_coords[2] - world_coords[0];

	//通过插值计算tbn空间，需要将法线由切线空间转换到世界空间
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

//将从模型中读取到的顶点数据传给结构体便于后续计算
void PhongShader::vertex_shader(int nfaces, int nvertex)
{
	vec4 temp_vert = to_vec4(payload.model->vert(nfaces, nvertex), 1.0f);
	vec4 temp_normal = to_vec4(payload.model->normal(nfaces, nvertex), 1.0f);

	payload.uv_attri[nvertex] = payload.model->uv(nfaces, nvertex);
	payload.in_uv[nvertex] = payload.uv_attri[nvertex];
	payload.clipcoord_attri[nvertex] = payload.mvp_matrix * temp_vert;
	payload.in_clipcoord[nvertex] = payload.clipcoord_attri[nvertex];

	for (int i = 0;i < 3;i++)
	{
		payload.worldcoord_attri[nvertex][i] = temp_vert[i];
		payload.in_worldcoord[nvertex][i] = temp_vert[i];
		payload.normal_attri[nvertex][i] = temp_normal[i];
		payload.in_normal[nvertex][i] = temp_normal[i];
	}
}

vec3 PhongShader::fragment_shader(float alpha, float beta, float gamma)
{
	vec4* clip_coords = payload.clipcoord_attri;
	vec3* world_coords = payload.worldcoord_attri;
	vec3* normals = payload.normal_attri;
	vec2* uvs = payload.uv_attri;

	//插值计算包括透视校正插值
	float Z = 1.0 / (alpha / clip_coords[0].w() + beta / clip_coords[1].w() + gamma / clip_coords[2].w());
	vec3 normal = (alpha * normals[0] / clip_coords[0].w() + beta * normals[1] / clip_coords[1].w()
		+ gamma * normals[2] / clip_coords[2].w()) * Z;
	vec2 uv = (alpha * uvs[0] / clip_coords[0].w() + beta * uvs[1] / clip_coords[1].w() +
		gamma * uvs[2] / clip_coords[2].w()) * Z;
	vec3 world_pos = (alpha * world_coords[0] / clip_coords[0].w() + beta * world_coords[1]
		/ clip_coords[1].w() + gamma * world_coords[2] / clip_coords[2].w()) * Z;

	//这个法线是局部坐标，还不是世界坐标
	//插值计算的法线还需要经过切线空间到世界空间的变幻
	if (payload.model->normalmap)
	{
		normal = cal_normal(normal, world_coords, uvs, uv, payload.model->normalmap);
	}

	//定义ka、ks、kd等参数
	vec3 ka(0.35, 0.35, 0.35);
	vec3 kd = payload.model->diffuse(uv);
	vec3 ks(0.8, 0.8, 0.8);

	float p = 150.0;
	vec3 l = unit_vector(vec3(1, 1, 1));
	vec3 light_ambient_intensity = kd;
	vec3 light_diffuse_intensity = vec3(0.9, 0.9, 0.9);
	vec3 light_specular_intensity = vec3(0.15, 0.15, 0.15);

	vec3 result_color(0, 0, 0);
	vec3 ambient, diffuse, specular;
	normal = unit_vector(normal);
	vec3 v = unit_vector(payload.camera->eye - world_pos);
	vec3 h = unit_vector(l + v);

	ambient = cwise_product(ka, light_ambient_intensity);
	diffuse = cwise_product(kd, light_diffuse_intensity) * float_max(dot(l, normal), 0);
	specular = cwise_product(ka, light_specular_intensity) * float_max(0, pow(dot(h, normal), p));

	result_color = ambient + diffuse + specular;

	return result_color * 255.f;
}