#pragma once
#include <Windows.h>

#include "maths.h"

//����������ͼ�Ĵ��ڣ���Ϊͼ����ʾ����ܼ򵥣�����ֱ��ʹ��windows��API
//����ԭ��
//��win32�д��ڵĽ��̹�����Ҫ����win32��Ϣѭ����os��Ϊ����ά��һ����Ϣ���У�
//ͨ������PeekMessage()���������ܴӶ�����ȡ������ͷ������Ϣ��
//Ȼ���������Զ���Ĵ��ڴ�����WindowProc()��ÿ����Ϣ���д���
//����������Ӧ�����¼����������¼���������Ӧ�Ļص�������



//���ȶ������
//������������λ���ƶ����Ҽ�����������ӽ��ƶ�

typedef struct mouse
{
	//������ƶ�
	vec2 orbit_pos;
	vec2 orbit_delta;

	//�ӽǵ��ƶ�
	vec2 fv_pos;
	vec2 fv_delta;

	//���������ֿ��Ƶ��ӽǷ�Χ��С
	float wheel_delta;

}mouse_t;

//���崰��
typedef struct window
{
	HWND h_window;//���ڴ��ڵĿؼ�
	//HDC��Windows���豸����������
	//��Windows�����У������������ʶ��Ӧ�ó�����������ʹ�õĶ����Ψһ������
	//	HDC��ָ���塢�ؼ��ľ�����ǳ�������
	HDC mem_dc;

	//��һ������ľ�������ǳ�֮Ϊλͼ�������ʾ�豸���λͼ���ڴ��еĴ洢�������
	//�������߼�λͼ�ĸߣ�����ɫ��ʽ��λֵ
	HBITMAP bm_old;
	HBITMAP bm_dib;
	unsigned char* window_fb;
	int width;
	int height;
	char keys[512];//���ڽ����û��������Ϣ
	char buttons[2];
	int is_close;
	mouse_t mouse_info;

}window_t;

extern window_t* window;

//���ڳ�ʼ������
int window_init(int width, int height, const char* title);

//�������ٺ���
int window_destroy();
//���ڻ��ƺ���
void window_draw(unsigned char* framebuffer);
void msg_dispatch();

vec2 get_mouse_pos();//��ȡ���λ��
float platform_get_time(void);//��ȡʱ��