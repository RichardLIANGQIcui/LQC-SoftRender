#include "./tgaimage.h"

#include <ctime>
#include <cmath>
#include <cstring>
#include <iostream>
#include <fstream>

TGAImage::TGAImage() : data(NULL), width(0), height(0), bytespp(0)
{
}

TGAImage::TGAImage(int w, int h, int bpp) : data(NULL), width(w), height(h), bytespp(bpp)
{
	unsigned long nbytes = width * height * bytespp;
	data = new unsigned char[nbytes];
	memset(data, 0, nbytes);
}

TGAImage::TGAImage(const TGAImage& img) : data(NULL), width(img.width), height(img.height), bytespp(img.bytespp)
{
	unsigned long nbytes = width * height * bytespp;
	data = new unsigned char[nbytes];
	memcpy(data, img.data, nbytes);
}

TGAImage::~TGAImage()
{
	if (data) delete[] data;
}

TGAImage& TGAImage::operator =(const TGAImage& img)
{
	if (this != &img)
	{
		if (data) delete[] data;
		width = img.width;
		height = img.height;
		bytespp = img.bytespp;
		unsigned long nbytes = width * height * bytespp;
		data = new unsigned char[nbytes];
		memcpy(data, img.data, nbytes);
	}
	return *this;
}

//读取未压缩文件
bool TGAImage::read_tga_file(const char* filename)//filename是文件路径
{
	if (data) delete[] data;
	data = NULL;
	std::ifstream in;
	in.open(filename, std::ios::binary);//TGA文件是二进制文件
	if (!in.is_open())//检查是否打开正确
	{
		std::cerr << "can't open file " << filename << "\n";
		in.close();
		return false;
	}
	//尝试读取文件的(首12个字节)的内容并且将它们存储在我们的TGAHeader结构中便于检查文件类型
	//
	TGA_Header header;
	in.read((char*)&header, sizeof(header));// 这个header是char*&类型，下面data是char*类型
	if (!in.good())
	{
		in.close();
		std::cerr << "an error occured while reading the header\n";
		return false;
	}


	width = header.width;
	height = header.height;
	bytespp = header.bitsperpixel >> 3;
	//我们需要确认高度和宽度至少为1个像素，并且bpp是24或32
	//如果这些值中的任何一个超出了它们的界限，我们将再一次显示一个错误，关闭文件，并且离开此函数。
	if (width <= 0 || height <= 0 || (bytespp != GRAYSCALE && bytespp != RGB && bytespp != RGBA))
	{
		in.close();
		std::cerr << "bad bpp (or width/height) value\n";
		return false;
	}

	unsigned long nbytes = bytespp * width * height;
	data = new unsigned char[nbytes];
	//接着，通过我们编的程序刚读取的头，我们继续尝试确定文件类型
	//这可以告诉我们它是压缩的、未压缩甚至是错误的文件类型。
	if (3 == header.datatypecode || 2 == header.datatypecode)
	{
		in.read((char*)data, nbytes);
		if (!in.good())
		{
			in.close();
			std::cerr << "an error occured while reading the data\n";
			return false;
		}
	}
	else if (10 == header.datatypecode || 11 == header.datatypecode)
	{
		if (!load_rle_data(in))
		{
			in.close();
			std::cerr << "an error occured while reading the data\n";
			return false;
		}
	}
	else
	{
		in.close();
		std::cerr << "unknown file format " << (int)header.datatypecode << "\n";
		return false;
	}

	//接下来我们设置图像的类型。24 bit图像是GL_RGB，32 bit 图像是GL_RGBA。
	if (!(header.imagedescriptor & 0x20))
	{
		flip_vertically();
	}
	if (header.imagedescriptor & 0x10)
	{
		flip_horizontally();
	}
	in.close();
	return true;
}


//读取压缩过的文件

bool TGAImage::load_rle_data(std::ifstream& in)
{
	/*我们也需要存储当前所处的像素，
		以及我们正在写入的图像数据的字节，这样避免溢出写入过多的旧数据。*/
	unsigned long pixelcount = width * height;// 图像中的像素数
	unsigned long currentpixel = 0; // 当前正在读取的像素
	unsigned long currentbyte = 0;// 当前正在向图像中写入的像素
	TGAColor colorbuffer;// 一个像素的存储空间
	
	do
	{
		unsigned char chunkheader = 0;//声明一个变量来存储“块”头
		//块头指示接下来的段是RLE还是RAW，它的长度是多少。
		/*如果一字节头小于等于127，则它是一个RAW头。*/
		chunkheader = in.get();
		if (!in.good())
		{
			std::cerr << "an error occured while reading the data\n";
			return false;
		}
		if (chunkheader < 128) // 如果是RAW块
		{
			chunkheader++;// 变量值加1以获取RAW像素的总数
			//--开始像素读取循环--
			for (int i = 0; i < chunkheader; i++)//每次循环读取和存储一个像素。
			{
				//--尝试读取一个像素--
				in.read((char*)colorbuffer.bgra, bytespp);
				if (!in.good())
				{
					std::cerr << "an error occured while reading the header\n";
					return false;
				}
				for (int t = 0; t < bytespp; t++)
					data[currentbyte++] = colorbuffer.bgra[t];
				currentpixel++;
				if (currentpixel > pixelcount)
				{
					std::cerr << "Too many pixels read\n";
					return false;
				}
			}
		}
		else
		{
			/*如果头大于127，那么它是下一个像素值随后将要重复的次数。
			要获取实际重复的数量，我们将它减去127以除去1bit的的头标示符*/
			chunkheader -= 127;
			in.read((char*)colorbuffer.bgra, bytespp);
			if (!in.good())
			{
				std::cerr << "an error occured while reading the header\n";
				return false;
			}
			for (int i = 0; i < chunkheader; i++)
			{
				for (int t = 0; t < bytespp; t++)
					data[currentbyte++] = colorbuffer.bgra[t];
				currentpixel++;
				if (currentpixel > pixelcount)
				{
					std::cerr << "Too many pixels read\n";
					return false;
				}
			}
		}
	} while (currentpixel < pixelcount);
	return true;
}

bool TGAImage::write_tga_file(const char* filename, bool rle)
{
	unsigned char developer_area_ref[4] = { 0, 0, 0, 0 };
	unsigned char extension_area_ref[4] = { 0, 0, 0, 0 };
	unsigned char footer[18] = { 'T','R','U','E','V','I','S','I','O','N','-','X','F','I','L','E','.','\0' };
	std::ofstream out;
	out.open(filename, std::ios::binary);
	if (!out.is_open())
	{
		std::cerr << "can't open file " << filename << "\n";
		out.close();
		return false;
	}
	TGA_Header header;
	memset((void*)&header, 0, sizeof(header));
	header.bitsperpixel = bytespp << 3;
	header.width = width;
	header.height = height;
	header.datatypecode = (bytespp == GRAYSCALE ? (rle ? 11 : 3) : (rle ? 10 : 2));
	header.imagedescriptor = 0x20; // top-left origin
	out.write((char*)&header, sizeof(header));
	if (!out.good())
	{
		out.close();
		std::cerr << "can't dump the tga file\n";
		return false;
	}
	if (!rle)
	{

		out.write((char*)data, width * height * bytespp);
		if (!out.good())
		{
			std::cerr << "can't unload raw data\n";
			out.close();
			return false;
		}
	}
	else
	{
		if (!unload_rle_data(out))
		{
			out.close();
			std::cerr << "can't unload rle data\n";
			return false;
		}
	}
	out.write((char*)developer_area_ref, sizeof(developer_area_ref));
	if (!out.good())
	{
		std::cerr << "can't dump the tga file\n";
		out.close();
		return false;
	}
	out.write((char*)extension_area_ref, sizeof(extension_area_ref));
	if (!out.good())
	{
		std::cerr << "can't dump the tga file\n";
		out.close();
		return false;
	}
	out.write((char*)footer, sizeof(footer));
	if (!out.good())
	{
		std::cerr << "can't dump the tga file\n";
		out.close();
		return false;
	}
	out.close();
	return true;
}

// TODO: it is not necessary to break a raw chunk for two equal pixels (for the matter of the resulting size)
bool TGAImage::unload_rle_data(std::ofstream& out)
{
	const unsigned char max_chunk_length = 128;
	unsigned long npixels = width * height;
	unsigned long curpix = 0;
	while (curpix < npixels)
	{
		unsigned long chunkstart = curpix * bytespp;
		unsigned long curbyte = curpix * bytespp;
		unsigned char run_length = 1;
		bool raw = true;
		while (curpix + run_length < npixels && run_length < max_chunk_length)
		{
			bool succ_eq = true;
			for (int t = 0; succ_eq && t < bytespp; t++)
			{
				succ_eq = (data[curbyte + t] == data[curbyte + t + bytespp]);
			}
			curbyte += bytespp;
			if (1 == run_length)
			{
				raw = !succ_eq;
			}
			if (raw && succ_eq)
			{
				run_length--;
				break;
			}
			if (!raw && !succ_eq)
			{
				break;
			}
			run_length++;
		}
		curpix += run_length;
		out.put(raw ? run_length - 1 : run_length + 127);
		if (!out.good())
		{
			std::cerr << "can't dump the tga file\n";
			return false;
		}
		out.write((char*)(data + chunkstart), (raw ? run_length * bytespp : bytespp));
		if (!out.good())
		{
			std::cerr << "can't dump the tga file\n";
			return false;
		}
	}
	return true;
}

TGAColor TGAImage::get(int x, int y)
{
	if (!data || x < 0 || y < 0 || x >= width || y >= height)
	{
		return TGAColor();
	}
	return TGAColor(data + (x + y * width) * bytespp, bytespp);
}

bool TGAImage::set(int x, int y, TGAColor& c)
{
	if (!data || x < 0 || y < 0 || x >= width || y >= height)
	{
		return false;
	}
	memcpy(data + (x + y * width) * bytespp, c.bgra, bytespp);
	return true;
}

bool TGAImage::set(int x, int y, const TGAColor& c)
{
	if (!data || x < 0 || y < 0 || x >= width || y >= height)
	{
		return false;
	}
	memcpy(data + (x + y * width) * bytespp, c.bgra, bytespp);
	return true;
}

int TGAImage::get_bytespp()
{
	return bytespp;
}

int TGAImage::get_width()
{
	return width;
}

int TGAImage::get_height()
{
	return height;
}

bool TGAImage::flip_horizontally()
{
	if (!data) return false;
	int half = width >> 1;
	for (int i = 0; i < half; i++)
	{
		for (int j = 0; j < height; j++)
		{
			TGAColor c1 = get(i, j);
			TGAColor c2 = get(width - 1 - i, j);
			set(i, j, c2);
			set(width - 1 - i, j, c1);
		}
	}
	return true;
}

bool TGAImage::flip_vertically()
{
	if (!data) return false;
	unsigned long bytes_per_line = width * bytespp;
	unsigned char* line = new unsigned char[bytes_per_line];
	int half = height >> 1;
	for (int j = 0; j < half; j++)
	{
		unsigned long l1 = j * bytes_per_line;
		unsigned long l2 = (height - 1 - j) * bytes_per_line;
		memmove((void*)line, (void*)(data + l1), bytes_per_line);
		memmove((void*)(data + l1), (void*)(data + l2), bytes_per_line);
		memmove((void*)(data + l2), (void*)line, bytes_per_line);
	}
	delete[] line;
	return true;
}

unsigned char* TGAImage::buffer()
{
	return data;
}

void TGAImage::clear()
{
	memset((void*)data, 0, width * height * bytespp);
}

bool TGAImage::scale(int w, int h)
{
	if (w <= 0 || h <= 0 || !data) return false;
	unsigned char* tdata = new unsigned char[w * h * bytespp];
	int nscanline = 0;
	int oscanline = 0;
	int erry = 0;
	unsigned long nlinebytes = w * bytespp;
	unsigned long olinebytes = width * bytespp;
	for (int j = 0; j < height; j++)
	{
		int errx = width - w;
		int nx = -bytespp;
		int ox = -bytespp;
		for (int i = 0; i < width; i++)
		{
			ox += bytespp;
			errx += w;
			while (errx >= (int)width)
			{
				errx -= width;
				nx += bytespp;
				memcpy(tdata + nscanline + nx, data + oscanline + ox, bytespp);
			}
		}
		erry += h;
		oscanline += olinebytes;
		while (erry >= (int)height)
		{
			if (erry >= (int)height << 1) // it means we jump over a scanline
				memcpy(tdata + nscanline + nlinebytes, tdata + nscanline, nlinebytes);
			erry -= height;
			nscanline += nlinebytes;
		}
	}
	delete[] data;
	data = tdata;
	width = w;
	height = h;
	return true;
}