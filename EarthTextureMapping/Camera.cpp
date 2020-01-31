#include "Camera.h"
#include "Utils.hpp"

#include "glm/glm/gtc/matrix_transform.hpp"


using namespace Utils;
const float toRadians = 3.14f / 180.0f;
Camera::Camera(double p_Yaw, double p_Pitch, glm::vec3 p_Position, glm::vec3 p_CameraUp,
	glm::vec3 p_CameraRight, glm::vec3 p_CameraFront)
	: m_Yaw(p_Yaw), m_Pitch(p_Pitch), m_Position(p_Position), m_CameraUp(p_CameraUp), m_CameraRight(p_CameraRight), m_CameraFront(p_CameraFront)
 { }

glm::mat4 Camera::GetViewMatrix()
{
	return glm::lookAt(m_Position, m_Position + m_CameraFront, m_CameraUp);
}

void Camera::SetYaw(float newYaw) { this->m_Yaw = newYaw; }
void Camera::SetPitch(float newPitch) { this->m_Pitch = newPitch; }
void Camera::SetCameraPosition(glm::vec3 newPosition) { this->m_Position = newPosition; }
// Getters
double Camera::GetYaw() const { return m_Yaw; }
double Camera::GetPitch() const { return m_Pitch; }
glm::vec3& Camera::GetCameraPosition() { return m_Position; }
glm::vec3& Camera::GetCameraUp() { return m_CameraUp; }
glm::vec3& Camera::GetCameraRight() { return m_CameraRight; }
glm::vec3& Camera::GetCameraFront() { return m_CameraFront; }

void Camera::ChangeCamera()
{
	m_CameraFront = glm::vec3(cos(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch)), sin(glm::radians(m_Pitch)), sin(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch)));
	m_CameraFront = glm::normalize(m_CameraFront);

	// m_CameraRight = glm::normalize(glm::cross(m_CameraFront, glm::vec3(0.0f, 1.0f, 0.0f)));
	
	
	// m_CameraUp = glm::normalize(glm::cross(m_CameraRight, m_CameraFront));
	
	// m_CameraRight = glm::normalize(glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), m_CameraFront));
	m_CameraRight = glm::normalize(glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), m_CameraFront));
	m_CameraUp = glm::normalize(glm::cross(m_CameraFront, m_CameraRight));

	this->m_Position += m_CameraFront * this->speed;
}


//