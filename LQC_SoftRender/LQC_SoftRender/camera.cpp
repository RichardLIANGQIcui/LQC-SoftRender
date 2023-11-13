#include "camera.h"
#include "win32.h"

Camera::Camera(vec3 e, vec3 t, vec3 up, float aspect) :
	eye(e), target(t), up(up), aspect(aspect) {
}

Camera::~Camera() {

}

//所谓更新相机坐标，就是利用鼠标输入的信息来更新相机所在的位置
// 实际上就是鼠标在x，y方向上如何移动，就按一定比例关系移动相机
//这里相机的坐标使用了球面坐标系求解
void update_camera_pos(Camera& camera)
{
	//获取此刻相机的球面坐标系
	vec3 from_target = camera.eye - camera.target;
	float radius = from_target.norm();
	float phi = (float)atan2(from_target[0], from_target[2]);
	float theta=(float)acos(from_target[1] / radius);
	float x_delta = window->mouse_info.orbit_delta[0] / window->width;
	float y_delta = window->mouse_info.orbit_delta[1] / window->height;

	//用户滚动鼠标滚轮实际上就是改变半径大小
	radius *= (float)pow(0.95, window->mouse_info.wheel_delta);

	float factor = 1.5 * PI;//改变速率
	//当按下鼠标左键是旋转相机，改变相机的球面坐标
	phi += x_delta * factor;
	theta +=y_delta* factor;
	if (theta > PI)
	{
		theta = PI - EPSILON * 100;
	}
	if (theta < 0)
	{
		theta = EPSILON * 100;
	}
	//因为按动了鼠标滚轮所以，相机的位置也发生相应的变化
	camera.eye[0] = camera.target[0] + radius * sin(phi) * sin(theta);
	camera.eye[1] = camera.target[1] + radius * cos(theta);
	camera.eye[2] = camera.target[2] + radius * sin(theta) * cos(phi);

	//当按下鼠标右键则是平移
	factor = radius * (float)tan(60.0 / 360 * PI) * 2.2;
	//捕捉用户分别在x和y方向上移动的距离再应用到相机上
	x_delta = window->mouse_info.fv_delta[0] / window->width;
	y_delta = window->mouse_info.fv_delta[1] / window->height;
	vec3 left = x_delta * factor * camera.x;
	vec3 up = y_delta * factor * camera.y;

	camera.eye += (left - up);
	camera.target += (left - up);

}

void handle_mouse_events(Camera& camera)
{
	if (window->buttons[0])
	{
		vec2 cur_pos = get_mouse_pos();
		window->mouse_info.orbit_delta = window->mouse_info.orbit_pos - cur_pos;
		window->mouse_info.orbit_pos = cur_pos;
	}

	if (window->buttons[1])
	{
		vec2 cur_pos = get_mouse_pos();
		window->mouse_info.fv_delta = window->mouse_info.fv_pos - cur_pos;
		window->mouse_info.fv_pos = cur_pos;
	}

	update_camera_pos(camera);
}

void handle_key_events(Camera& camera)
{
	float distance = (camera.target - camera.eye).norm();
	if (window->keys['W'])//相机向物体移动
	{
		camera.eye += -10.0 / window->width * camera.z * distance;
	}
	if (window->keys['S'])//相机远离物体移动
	{
		camera.eye += 0.05f * camera.z ;
	}
	if (window->keys[VK_UP] || window->keys['Q'])
	{
		camera.eye += 0.05f * camera.y;
		camera.target += 0.05f * camera.y;
	}
	if (window->keys[VK_DOWN] || window->keys['E'])
	{
		camera.eye += -0.05f * camera.y;
		camera.target += -0.05f * camera.y;
	}
	if (window->keys[VK_LEFT] || window->keys['A'])
	{
		camera.eye += -0.05f * camera.x;
		camera.target += -0.05f * camera.x;
	}
	if (window->keys[VK_RIGHT] || window->keys['D'])
	{
		camera.eye += 0.05f * camera.x;
		camera.target += 0.05f * camera.x;
	}
	if (window->keys[VK_ESCAPE])
	{
		window->is_close = 1;
	}

}


void handle_events(Camera& camera)
{
	//计算相机坐标系
	camera.z = unit_vector(camera.eye - camera.target);//相机的朝向为z方向
	camera.x = unit_vector(cross(camera.up, camera.z));
	camera.y = unit_vector(cross(camera.z, camera.x));

	handle_mouse_events(camera);
	handle_key_events(camera);

}


