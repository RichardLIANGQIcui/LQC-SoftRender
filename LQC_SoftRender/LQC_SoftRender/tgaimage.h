#pragma once
#include <fstream>

#pragma pack(push,1)
//建立颜色和图像结构体

using namespace std;

//用来表示文件头，文件头用来决定文件类型。
struct TGA_Header
{
	char idlength;
	char colormaptype;
	char datatypecode;
	short colormaporigin;
	short colormaplength;
	char colormapdepth;
	short x_origin;
	short y_origin;
	short width;
	short height;
	char  bitsperpixel;
	char  imagedescriptor;
};
#pragma pack(pop)

//首先最基本的颜色是由4个无符号整形构成，颜色就有获取某个RGB值，还有初始化
//还有颜色值乘上某个强度变大或变小

struct TGAColor
{
	unsigned char bgra[4];
	unsigned char bytespp;

	TGAColor():bgra(), bytespp(1)
	{
		for (int i = 0;i < 4;i++)
		{
			bgra[i] = 0;
		}
	}

	TGAColor(unsigned char R, unsigned char G, unsigned char B, unsigned char A = 255):bgra(),bytespp(4)
	{
		bgra[0] = B;
		bgra[1] = G;
		bgra[2] = R;
		bgra[3] = A;

	}

	TGAColor(unsigned char v) :bgra(), bytespp(1)
	{
		for (int i = 0;i < 4;i++)
		{
			bgra[i] = 0;
		}
		bgra[0] = v;
	}

	TGAColor(const unsigned char* p, unsigned char bpp) :bgra(), bytespp(bpp)
	{
		for (int i = 0;i < 4;i++)
		{
			bgra[i] = p[i];
		}
		for (int i = bpp;i < 4;i++)
		{
			bgra[i] = 0;
		}
	}

	unsigned char& operator[](const int i)
	{
		return bgra[i];
	}

	TGAColor operator*(float intensity) const
	{
		TGAColor res = *this;
		intensity = (intensity > 1.f ? 1.f : (intensity < 0.f ? 0.f : intensity));
		for (int i = 0;i < 4;i++)
		{
			res.bgra[i] *= intensity;
		}
		return res;
	}

};


//图像的定义
//图像要有长宽、数组存放数据，从图像读出数据和将数据存入图像的函数
//图像采用的格式、竖直和水平作滤波操作、缩放图像操作，设置获取图像某个位置上的颜色
//清空图像数据
class TGAImage
{
protected:
	unsigned char* data;
	int width;
	int height;
	int bytespp;//每个像素存储大小，一般是4个整形，对应RGBA

	bool load_rle_data(ifstream& in);
	bool unload_rle_data(ofstream& out);

public:
	enum Format
	{
		GRAYSCALE=1,RGB=3,RGBA=4
	};

	TGAImage();
	TGAImage(int w, int h, int bpp);
	TGAImage(const TGAImage& img);
	~TGAImage();
	bool read_tga_file(const char* filename);
	bool write_tga_file(const char* filename, bool rle = true);
	bool flip_horizontally();
	bool flip_vertically();
	bool scale(int w, int h);
	TGAColor get(int x, int y);
	bool set(int x, int y, TGAColor& c);
	bool set(int x, int y, const TGAColor& c);
	TGAImage& operator =(const TGAImage& img);
	int get_width();
	int get_height();
	int get_bytespp();
	unsigned char* buffer();
	void clear();


};

