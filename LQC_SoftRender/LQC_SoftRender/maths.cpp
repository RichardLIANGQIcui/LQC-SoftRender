#include "maths.h"

using namespace std;


//vec2
vec2::vec2():e{ 0,0 } {};
vec2::vec2(float e0, float e1):e{e0,e1}{}

float vec2::x() const
{
	return e[0];
}

float vec2::y() const
{
	return e[1];
}

float& vec2::operator[](int i)
{
	return e[i];
}

float vec2::operator[](int i) const
{
	return e[i];
}

vec2 vec2::operator-() const
{
	return vec2(-e[0], -e[1]);
}

vec2& vec2::operator+=(const vec2& v)
{
	e[0] += v[0];
	e[1] += v[1];
	return *this;
}

vec2& vec2::operator*=(const float t)
{
	e[0] *= t;
	e[1] *= t;
	return *this;
}

vec2& vec2::operator/=(const float t)
{
	e[0] /= t;
	e[1] /= t;
	return *this;
}

float vec2::norm_squared() const
{
	return e[0] * e[0]+e[1]*e[1];
}

float vec2::norm() const
{
	return sqrt(norm_squared());
}

std::ostream& operator<<(std::ostream& out, const vec2& v)
{
	return out << v.e[0] << ' ' << v.e[1];
}

vec2 operator+(const vec2& u, const vec2& v)
{
	return vec2(u.e[0] + v.e[0], u.e[1] + v.e[1]);
}

vec2 operator-(const vec2& u, const vec2& v)
{
	return vec2(u.e[0] - v.e[0], u.e[1] - v.e[1]);
}

vec2 operator*(const vec2& u, const vec2& v)
{
	return vec2(u.e[0] * v.e[0], u.e[1] * v.e[1]);
}

vec2 operator*(double t, const vec2& v)
{
	return vec2(t* v.e[0],t * v.e[1]);
}

vec2 operator*(const vec2& v, double t)
{
	return t * v;
}

vec2 operator/(vec2 v, double t)
{
	return (1 / t) * v;
}


//vec3--------------------------------------
vec3::vec3() :e{ 0,0,0 } {};
vec3::vec3(float e0, float e1,float e2) :e{ e0,e1,e2 } {}

float vec3::x() const
{
	return e[0];
}

float vec3::y() const
{
	return e[1];
}

float vec3::z() const
{
	return e[2];
}

float& vec3::operator[](int i)
{
	return e[i];
}

float vec3::operator[](int i) const
{
	return e[i];
}

vec3 vec3::operator-() const
{
	return vec3(-e[0], -e[1],-e[2]);
}

vec3& vec3::operator+=(const vec3& v)
{
	e[0] += v[0];
	e[1] += v[1];
	e[2] += v[2];
	return *this;
}

vec3& vec3::operator*=(const float t)
{
	e[0] *= t;
	e[1] *= t;
	e[2] *= t;
	return *this;
}

vec3& vec3::operator/=(const float t)
{
	e[0] /= t;
	e[1] /= t;
	e[2] /= t;
	return *this;
}

float vec3::norm_squared() const
{
	return e[0] * e[0] + e[1] * e[1]+e[2]*e[2];
}

float vec3::norm() const
{
	return sqrt(norm_squared());
}

std::ostream& operator<<(std::ostream& out, const vec3& v)
{
	return out << v.e[0] << ' ' << v.e[1]<<' '<<v.e[2];
}

vec3 operator+(const vec3& u, const vec3& v)
{
	return vec3(u.e[0] + v.e[0], u.e[1] + v.e[1],u.e[2]+v.e[2]);
}

vec3 operator-(const vec3& u, const vec3& v)
{
	return vec3(u.e[0] - v.e[0], u.e[1] - v.e[1],u.e[2]-v.e[2]);
}

vec3 operator*(const vec3& u, const vec3& v)
{
	return vec3(u.e[0] * v.e[0], u.e[1] * v.e[1],u.e[2]*v.e[2]);
}

vec3 operator*(double t, const vec3& v)
{
	return vec3(t * v.e[0], t * v.e[1],t*v.e[2]);
}

vec3 operator*(const vec3& v, double t)
{
	return t * v;
}

vec3 operator/(vec3 v, double t)
{
	return (1 / t) * v;
}

double dot(const vec3& u, const vec3& v)
{
	return u.e[0] * v.e[0] + u.e[1] * v.e[1] + u.e[2] * v.e[2];
}

vec3 cross(const vec3& u, const vec3& v)
{
	return vec3(u.e[1] * v.e[2] - u.e[2] * v.e[1],
		u.e[2] * v.e[0] - u.e[0] * v.e[2],
		u.e[0] * v.e[1] - u.e[1] * v.e[0]);
}

vec3 unit_vector(const vec3& v)
{
	return v / v.norm();
}

vec3 cwise_product(const vec3& a, const vec3& b)
{
	return vec3(a[0] * b[0], a[1] * b[1], a[2] * b[2]);
}


//vec4---------------------------------------

vec4::vec4() :e{ 0,0,0,0 } {};
vec4::vec4(float e0, float e1, float e2,float e3) :e{ e0,e1,e2,e3 } {}

float vec4::x() const
{
	return e[0];
}

float vec4::y() const
{
	return e[1];
}

float vec4::z() const
{
	return e[2];
}

float vec4::w() const
{
	return e[3];
}

float& vec4::operator[](int i)
{
	return e[i];
}

float vec4::operator[](int i) const
{
	return e[i];
}

vec4& vec4::operator*=(const float t)
{
	e[0] *= t;
	e[1] *= t;
	e[2] *= t;
	e[3] *= t;
	return *this;
}

vec4& vec4::operator/=(const float t)
{
	e[0] /= t;
	e[1] /= t;
	e[2] /= t;
	e[3] /= t;
	return *this;
}

vec4 to_vec4(const vec3& u, float w)
{
	return vec4(u[0], u[1], u[2], w);
}

std::ostream& operator<<(std::ostream& out, const vec4& v)
{
	return out << v.e[0] << ' ' << v.e[1] << ' ' << v.e[2]<<' '<<v.e[3];
}

vec4 operator+(const vec4& u, const vec4& v)
{
	return vec4(u.e[0] + v.e[0], u.e[1] + v.e[1], u.e[2] + v.e[2],u.e[3]+v.e[3]);
}

vec4 operator-(const vec4& u, const vec4& v)
{
	return vec4(u.e[0] - v.e[0], u.e[1] - v.e[1], u.e[2] - v.e[2],u.e[3]-v.e[3]);
}

vec4 operator*(double t, const vec4& v)
{
	return vec4(t * v.e[0], t * v.e[1], t * v.e[2],t*v.e[2]);
}

vec4 operator*(const vec4& v, double t)
{
	return t * v;
}

//mat3---------------------------------------------------
mat3::mat3(){}

vec3& mat3::operator[](int i)
{
	return rows[i];
}

vec3 mat3::operator[](int i) const
{
	return rows[i];
}

//矩阵行列式、伴随矩阵、逆矩阵求解
//行列式https://blog.csdn.net/weixin_37410657/article/details/130695017?ops_request_misc=&request_id=&biz_id=102&utm_term=%E7%9F%A9%E9%98%B5%E8%A1%8C%E5%88%97%E5%BC%8F&utm_medium=distribute.pc_search_result.none-task-blog-2~all~sobaiduweb~default-0-130695017.142^v94^chatsearchT3_1&spm=1018.2226.3001.4187
//对于三阶，其公式为aei+bfg+cdh−afh−bdi−ceg
static float mat3_determinant(const mat3& m)
{
	float a = m[0][0];
	float b = m[0][1];
	float c = m[0][2];
	float d = m[1][0];
	float e = m[1][1];
	float f = m[1][2];
	float g = m[2][0];
	float h = m[2][1];
	float i = m[2][2];
	return a * e * i + b * f * g + c * d * h - a * f * h - b * d * i - c * e * g;
}

//伴随矩阵:每个元素的余子式的行列式计算的结果构成伴随矩阵，即每个元素都是可以计算的
static mat3 mat3_adjoint(const mat3& m)
{
	mat3 adjoint;
	adjoint[0][0] = +(m[1][1] * m[2][2] - m[2][1] * m[1][2]);
	adjoint[0][1] = -(m[1][0] * m[2][2] - m[2][0] * m[1][2]);
	adjoint[0][2] = +(m[1][0] * m[2][1] - m[2][0] * m[1][1]);
	adjoint[1][0] = -(m[0][1] * m[2][2] - m[2][1] * m[0][2]);
	adjoint[1][1] = +(m[0][0] * m[2][2] - m[2][0] * m[0][2]);
	adjoint[1][2] = -(m[0][0] * m[2][1] - m[2][0] * m[0][1]);
	adjoint[2][0] = +(m[0][1] * m[1][2] - m[1][1] * m[0][2]);
	adjoint[2][1] = -(m[0][0] * m[1][2] - m[1][0] * m[0][2]);
	adjoint[2][2] = +(m[0][0] * m[1][1] - m[1][0] * m[0][1]);
	return adjoint;
}


//转置的实现就是m[i][j] = m[j][i]
mat3 mat3::transpose() const
{
	mat3 transpose;

	for (int i = 0;i < 3;i++)
	{
		for (int j = 0;j < 3;j++)
		{
			transpose[i][j] = rows[j][i];
		}
	}
	return transpose;
}

mat3 mat3::identity()
{
	mat3 mat;
	for (int i = 0;i < 3;i++)
	{
		for (int j = 0;j < 3;j++)
		{
			mat[i][j] = i == j ? 1 : 0;
		}
	}
	return mat;
}

//矩阵求逆包括待定系数法、高斯消元法，用LU分解求矩阵的逆，伴随矩阵+代数余子式
//这里使用的就是伴随矩阵+代数余子式，原理是矩阵的逆 = 伴随矩阵/行列式
mat3 mat3::inverse_transpose() const
{
	int i, j;
	mat3 inverse_transpose, adjoint;
	float determinant, inv_determinant;

	adjoint = mat3_adjoint(*this);
	determinant = mat3_determinant(*this);
	inv_determinant = 1 / determinant;

	for (int i = 0;i < 3;i++)
	{
		for (int j = 0;j < 3;j++)
		{
			inverse_transpose[i][j] = adjoint[i][j] * inv_determinant;
		}
	}
	return inverse_transpose;
}

mat3 mat3::inverse() const
{
	return inverse_transpose().transpose();
}

std::ostream& operator<<(std::ostream& out, const mat3& m)
{
	return out << m[0] << "\n" << m[1] << "\n" << m[2];
}


//mat4--------------------------
mat4::mat4() {}
vec4 mat4::operator[](int i)const { return rows[i]; }
vec4& mat4::operator[](int i) { return  rows[i]; }


//矩阵的次因子、辅因子和伴随矩阵是矩阵求逆的一种方法。
//首先，次因子矩阵是由矩阵的代数余子式组成的矩阵。
//代数余子式是通过去除矩阵的某一行和某一列后计算所得的行列式的值。
//次因子矩阵的每个元素都是对应位置上的代数余子式。
//然后，辅助因子矩阵是将次因子矩阵的元素按照一定的规律进行正负交替变换后得到的矩阵。
//最后，伴随矩阵是将辅助因子矩阵转置得到的矩阵

//求四维矩阵某一行某一列的代数余子式
static float mat4_minor(mat4 m, int r, int c)
{
	mat3 cut_down;
	//剔除r行c列得到一个新矩阵cut_down
	int i, j;
	for (i = 0;i < 3;i++)
	{
		for (j = 0;j < 3;j++)
		{
			int row = i < r ? i : i + 1;
			int col = j < c ? j : j + 1;
			cut_down[i][j] = m[row][col];
		}
	}

	return mat3_determinant(cut_down);
}

//这里实际将代数余子式乘上辅因子-1的（r+c）次方
static float mat4_cofactor(mat4 m, int r, int c)
{
	float sign = (r + c) % 2 == 0 ? 1.0f : -1.0f;
	float minor = mat4_minor(m, r, c) ;

	return sign * minor;
}

//四维的伴随矩阵
static mat4 mat4_adjoint(mat4 m)
{
	mat4 adjoint;
	for (int i = 0;i < 4;i++)
	{
		for (int j = 0;j < 4;j++)
		{
			adjoint[i][j] = mat4_cofactor(m, i, j);
		}
	}
	return adjoint;
}

mat4 mat4::identity()
{
	mat4 mat;
	for (int i = 0;i < 4;i++)
	{
		for (int j = 0;j < 4;j++)
		{
			mat[i][j] = i == j ? 1 : 0;
		}
	}
	return mat;
}

mat4 mat4::transpose() const
{
	mat4 transpose;
	for (int i = 0;i < 4;i++)
	{
		for (int j = 0;j < 4;j++)
		{
			transpose[i][j] = rows[j][i];
		}
	}
	return transpose;
}

mat4 mat4::inverse_transpose() const
{
	float determinant, inv_determinant;
	mat4 adjoint, inverse_transpose;

	adjoint = mat4_adjoint(*this);
	determinant = 0;

	for (int i = 0;i < 4;i++)
	{
		determinant += rows[0][i] * adjoint[0][i];
	}

	inv_determinant = 1 / determinant;

	for (int i = 0;i < 4;i++)
	{
		for (int j = 0;j < 4;j++)
		{
			inverse_transpose[i][j] = adjoint[i][j] * inv_determinant;
		}
	}
	return inverse_transpose;
}

mat4 mat4::inverse() const
{
	return inverse_transpose().transpose();
}

std::ostream& operator<<(std::ostream& out, const mat4& m)
{
	return out << m[0] << "\n" << m[1] << "\n" << m[2] << "\n" << m[3];
}

vec4 operator*(const mat4& m, const vec4 v)
{
	float product[4];
	for (int i = 0;i < 4;i++)
	{
		float a = m[i][0] * v[0];
		float b = m[i][1] * v[1];
		float c = m[i][2] * v[2];
		float d = m[i][3] * v[3];

		product[i] = a + b + c + d;
	}

	return vec4(product[0], product[1], product[2], product[3]);
}

mat4 operator*(const mat4& m1, const mat4& m2)
{
	mat4 procudt;

	for (int i = 0;i < 4;i++)
	{
		for (int j = 0;j < 4;j++)
		{
			float a = m1[i][0] * m2[0][j];
			float b = m1[i][1] * m2[1][j];
			float c = m1[i][2] * m2[2][j];
			float d = m1[i][3] * m2[3][j];

			procudt[i][j] = a + b + c + d;
		}
	}

	return procudt;
}

mat4 mat4_translate(float tx, float ty, float tz)
{
	mat4 translate = mat4::identity();
	translate[0][3] = tx;
	translate[1][3] = ty;
	translate[2][3] = tz;

	return translate;
}

mat4 mat4_scale(float sx, float sy, float sz)
{
	mat4 scale = mat4::identity();
	scale[0][0] = sx;
	scale[1][1] = sy;
	scale[2][2] = sz;

	return scale;
}

mat4 mat4_rotate_x(float angle)
{
	mat4 m = mat4::identity();
	angle = angle / 180 * PI;
	float c = cos(angle);
	float s = sin(angle);
	m[1][1] = c;
	m[1][2] = -s;
	m[2][1] = s;
	m[2][2] = c;
	return m;
}

mat4 mat4_rotate_y(float angle)
{
	mat4 m = mat4::identity();
	angle = angle / 180 * PI;
	float c = cos(angle);
	float s = sin(angle);
	m[0][0] = c;
	m[0][2] = s;
	m[2][0] = -s;
	m[2][2] = c;
	return m;
}

mat4 mat4_rotate_z(float angle)
{
	mat4 m = mat4::identity();
	angle = angle / 180 * PI;
	float c = cos(angle);
	float s = sin(angle);
	m[0][0] = c;
	m[0][1] = -s;
	m[1][0] = s;
	m[1][1] = c;
	return m;
}

mat4 mat4_lookat(vec3 eye, vec3 target, vec3 up)
{
	mat4 m = mat4::identity();

	vec3 z = unit_vector(eye - target);
	vec3 x = unit_vector(cross(up, z));
	vec3 y = unit_vector(cross(z, x));

	m[0][0] = x[0];
	m[0][1] = x[1];
	m[0][2] = x[2];

	m[1][0] = y[0];
	m[1][1] = y[1];
	m[1][2] = y[2];

	m[2][0] = z[0];
	m[2][1] = z[1];
	m[2][2] = z[2];

	m[0][3] = -dot(x, eye); 
	m[1][3] = -dot(y, eye);
	m[2][3] = -dot(z, eye);

	return m;
}

mat4 mat4_ortho(float left, float right, float bottom, float top,float near, float far)
{
	float x_range = right - left;
	float y_range = top - bottom;
	float z_range = near - far;
	mat4 m = mat4::identity();
	m[0][0] = 2 / x_range;
	m[1][1] = 2 / y_range;
	m[2][2] = 2 / z_range;
	m[0][3] = -(left + right) / x_range;
	m[1][3] = -(bottom + top) / y_range;
	m[2][3] = -(near + far) / z_range;
	return m;

}


mat4 mat4_perspective(float fovy, float aspect, float near, float far)
{
	mat4 m = mat4::identity();
	fovy = fovy / 180.0 * PI;
	float t = fabs(near) * tan(fovy / 2);
	float r = aspect * t;

	m[0][0] = near / r;
	m[1][1] = near / t;
	m[2][2] = (near + far) / (near - far);
	m[2][3] = 2 * near * far / (far - near);
	m[3][2] = 1;
	m[3][3] = 0;
	return m;
}

float float_clamp(float f, float min, float max)
{
	return f<min ? min : (f>max ? max : f);
}

float float_max(float a, float b)
{
	return a > b ? a : b;
}

float float_min(float a, float b)
{
	return a < b ? a : b;
}

float float_lerp(float start, float end, float alpha)
{
	return (end - start) * alpha + start;
}

vec2 vec2_lerp(vec2& start, vec2& end, float alpha)
{
	return (end - start) * alpha + start;
}

vec3 vec3_lerp(vec3& start, vec3& end, float alpha)
{
	return (end - start) * alpha + start;
}

vec4 vec4_lerp(vec4& start, vec4& end, float alpha)
{
	return start + (end - start) * alpha;
}