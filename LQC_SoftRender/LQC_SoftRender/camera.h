#pragma once

#include "maths.h"

//相机的定义
//相机位置、目标位置、向上坐标系、横纵比用于计算投影矩阵

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

//处理相机函数，用于处理用户对相机的控制
void update_camera_pos(Camera& camera);
void handle_events(Camera& camera);




