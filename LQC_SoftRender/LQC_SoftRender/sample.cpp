#include "sample.h"
#include <stdlib.h>
#include <thread>

using namespace std;
 
//�������������ǲ��������������ͼ�е��ĸ��棬ͬʱ����������Ӧ����������
//ԭ���������������6���涼�и��Զ�Ӧ�ı��0-5����һ�����Ǹ��ݷ����xyz��С����֪�������ĸ���
//�����xֵ�����������������ֵ��˵�������ǳ���x�᷽���ٸ����������жϻ��е���
//����ȷ�����������������
//�������Ҫ����������귶Χ��0-1֮��
//������x�����ֵ�����ô��Ҫ��yz�����ֵ����xת����-1-1
//Ȼ�������������������-1-1֮�䣬���Ի���Ҫ+1�ٳ�2

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
	//��uv�����������ȡģ���㣬�޶���Χ��0-1֮��
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
//Ԥ�������ͼ��Ĺ���
//�Ͳ�������������Ҫ�Բ������÷������и���������ٶȣ�������ֲ�������
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

//������Ȳ���
vec3 hemisphereSample_uniform(float u, float v)
{
	float phi = 2.0f * v * PI;
	float cosTheta = 1.0f - u;//����������z������
	float sinTheta = sqrt(1.0f - cosTheta * cosTheta);//�ٶ�һ������Ƕ�
	return vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
}

vec3 hemisphereSample_cos(float u, float v)
{
	float phi = v * 2.0 * PI;
	float cosTheta = sqrt(1.0 - u);
	float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
	return vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
}

//��Ҫ�Բ���������GGX���������GGX����PBR�еķ��߷ֲ���������ʾ����
vec3 ImportanceSampleGGX(vec2 Xi, vec3 N, float roughness)
{
	//Xi���ɵͲ����������ɵ�������������������ת��ΪH
	float a = roughness * roughness;

	//Ϊ�˽��������ת����H�ĵѿ�����������Ԥ�ȼ���
	float phi = 2.0 * PI * Xi.x();
	float cosTheta = sqrt((1.0 - Xi.y())) / (1.0 + (a * a - 1.0) * Xi.y());
	float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

	vec3 H;
	H[0] = cos(phi) * sinTheta;
	H[1] = sin(phi) * sinTheta;
	H[2] = cosTheta;

	//��H�����߿ռ�ת������������ռ�
	vec3 up = abs(N.z()) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
	vec3 tangent = unit_vector(cross(up, N));
	vec3 bitangent = cross(N, tangent);

	vec3 sampleVec = tangent * H.x() + bitangent * H.y() + N * H.z();

	return unit_vector(sampleVec);

}

//Ԥ����BRDF
//���㼸���ڱκ���G
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

//Ϊ�����岻ͬ�������÷�������
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


//------------------------���淴�䲿�־������ȥ��learnOpengl��IBL����
//��Ԥ�˲�������ͼ��ԭ����ǣ�����Ҫ���ǻ�ȡһ�����򣬸����������ȥ����������ͼ
//������ͼ�Ǵ洢��ģ���еģ�ģ��ͨ����ȡ���ݿ��Ի�ȡ
//Ȼ����Ͳ�����Ҫ�Բ�����ȡһ���������H��Ȼ��ÿ�����ض�Ӧ�ķ�����֪��
//�Ϳ��������䷽�����䷽����Ǵ򵽻�����ͼ�Ĳ�������
//����ȡ����radiance�ٳ��Ϸ��ߺ����䷽�������ֵ�͵õ�������
//�ٰ�����������浽�µ�ͼ��

void generate_prefilter_map(int thread_id, int face_id, int mip_level, TGAImage& image)
{
	int factor = 1;
	for (int temp = 0;temp < mip_level;temp++)
	{
		factor *= 2;
	}
	//��ĳһmip�����µ�ͼ��ߴ�
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

	//���ôֲڶ�ϵ��0-1֮��
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
			//���ÿ�����ض�Ӧ�ķ���
			float x_coord, y_coord, z_coord;
			set_normal_coord(face_id, x, y, x_coord, y_coord, z_coord, float(width - 1));

			vec3 normal = vec3(x_coord, y_coord, z_coord);
			normal = unit_vector(normal);
			vec3 up = fabs(normal[1]) < 0.999f ? vec3(0.0f, 1.0f, 0.0f) : vec3(0.0f, 0.0f, 1.0f);
			vec3 right = unit_vector(cross(up, normal));
			up = cross(normal, right);

			vec3 r = normal;
			vec3 v = r;//���߷���Ҳ�����߷���

			prefilter_color = vec3(0, 0, 0);
			float total_weight = 0.0f;
			int numSamples = 1024;
			//��ʼ�������
			for (int i = 0;i < numSamples;i++)
			{
				//�����������
				vec2 Xi = hammersley2d(i, numSamples);
				//�õ��������
				vec3 h = ImportanceSampleGGX(Xi, normal, roughness[mip_level]);
				//�����䷽��
				vec3 l = unit_vector(2 * dot(v, h) * h - v);
				//ͨ�����䷽��Ի�����ͼ���в���
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

//diffuse���֣�ע����ն�ͼ����������ͼ������ֻ������һ����
//���ɵ�����ķ��ն�ͼ,�������ͬ�ϣ�����û����mipmap����ʹֲڶȣ�
//������������̲���������Ȳ���������ͼ��������ͼ��ģ���ϻ�ȡ
//��ξ��Ȳ������ǽ����������������ĳ����ֵ��Ȼ��Χ����������
//������������ͣ�£�
//�������õ�����������������ת�����ѿ������꣬�ٽ������߿ռ䵽����ռ��ת���õ����Ĳ�������
//���˲����������ܲ���������ͼ��colorֵ���ٳ���cos��sinֵ����һ��ƽ��
//cos����ƽ�⵱�ǶȽϴ�ʱ��Ƚ�������sinֵ������ƽ�⵱�����춥����ʱ��Χ��С�Ĺ���
//��󽫵õ���irradiance�ۼ�ƽ����������洢Ϊ���ն�ͼ��Ӧÿ�����صľ�����
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
			//��������þ��Ȳ���
			for (float phi = 0.0f;phi < 2.0 * PI;phi += sampleDelta)
			{
				for (float theta = 0.0f;theta < 0.5 * PI;theta += sampleDelta)
				{
					//������ռ�����ת�����ѿ�������
					vec3 tangentSample = vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
					//�ٴ����߿ռ�ת������ռ�
					vec3 sampleVec = tangentSample.x() * right + tangentSample.y() * up + tangentSample.z() * 
						normal;
					sampleVec = unit_vector(sampleVec);
					vec3 color = cubemap_sampling(sampleVec, p.model->environment_map);
					//�ٳ�������ֵ�õ������
	/*				���ǽ���������ɫֵ����ϵ�� cos(��) ����Ϊ�ϴ�ǶȵĹ������
					��ϵ�� sin(��) ������Ȩ��ϸ߰�������Ľ�С��������Ĺ��׶ȡ�*/
					irradiance += color * sin(theta) * cos(theta);
					numSamples++;
				}
			}

			//���ֵ
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

//���淴���brdf�Ļ��ֽ��
//brdf����ʽ�Ƶ�����γɸ������ϵ��F0�йصı�����ƫ�����ʽ��ʵ����
//��n��v�ļн��Լ�roughness�йأ�ͬ���������þ��������ͬ�ļнǺʹֲڶ���Ԥ����
//���ǽ��Ƕ� �� �ʹֲڶ���Ϊ���룬����Ҫ�Բ�����������������
//�������������Ͻ�� BRDF �ķ���������������д���
//Ȼ�����ÿ�������� F0 ��ϵ����ƫ����ȡƽ��ֵ��
vec3 IntegrateBRDF(float NdotV, float roughness)
{
	//���ڸ���ͬ�ԣ�����ȡһ���ӽǷ���V����
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

//��brdf�Ļ��ֽ�����lutͼ��ȡ�ֱ���256x256���д洢

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

//������Ԥ�˲���ͼֻ������ĳ��mipmap����ģ�������Ҫ������mipmap�����ͼ����
//ͬʱԤ�˲���ͼ������������ͼ����������Ҫ��6���涼Ҫ��Ⱦ����
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
		//���̲߳�����ÿ���߳�����һ��Ԥ�˲���ͼ
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
			//��ԭ�������ͼ������½�
			image.flip_vertically();
			image.write_tga_file(paths[face_id]);

		}

	}

}

//ͬ�����ն�ͼ����������ͼ��6���涼Ҫ��Ⱦ������
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

