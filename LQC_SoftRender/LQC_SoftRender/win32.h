#pragma once
#include <Windows.h>

#include "maths.h"

//用于生成视图的窗口，因为图形显示界面很简单，所以直接使用windows的API
//基本原理：
//在win32中窗口的进程工作主要依赖win32消息循环，os会为窗口维护一个消息队列，
//通过调用PeekMessage()函数我们能从队列中取出队列头部的消息，
//然后用我们自定义的窗口处理函数WindowProc()对每个消息进行处理，
//包括各种响应这种事件，鼠标键盘事件，调用相应的回调函数。



//首先定义鼠标
//左键用于相机的位置移动，右键用于相机的视角移动

typedef struct mouse
{
	//相机的移动
	vec2 orbit_pos;
	vec2 orbit_delta;

	//视角的移动
	vec2 fv_pos;
	vec2 fv_delta;

	//用于鼠标滚轮控制的视角范围大小
	float wheel_delta;

}mouse_t;

//定义窗口
typedef struct window
{
	HWND h_window;//用于窗口的控件
	//HDC是Windows的设备描述表句柄。
	//在Windows环境中，句柄是用来标识被应用程序所建立或使用的对象的唯一整数，
	//	HDC是指窗体、控件的句柄，是长整类型
	HDC mem_dc;

	//是一个特殊的句柄，我们称之为位图句柄，表示设备相关位图在内存中的存储区域代码
	//定义了逻辑位图的高，宽，颜色格式和位值
	HBITMAP bm_old;
	HBITMAP bm_dib;
	unsigned char* window_fb;
	int width;
	int height;
	char keys[512];//用于接收用户输入的信息
	char buttons[2];
	int is_close;
	mouse_t mouse_info;

}window_t;

extern window_t* window;

//窗口初始化函数
int window_init(int width, int height, const char* title);

//窗口销毁函数
int window_destroy();
//窗口绘制函数
void window_draw(unsigned char* framebuffer);
void msg_dispatch();

vec2 get_mouse_pos();//获取鼠标位置
float platform_get_time(void);//获取时间