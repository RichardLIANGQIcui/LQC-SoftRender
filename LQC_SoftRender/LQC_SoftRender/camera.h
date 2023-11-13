#pragma once

#include "maths.h"

//����Ķ���
//���λ�á�Ŀ��λ�á���������ϵ�����ݱ����ڼ���ͶӰ����

class Camera
{
public:
	Camera(vec3 eye, vec3 t, vec3 up, float aspect);
	~Camera();

	vec3 eye;
	vec3 target;
	vec3 up;
	vec3 x;
	vec3 y;
	vec3 z;
	float aspect;

};

//����������������ڴ����û�������Ŀ���
void update_camera_pos(Camera& camera);
void handle_events(Camera& camera);




