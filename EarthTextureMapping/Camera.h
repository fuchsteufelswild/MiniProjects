#ifndef _CAMERA_
#define _CAMERA_

#include "glm/glm/glm.hpp"

class Camera
{
private: // Camera Attributes
	double m_Yaw, m_Pitch;
	glm::vec3 m_Position;
	glm::vec3 m_CameraUp;
	glm::vec3 m_CameraRight;
	glm::vec3 m_CameraFront;
public:
	// Homework
	float heightFactor = 0;
	float heightOffset = 0;
	glm::vec3 lightPosition;
	float speed = 0;
public:
	Camera(double p_Yaw = -90.0f, double p_Pitch = 0, glm::vec3 p_Position = glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3 p_CameraUp = glm::vec3(0.0f, 1.0f, 0.0f),
		glm::vec3 p_CameraRight = glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3 p_CameraFront = glm::vec3(0.0f, 0.0f, -1.0f));
	~Camera() = default;

	glm::mat4 GetViewMatrix();
public:
	void SetCameraPosition(glm::vec3 newPosition);
	// HW
	void SetYaw(float newYaw);
	void SetPitch(float newPitch);
public:
	double GetYaw() const;
	double GetPitch() const;
	glm::vec3& GetCameraPosition();
	glm::vec3& GetCameraUp();
	glm::vec3& GetCameraRight();
	glm::vec3& GetCameraFront();

	// Homework Related
	void ChangeCamera();
};



#endif