#include "pipeline.h"

static SpinLock sp;

//�ж��Ƿ��Ǳ��棬���������˳ʱ��������Ǳ��棬��ʱ��������
//�������ֶ�����������������������������ˣ���ʱ��Ϊ��ֵ��˳ʱ��Ϊ��ֵ

static int is_back_facing(vec3 ndc_pos[3])
{
	vec3 a = ndc_pos[0];
	vec3 b = ndc_pos[1];
	vec3 c = ndc_pos[2];

	float signed_area = a.x() * b.y() - a.y() * b.x() +
		b.x() * c.y() - b.y() * c.x() +
		c.x() * a.y() - c.y() * a.x();

	return signed_area <= 0;
}

//��������
static vec3 compute_barycentric2D(float x, float y, const vec3* v)
{
	float c1 = (x * (v[1].y() - v[2].y()) + (v[2].x() - v[1].x()) * y + v[1].x() * v[2].y() - v[2].x() * v[1].y()) / (v[0].x() * (v[1].y() - v[2].y()) + (v[2].x() - v[1].x()) * v[0].y() + v[1].x() * v[2].y() - v[2].x() * v[1].y());
	float c2 = (x * (v[2].y() - v[0].y()) + (v[0].x() - v[2].x()) * y + v[2].x() * v[0].y() - v[0].x() * v[2].y()) / (v[1].x() * (v[2].y() - v[0].y()) + (v[0].x() - v[2].x()) * v[1].y() + v[2].x() * v[0].y() - v[0].x() * v[2].y());
	return vec3(c1, c2, 1 - c1 - c2);
}
//�Ѷ�Ӧÿ�����ص�ֵ���浽framebuffer�ϣ�û���һ��ֵ�ʹ����
static void set_color(unsigned char* framebuffer, int x, int y, unsigned char color[])
{
	int i;
	int index = ((WINDOW_HEDIGHT - y - 1) * WINDOW_WIDTH + x) * 4;
	for (i = 0;i < 3;i++)
	{
		framebuffer[index + i] = color[i];
	}
}

//�Ƿ����������ڲ�
static int is_inside_triangle(float alpha, float beta, float gamma)
{
	//֪����������ֱ���ж��Ƿ񶼴���0������0�����������ڲ�����һ��С��0�����ⲿ
	int flag = 0;
	//��������0�Ƚ�ʹ��EPSILON
	if (alpha > -EPSILON && beta > -EPSILON && gamma > -EPSILON)
	{
		flag = 1;
	}
	return flag;
}

//��ȡframebuffer�ϵ�����ֵ
static int get_index(int x, int y)
{
	return(WINDOW_HEDIGHT - y - 1) * WINDOW_WIDTH + x;
}


typedef enum
{
	W_PLANE,//Wƽ����һ������Ĳü�ƽ�棬λ����׵���Զ�������һ�ࡣ��������Ȼ�����(depth buffer)�еĽ��ü�,��Զ������������޳�
	X_RIGHT,//X_RIGHT �� X_LEFT���������ü�ƽ�涨�����Ӿ�������ұ߽磬�����޳������Ұ��Χ֮������塣
	X_LEFT,
	Y_TOP,//Y_TOP �� Y_BOTTOM���������ü�ƽ�涨�����Ӿ�������±߽磬�����޳������Ұ��Χ֮������塣
	Y_BOTTOM,
	Z_NEAR,//Z_NEAR �� Z_FAR���������ü�ƽ�涨�����Ӿ���Ľ��ü�ƽ���Զ�ü�ƽ�棬�����޳������Ұ��Χ֮������塣
	Z_FAR
}clip_plane;//7���ü���

//�ж�ĳ�����Ƿ��ڲü��淶Χ�ڣ��ھͱ��������ھͶ���
static int is_inside_plane(clip_plane c_plane, vec4 vertex)
{
	switch (c_plane)
	{
	case W_PLANE:
		return vertex.w() <= -EPSILON;//��Ҫע�����w�Ǹ���ֵ������������Ҫ�ж��Ƿ�<=0
	case X_RIGHT:
		return vertex.x() >= vertex.w();//��Ϊw�Ǹ�ֵ����ͽ���Ϊʲô��>=
	case X_LEFT:
		return vertex.x() <= -vertex.w();
	case Y_TOP:
		return vertex.y() >= vertex.w();
	case Y_BOTTOM:
		return vertex.y() <= -vertex.w();
	case Z_NEAR:
		return vertex.z() >= vertex.w();
	case Z_FAR:
		return vertex.z() <= -vertex.w();

	default:
		return 0;
	}
}

//�ú�������ǵ�һ��������ƽ���ڣ�������һ����������׵����ʱ��Ҫ���н�������ڵ�λ�õı����жϣ��Ӷ����к������и�
//prev ��ǰһ����������꣬curv �ǵ�ǰ���������
//�����������Ǹ��ݸ����Ĳü�ƽ�棬����ǰһ������͵�ǰ����֮����ཻ������
//����ཻ��������ȷ���ڲü������У�
//ǰһ������͵�ǰ����֮��Ľ����ڲü�ƽ���ϵ�λ�á�
//���Ӳ�����ǰһ������͵�ǰ�����ڲü�ƽ���ϵľ����ֵ��
//��ĸ������ǰһ������͵�ǰ�����ڲü�ƽ���ϵľ����ֵ�ټ�ȥһ����С��ƫ���� EPSILON��
//�Ա�����������
//��������е��󵼣���Ҫ����Ϊw�Ǹ�ֵ�������Ͼ���һ���߶��ཻ��һƽ�棬Ȼ���������λ���㽻�㣬�������������
static float get_intersect_ratio(vec4 prev, vec4 curv, clip_plane c_plane)
{
	switch (c_plane)
	{
	case W_PLANE:
		return (prev.w() + EPSILON) / (prev.w() - curv.w());
	case X_RIGHT:
		return (prev.w() - prev.x()) / ((prev.w() - prev.x()) - (curv.w() - curv.x()));
	case X_LEFT:
		return (prev.w() + prev.x()) / ((prev.w() + prev.x()) - (curv.w() + curv.x()));
	case Y_TOP:
		return (prev.w() - prev.y()) / ((prev.w() - prev.y()) - (curv.w() - curv.y()));
	case Y_BOTTOM:
		return (prev.w() + prev.y()) / ((prev.w() + prev.y()) - (curv.w() + curv.y()));
	case Z_NEAR:
		return (prev.w() - prev.z()) / ((prev.w() - prev.z()) - (curv.w() - curv.z()));
	case Z_FAR:
		return (prev.w() + prev.z()) / ((prev.w() + prev.z()) - (curv.w() + curv.z()));
	default:
		return 0;
	}
}

//��������������Ǹ��ݸ����Ĳü�ƽ�����ά������вü������ü���Ķ���洢����������У������زü���Ķ�������
//����㷨��sutherland-hodgman�㷨��һ�ֱ��塣
static int clip_with_plane(clip_plane c_plane, int num_vert, payload_t& payload)
{
	int out_vert_num = 0;//���ڼ�¼�ü���Ķ�������
	int previous_index, current_index;//���ڼ�¼��ǰ�����������ǰһ�����������
	int is_odd = (c_plane + 1) % 2;//�����жϵ�ǰ���Ƿ�Ϊ����ƽ��

	//���������������ݵ�ָ�룬����ƽ�����ż�ԣ����ö�Ӧ��������������
	vec4* in_clipcoord = is_odd ? payload.in_clipcoord : payload.out_clipcoord;
	vec3* in_worldcoord = is_odd ? payload.in_worldcoord : payload.out_worldcoord;
	vec3* in_normal = is_odd ? payload.in_normal : payload.out_normal;
	vec2* in_uv = is_odd ? payload.in_uv : payload.out_uv;
	vec4* out_clipcoord = is_odd ? payload.out_clipcoord : payload.in_clipcoord;
	vec3* out_worldcoord = is_odd ? payload.out_worldcoord : payload.in_worldcoord;
	vec3* out_normal = is_odd ? payload.out_normal :payload.in_normal;
	vec2* out_uv = is_odd ? payload.out_uv : payload.in_uv;

	//�������еıߣ��ӵ�һ�����㿪ʼ
	for (int i = 0;i < num_vert;i++)
	{
		current_index = i;//��ǰ���������
		previous_index = (i - 1 + num_vert) / num_vert;//ǰһ�����������
		vec4 cur_vertex = in_clipcoord[current_index];//��������ֵ��ȡ�ü�����
		vec4 pre_vertex = in_clipcoord[previous_index];

		bool is_cur_inside = is_inside_plane(c_plane, cur_vertex);//�жϵ�ǰ�����Ƿ��ڲü���Χ��
		bool is_pre_inside = is_inside_plane(c_plane, pre_vertex);//�ж�ǰһ�������Ƿ��ڲü���Χ��

		if(is_cur_inside != is_pre_inside)//������������ڲü���Ĳ�ͬ��
		{
			//���㽻��ռ��
			float ratio = get_intersect_ratio(pre_vertex, cur_vertex, c_plane);

			//ͨ��ռ�Ȳ�ֵ�������������
			out_clipcoord[out_vert_num] = vec4_lerp(pre_vertex, cur_vertex, ratio);
			out_worldcoord[out_vert_num] = vec3_lerp(in_worldcoord[previous_index], in_worldcoord[current_index], ratio);
			out_normal[out_vert_num] = vec3_lerp(in_normal[previous_index], in_normal[current_index], ratio);
			out_uv[out_vert_num] = vec2_lerp(in_uv[previous_index], in_uv[current_index], ratio);

			out_vert_num++;

		}

		if (is_cur_inside)//��ǰ�������ڲü����ڣ�����Ҫ������ӵ��ü���Ķ����б���
		{
			out_clipcoord[out_vert_num] = cur_vertex;
			out_worldcoord[out_vert_num] = in_worldcoord[current_index];
			out_normal[out_vert_num] = in_normal[current_index];
			out_uv[out_vert_num] = in_uv[current_index];

			out_vert_num++;
		}

	}

	return out_vert_num;//���زü���Ķ�������

}

//��βü�������׵���6���ü��涼�������������н��вü�
static int homo_clipping(payload_t& payload)
{
	int num_vertex = 3;
	num_vertex = clip_with_plane(W_PLANE, num_vertex, payload);
	num_vertex = clip_with_plane(X_RIGHT, num_vertex, payload);
	num_vertex = clip_with_plane(X_LEFT, num_vertex, payload);
	num_vertex = clip_with_plane(Y_TOP, num_vertex, payload);
	num_vertex = clip_with_plane(Y_BOTTOM, num_vertex, payload);
	num_vertex = clip_with_plane(Z_NEAR, num_vertex, payload);
	num_vertex = clip_with_plane(Z_FAR, num_vertex, payload);

	return num_vertex;
}

//�������ü���������������´洢���ṹ���������
static void transform_attri(payload_t& payload, int index0, int index1, int index2)
{
	payload.clipcoord_attri[0] = payload.out_clipcoord[index0];
	payload.clipcoord_attri[1] = payload.out_clipcoord[index1];
	payload.clipcoord_attri[2] = payload.out_clipcoord[index2];

	payload.worldcoord_attri[0] = payload.out_worldcoord[index0];
	payload.worldcoord_attri[1] = payload.out_worldcoord[index1];
	payload.worldcoord_attri[2] = payload.out_worldcoord[index2];

	payload.normal_attri[0] = payload.out_normal[index0];
	payload.normal_attri[1] = payload.out_normal[index1];
	payload.normal_attri[2] = payload.out_normal[index2];

	payload.uv_attri[0] = payload.out_uv[index0];
	payload.uv_attri[1] = payload.out_uv[index1];
	payload.uv_attri[2] = payload.out_uv[index2];
}

//���̹߳�դ��
//�����ֱ��ʾ�ü����ꡢ�������к�bitmap�󶨻��Ƶ�framebuffer��
//��Ȳ��Ե�zbuffer���Լ�ģ��ʹ�õ���ɫ��
void rasterize_singlethread(vec4* clipcoord_attri, unsigned char* framebuffer,
	float* zbuffer, IShader& shader)
{
	//�����׼���豸���꣬��Ϊ�������Σ�������������ά����
	vec3 ndc_pos[3];
	vec3 screen_pos[3];//��Ļ����

	//������ɫ������
	unsigned char c[3];

	int width = window->width;
	int height = window->height;
	int is_skybox = shader.payload.model->is_skybox;

	//͸�ӳ��������ü������õ�ndc���꣬͸�ӳ������w������ʾԶ���������γɽ���ԶС��һ��Ч��
	for (int i = 0;i < 3;i++)
	{
		ndc_pos[i][0] = clipcoord_attri[i][0] / clipcoord_attri[i].w();
		ndc_pos[i][1] = clipcoord_attri[i][1] / clipcoord_attri[i].w();
		ndc_pos[i][2] = clipcoord_attri[i][2] / clipcoord_attri[i].w();

	}

	//�ӿڱ�õõ���Ļ����
	for (int i = 0;i < 3;i++)
	{
		screen_pos[i][0] = (ndc_pos[i][0] + 1.0) * 0.5 * ((double)width - 1);
		screen_pos[i][1] = (ndc_pos[i][1] + 1.0) * 0.5 * ((double)height - 1);
		screen_pos[i][2] = is_skybox ? 1000 : -clipcoord_attri[i].w();//��ͼ�ռ�Zֵ����պ�Ĭ��Ϊ����Զ
	}

	//�����޳�����պв���Ҫ
	if (!is_skybox)
	{
		if (is_back_facing(ndc_pos))
		{
			return;//�޳���ֱ�ӷ����������õ�
		}
	}

	//��Ҫ��դ���������εİ�Χ��
	float xmin = 10000, xmax = -10000, ymin = 10000, ymax = -10000;
	for (int i = 0;i < 3;i++)
	{
		xmin = float_min(xmin, screen_pos[i][0]);
		xmax = float_max(xmax, screen_pos[i][0]);
		ymin = float_min(ymin, screen_pos[i][1]);
		ymax = float_max(ymax, screen_pos[i][1]);
	}

	//��դ��

	for (int x = (int)xmin;x <= (int)xmax;x++)
	{
		for (int y = (int)ymin;y <= (int)ymax;y++)
		{
			//��������������������ε���������,������Ȳ�ֵ��������ɫ������
			vec3 barycentric = compute_barycentric2D((float)(x + 0.5), (float)(y + 0.5), screen_pos);
			float alpha = barycentric.x(); float beta = barycentric.y();float gamma = barycentric.z();

			//����������������ھͻ���
			if (is_inside_triangle(alpha, beta, gamma))
			{
				//��ȡ��������framebuffer�ϵ������Ա��ȡ��ɫֵ
				int index = get_index(x, y);

				//͸��У����ֵ
				float normalizer = 1.0 / ((double)alpha / clipcoord_attri[0].w() +
					(double)beta / clipcoord_attri[1].w() + 
					(double)gamma / clipcoord_attri[2].w());

				//������ʵ�����ֵz
				//�ڽ���͸�Ӳ�ֵʱ����Ҫ�Զ����������й�һ������������Բü������w������
				float z = (alpha * screen_pos[0].z() / clipcoord_attri[0].w() +
					beta * screen_pos[1].z() / clipcoord_attri[1].w() + 
					gamma * screen_pos[2].z() / clipcoord_attri[2].w()) * normalizer;

				//������Ȳ���
				if (zbuffer[index] > z)
				{
					zbuffer[index] = z;//������Ȼ���
					vec3 color = shader.fragment_shader(alpha, beta, gamma);
					//����Ƭ����ɫ�����м��㣬��Ϊû�����ù�Դ��ֻ��ֱ�ӹ��գ������������ע�͵�
//					if (shader.payload.isPBR)
//					{
//#if 1	
//						//������ǿ��ת��Ϊ����
//						PBRShader* pshader = dynamic_cast<PBRShader*>(&shader);
//						color = pshader->fragment_shader(alpha, beta, gamma) +
//							pshader->direct_fragment_shader(alpha, beta, gamma);
//#else
//						color = shader.fragment_shader(alpha, beta, gamma);
//#endif
//					}
//					else
//					{
//						color = shader.fragment_shader(alpha, beta, gamma);
//					}

					//����ɫֵ������0-255֮��
					for (int i = 0;i < 3;i++)
					{
						c[i] = (int)float_clamp(color[i], 0, 255);
					}

					set_color(framebuffer, x, y, c);

				}
			}

		}
	}
}
	

//���̹߳�դ��
void rasterize_multithread(vec4* clipcoord_attri, unsigned char* framebuffer, float* zbuffer, IShader& shader)
{
	vec3 ndc_pos[3];
	vec3 screen_pos[3];
	unsigned char c[3];
	int width = window->width;
	int height = window->height;

	int i = 0, j = 0, flag = 0;

	for (i = 0;i < 3;i++)
	{
		if (clipcoord_attri[i].w() > -0.1f)
		{
			flag = 1;
		}

		ndc_pos[i][0] = clipcoord_attri[i][0] / clipcoord_attri[i].w();
		ndc_pos[i][1] = clipcoord_attri[i][1] / clipcoord_attri[i].w();
		ndc_pos[i][2] = clipcoord_attri[i][2] / clipcoord_attri[i].w();
	}

	//�ӿڱ��
	for (i = 0;i < 3;i++)
	{
		screen_pos[i][0] = 0.5 * width * (ndc_pos[i][0] + 1.0);
		screen_pos[i][1] = 0.5 * height * (ndc_pos[i][1] + 1.0);
		screen_pos[i][2] = -clipcoord_attri[i].w();
	}

	//���ü�����
	if (flag)
	{
		return;
	}

	//�����εİ�Χ��
	float xmin = 10000, xmax = -10000, ymin = 10000, ymax = -10000;
	for (i = 0;i < 3;i++)
	{
		xmin = float_min(xmin, screen_pos[i][0]);
		xmax = float_max(xmax, screen_pos[i][0]);
		ymin = float_min(ymin, screen_pos[i][1]);
		ymax = float_max(ymax, screen_pos[i][1]);
	}

	//���̵߳Ĺ�դ��
	for (int x = (int)xmin;x <= (int)xmax;x++)
	{
		for (int y = (int)ymin;y <= (int)ymax;y++)
		{
			//������������ں�����Ȳ�ֵ����ɫ����
			vec3 result = compute_barycentric2D((float)x + 0.5, (float)y + 0.5, screen_pos);
			float alpha = result.x(), beta = result.y(), gamma = result.z();

			if (is_inside_triangle(alpha, beta, gamma))
			{
				int index = get_index(x, y);

				float normalizer = 1.0 / (alpha / clipcoord_attri[0].w() + beta / clipcoord_attri[1].w()
					+ gamma / clipcoord_attri[2].w());

				float z = (alpha * screen_pos[0].z() / clipcoord_attri[0].w() +
					beta * screen_pos[1].z() / clipcoord_attri[1].w() + 
					gamma * screen_pos[2].z() / clipcoord_attri[2].w())*normalizer;

				if (zbuffer[index] > z)
				{
					zbuffer[index] = z;
					vec3 color = shader.fragment_shader(alpha, beta, gamma);

					//������ɫ��0-255
					for (int i = 0;i < 3;i ++ )
					{
						c[i] = (int)float_clamp(color[i], 0, 255);
					}

					//ʹ��������������Ƿ���Ҫд����ɫ��buffer�У������Ҫ����д����
					//���߳���Ҫ�������Է�ֹ����д�����
					sp.lock();
					if (zbuffer[index] > z - EPSILON)
					{
						zbuffer[index] = z;
						set_color(framebuffer, x, y, c);
					}
					sp.unlock();

				}

			}

		}
	}


}
	

//���������Σ����δ���͹�դ�����������еĹ�դ�����������Ƭ����ɫ���Ĵ���
//�����������̶����������ʵ��
//���Ƚ�ģ�����ݴ����������ɫ����������nface��i���Ի�ȡģ�͵Ķ�������
//��������ٽ�����βü��õ��ü����꣬ͬʱ��òü���Ķ�����

//�������ж��㣬���������㹹�ɵ�payload�����դ�������н��й�դ��
//�����еĹ�դ�����������˲ü����굽ndc���굽�ӿڱ�ã��ٶ�������Χ���ڵ������ν��й�դ��
//�������������ڵ�Ƭ�ζ������ڵ����¸���zbuffer��framebuffer��


void draw_triangles(unsigned char* framebuffer, float* zbuffer, IShader& shader, int nface)
{
	//������������㶼�ü�����ɫ������
	for (int i = 0;i < 3;i++)
	{
		shader.vertex_shader(nface, i);
	}

	//��βü�
	int num_vertex = homo_clipping(shader.payload);

	//���ü���������ν��й�դ��
	//index0 �ǹ̶��ģ��� index1 �� index2 �᲻�ϵ��������γɲ�ͬ��������
	for (int i = 0;i < num_vertex - 2;i++)
	{
		int index0 = 0;
		int index1 = i + 1;
		int index2 = i + 2;

		//�������ü�����������´洢���ṹ����
		transform_attri(shader.payload, index0, index1, index2);

		rasterize_singlethread(shader.payload.clipcoord_attri, framebuffer, zbuffer, shader);
	}

}

