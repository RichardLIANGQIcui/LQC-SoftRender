#include "pipeline.h"

static SpinLock sp;

//判断是否是背面，如果顶点是顺时针排序就是背面，逆时针是正面
//根据右手定则，利用三角形任意两相邻向量叉乘，逆时针为正值，顺时针为负值

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

//重心坐标
static vec3 compute_barycentric2D(float x, float y, const vec3* v)
{
	float c1 = (x * (v[1].y() - v[2].y()) + (v[2].x() - v[1].x()) * y + v[1].x() * v[2].y() - v[2].x() * v[1].y()) / (v[0].x() * (v[1].y() - v[2].y()) + (v[2].x() - v[1].x()) * v[0].y() + v[1].x() * v[2].y() - v[2].x() * v[1].y());
	float c2 = (x * (v[2].y() - v[0].y()) + (v[0].x() - v[2].x()) * y + v[2].x() * v[0].y() - v[0].x() * v[2].y()) / (v[1].x() * (v[2].y() - v[0].y()) + (v[0].x() - v[2].x()) * v[1].y() + v[2].x() * v[0].y() - v[0].x() * v[2].y());
	return vec3(c1, c2, 1 - c1 - c2);
}
//把对应每个像素的值都存到framebuffer上，没算出一个值就存进来
static void set_color(unsigned char* framebuffer, int x, int y, unsigned char color[])
{
	int i;
	int index = ((WINDOW_HEDIGHT - y - 1) * WINDOW_WIDTH + x) * 4;
	for (i = 0;i < 3;i++)
	{
		framebuffer[index + i] = color[i];
	}
}

//是否在三角形内部
static int is_inside_triangle(float alpha, float beta, float gamma)
{
	//知道重心坐标直接判断是否都大于0，大于0则在三角形内部，有一个小于0则在外部
	int flag = 0;
	//浮点数和0比较使用EPSILON
	if (alpha > -EPSILON && beta > -EPSILON && gamma > -EPSILON)
	{
		flag = 1;
	}
	return flag;
}

//获取framebuffer上的索引值
static int get_index(int x, int y)
{
	return(WINDOW_HEDIGHT - y - 1) * WINDOW_WIDTH + x;
}


typedef enum
{
	W_PLANE,//W平面是一个虚拟的裁剪平面，位于视椎体的远离相机的一侧。它用于深度缓冲区(depth buffer)中的近裁剪,将远离相机的物体剔除
	X_RIGHT,//X_RIGHT 和 X_LEFT：这两个裁剪平面定义了视景体的左右边界，用于剔除相机视野范围之外的物体。
	X_LEFT,
	Y_TOP,//Y_TOP 和 Y_BOTTOM：这两个裁剪平面定义了视景体的上下边界，用于剔除相机视野范围之外的物体。
	Y_BOTTOM,
	Z_NEAR,//Z_NEAR 和 Z_FAR：这两个裁剪平面定义了视景体的近裁剪平面和远裁剪平面，用于剔除相机视野范围之外的物体。
	Z_FAR
}clip_plane;//7个裁剪面

//判读某个点是否在裁剪面范围内，在就保留，不在就丢弃
static int is_inside_plane(clip_plane c_plane, vec4 vertex)
{
	switch (c_plane)
	{
	case W_PLANE:
		return vertex.w() <= -EPSILON;//需要注意的是w是个负值，所以这里需要判断是否<=0
	case X_RIGHT:
		return vertex.x() >= vertex.w();//因为w是负值，这就解释为什么是>=
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

//该函数求的是当一个顶点在平面内，而另外一个顶点在视椎体外时就要进行交点的所在的位置的比例判断，从而进行后续的切割
//prev 是前一个顶点的坐标，curv 是当前顶点的坐标
//函数的作用是根据给定的裁剪平面，计算前一个顶点和当前顶点之间的相交比例。
//这个相交比例用于确定在裁剪过程中，
//前一个顶点和当前顶点之间的交点在裁剪平面上的位置。
//分子部分是前一个顶点和当前顶点在裁剪平面上的距离差值，
//分母部分是前一个顶点和当前顶点在裁剪平面上的距离差值再减去一个很小的偏移量 EPSILON，
//以避免除以零错误。
//下面代码有点误导，主要是因为w是负值，本质上就求一条线段相交于一平面，然后根据两点位置算交点，再算比例的问题
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

//这个函数的作用是根据给定的裁剪平面对三维物体进行裁剪，将裁剪后的顶点存储在输出数据中，并返回裁剪后的顶点数量
//这个算法是sutherland-hodgman算法的一种变体。
static int clip_with_plane(clip_plane c_plane, int num_vert, payload_t& payload)
{
	int out_vert_num = 0;//用于记录裁剪后的顶点数量
	int previous_index, current_index;//用于记录当前顶点的索引和前一个顶点的索引
	int is_odd = (c_plane + 1) % 2;//用来判断当前面是否为奇数平面

	//设置输入和输出数据的指针，根据平面的奇偶性，设置对应的输入和输出数据
	vec4* in_clipcoord = is_odd ? payload.in_clipcoord : payload.out_clipcoord;
	vec3* in_worldcoord = is_odd ? payload.in_worldcoord : payload.out_worldcoord;
	vec3* in_normal = is_odd ? payload.in_normal : payload.out_normal;
	vec2* in_uv = is_odd ? payload.in_uv : payload.out_uv;
	vec4* out_clipcoord = is_odd ? payload.out_clipcoord : payload.in_clipcoord;
	vec3* out_worldcoord = is_odd ? payload.out_worldcoord : payload.in_worldcoord;
	vec3* out_normal = is_odd ? payload.out_normal :payload.in_normal;
	vec2* out_uv = is_odd ? payload.out_uv : payload.in_uv;

	//遍历所有的边，从第一个顶点开始
	for (int i = 0;i < num_vert;i++)
	{
		current_index = i;//当前顶点的索引
		previous_index = (i - 1 + num_vert) / num_vert;//前一个顶点的索引
		vec4 cur_vertex = in_clipcoord[current_index];//根据索引值获取裁剪坐标
		vec4 pre_vertex = in_clipcoord[previous_index];

		bool is_cur_inside = is_inside_plane(c_plane, cur_vertex);//判断当前顶点是否在裁剪范围内
		bool is_pre_inside = is_inside_plane(c_plane, pre_vertex);//判断前一个顶点是否在裁剪范围内

		if(is_cur_inside != is_pre_inside)//如果两个顶点在裁剪面的不同侧
		{
			//计算交点占比
			float ratio = get_intersect_ratio(pre_vertex, cur_vertex, c_plane);

			//通过占比插值计算出交点坐标
			out_clipcoord[out_vert_num] = vec4_lerp(pre_vertex, cur_vertex, ratio);
			out_worldcoord[out_vert_num] = vec3_lerp(in_worldcoord[previous_index], in_worldcoord[current_index], ratio);
			out_normal[out_vert_num] = vec3_lerp(in_normal[previous_index], in_normal[current_index], ratio);
			out_uv[out_vert_num] = vec2_lerp(in_uv[previous_index], in_uv[current_index], ratio);

			out_vert_num++;

		}

		if (is_cur_inside)//当前顶点在在裁剪面内，则需要将其添加到裁剪后的顶点列表中
		{
			out_clipcoord[out_vert_num] = cur_vertex;
			out_worldcoord[out_vert_num] = in_worldcoord[current_index];
			out_normal[out_vert_num] = in_normal[current_index];
			out_uv[out_vert_num] = in_uv[current_index];

			out_vert_num++;
		}

	}

	return out_vert_num;//返回裁剪后的顶点数量

}

//齐次裁剪，将视椎体的6个裁剪面都传到上述函数中进行裁剪
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

//将经过裁剪后的所有属性重新存储到结构体的属性中
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

//单线程光栅化
//参数分别表示裁剪坐标、用来进行和bitmap绑定绘制的framebuffer，
//深度测试的zbuffer，以及模型使用的着色器
void rasterize_singlethread(vec4* clipcoord_attri, unsigned char* framebuffer,
	float* zbuffer, IShader& shader)
{
	//定义标准化设备坐标，因为是三角形，所以是三个三维坐标
	vec3 ndc_pos[3];
	vec3 screen_pos[3];//屏幕坐标

	//保存颜色的数组
	unsigned char c[3];

	int width = window->width;
	int height = window->height;
	int is_skybox = shader.payload.model->is_skybox;

	//透视除法，将裁剪坐标变幻到ndc坐标，透视除法里的w分量表示远近，用来形成近大远小的一个效果
	for (int i = 0;i < 3;i++)
	{
		ndc_pos[i][0] = clipcoord_attri[i][0] / clipcoord_attri[i].w();
		ndc_pos[i][1] = clipcoord_attri[i][1] / clipcoord_attri[i].w();
		ndc_pos[i][2] = clipcoord_attri[i][2] / clipcoord_attri[i].w();

	}

	//视口变幻得到屏幕坐标
	for (int i = 0;i < 3;i++)
	{
		screen_pos[i][0] = (ndc_pos[i][0] + 1.0) * 0.5 * ((double)width - 1);
		screen_pos[i][1] = (ndc_pos[i][1] + 1.0) * 0.5 * ((double)height - 1);
		screen_pos[i][2] = is_skybox ? 1000 : -clipcoord_attri[i].w();//视图空间Z值，天空盒默认为无穷远
	}

	//背部剔除。天空盒不需要
	if (!is_skybox)
	{
		if (is_back_facing(ndc_pos))
		{
			return;//剔除就直接返回无需计算该点
		}
	}

	//需要光栅化的三角形的包围盒
	float xmin = 10000, xmax = -10000, ymin = 10000, ymax = -10000;
	for (int i = 0;i < 3;i++)
	{
		xmin = float_min(xmin, screen_pos[i][0]);
		xmax = float_max(xmax, screen_pos[i][0]);
		ymin = float_min(ymin, screen_pos[i][1]);
		ymax = float_max(ymax, screen_pos[i][1]);
	}

	//光栅化

	for (int x = (int)xmin;x <= (int)xmax;x++)
	{
		for (int y = (int)ymin;y <= (int)ymax;y++)
		{
			//计算该像素中心在三角形的重心坐标,便于深度插值及后续着色器计算
			vec3 barycentric = compute_barycentric2D((float)(x + 0.5), (float)(y + 0.5), screen_pos);
			float alpha = barycentric.x(); float beta = barycentric.y();float gamma = barycentric.z();

			//如果像素在三角形内就绘制
			if (is_inside_triangle(alpha, beta, gamma))
			{
				//获取该像素在framebuffer上的索引以便获取颜色值
				int index = get_index(x, y);

				//透视校正插值
				float normalizer = 1.0 / ((double)alpha / clipcoord_attri[0].w() +
					(double)beta / clipcoord_attri[1].w() + 
					(double)gamma / clipcoord_attri[2].w());

				//求像素实际深度值z
				//在进行透视插值时，需要对顶点的坐标进行归一化，即将其除以裁剪坐标的w分量。
				float z = (alpha * screen_pos[0].z() / clipcoord_attri[0].w() +
					beta * screen_pos[1].z() / clipcoord_attri[1].w() + 
					gamma * screen_pos[2].z() / clipcoord_attri[2].w()) * normalizer;

				//进行深度测试
				if (zbuffer[index] > z)
				{
					zbuffer[index] = z;//更新深度缓存
					vec3 color = shader.fragment_shader(alpha, beta, gamma);
					//调用片段着色器进行计算，因为没有设置光源，只有直接光照，所以下面代码注释掉
//					if (shader.payload.isPBR)
//					{
//#if 1	
//						//将父类强制转换为子类
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

					//把颜色值限制在0-255之间
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
	

//多线程光栅化
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

	//视口变幻
	for (i = 0;i < 3;i++)
	{
		screen_pos[i][0] = 0.5 * width * (ndc_pos[i][0] + 1.0);
		screen_pos[i][1] = 0.5 * height * (ndc_pos[i][1] + 1.0);
		screen_pos[i][2] = -clipcoord_attri[i].w();
	}

	//检查裁剪条件
	if (flag)
	{
		return;
	}

	//三角形的包围盒
	float xmin = 10000, xmax = -10000, ymin = 10000, ymax = -10000;
	for (i = 0;i < 3;i++)
	{
		xmin = float_min(xmin, screen_pos[i][0]);
		xmax = float_max(xmax, screen_pos[i][0]);
		ymin = float_min(ymin, screen_pos[i][1]);
		ymax = float_max(ymax, screen_pos[i][1]);
	}

	//多线程的光栅化
	for (int x = (int)xmin;x <= (int)xmax;x++)
	{
		for (int y = (int)ymin;y <= (int)ymax;y++)
		{
			//求重心坐标便于后续深度插值和颜色计算
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

					//限制颜色到0-255
					for (int i = 0;i < 3;i ++ )
					{
						c[i] = (int)float_clamp(color[i], 0, 255);
					}

					//使用自旋锁来检查是否需要写入颜色到buffer中，如果需要，重写数据
					//多线程需要加锁，以防止数据写入出错
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
	

//绘制三角形，几何处理和光栅化处理，代码中的光栅化处理包括了片段着色器的处理
//整个绘制流程都在这个函数实现
//首先将模型数据传入给顶点着色器处理，传入nface和i可以获取模型的顶点数据
//处理完后再进行齐次裁剪得到裁剪坐标，同时获得裁剪后的顶点数

//遍历所有顶点，将三个顶点构成的payload传入光栅化函数中进行光栅化
//代码中的光栅化函数包括了裁剪坐标到ndc坐标到视口变幻，再对整个包围盒内的三角形进行光栅化
//即不在三角形内的片段丢弃，在的留下更新zbuffer即framebuffer。


void draw_triangles(unsigned char* framebuffer, float* zbuffer, IShader& shader, int nface)
{
	//将面的三个顶点都用几何着色器处理
	for (int i = 0;i < 3;i++)
	{
		shader.vertex_shader(nface, i);
	}

	//齐次裁剪
	int num_vertex = homo_clipping(shader.payload);

	//将裁剪后的三角形进行光栅化
	//index0 是固定的，而 index1 和 index2 会不断递增，以形成不同的三角形
	for (int i = 0;i < num_vertex - 2;i++)
	{
		int index0 = 0;
		int index1 = i + 1;
		int index2 = i + 2;

		//将经过裁剪后的坐标重新存储进结构体中
		transform_attri(shader.payload, index0, index1, index2);

		rasterize_singlethread(shader.payload.clipcoord_attri, framebuffer, zbuffer, shader);
	}

}

