
#include "Window.h"
#include "Camera.h"
#include <iostream>

Window::Window(unsigned int p_Height, unsigned int p_Width, std::string p_Title, Camera* p_Camera,
	GLFWmonitor* p_Monitor, GLFWwindow* p_SharedWindow)
	:	m_Height(p_Height), m_Width(p_Width), m_Title(p_Title), mainCamera(p_Camera), m_Monitor(p_Monitor), m_SharedWindow(p_SharedWindow),
		k_W(nullptr), k_S(nullptr), k_A(nullptr), k_D(nullptr)
{
	k_W = new Key(GLFW_KEY_W, new MouseRotateVerticalUpCommand());
	k_S = new Key(GLFW_KEY_S, new MouseRotateVerticalDownCommand());
	k_D = new Key(GLFW_KEY_D, new MouseRotateHorizontalRightCommand());
	k_A = new Key(GLFW_KEY_A, new MouseRotateHorizontalLeftCommand());
	// Homework
	k_R = new Key(GLFW_KEY_R, new IncreaseHeightFactor());
	k_F = new Key(GLFW_KEY_F, new DecreaseHeightFactor());
	k_Q = new Key(GLFW_KEY_Q, new IncreaseHeightOffset());
	k_E = new Key(GLFW_KEY_E, new DecreaseHeightOffset());
	k_T = new Key(GLFW_KEY_T, new MoveLightDown());
	k_G = new Key(GLFW_KEY_G, new MoveLightUp());
	k_Up = new Key(GLFW_KEY_UP, new MoveLightForward());
	k_Down = new Key(GLFW_KEY_DOWN, new MoveLightBackward());
	k_Right = new Key(GLFW_KEY_RIGHT, new MoveLightLeft());
	k_Left = new Key(GLFW_KEY_LEFT, new MoveLightRight());
	k_Y = new Key(GLFW_KEY_Y, new IncreaseCamSpeed());
	k_H = new Key(GLFW_KEY_H, new DecreaseCamSpeed());

	if (!glfwInit())
		std::cout << "Error\n";

	m_GLWindow = glfwCreateWindow(m_Height, m_Width, m_Title.c_str(), m_Monitor, m_SharedWindow);
	
	glfwMakeContextCurrent(m_GLWindow);
	
	if (glewInit() != GLEW_OK)
		std::cout << "Error!";
	glEnable(GL_DEPTH_TEST);

	glfwSetWindowUserPointer(m_GLWindow, this);

	CreateEventCallbacks();
}

Window::~Window()
{
	// TODO
}

void Window::CreateEventCallbacks()
{
	glfwSetKeyCallback(m_GLWindow, OnKeyPress);
	// glfwSetCursorPosCallback(m_GLWindow, OnMouseTurn);
}

void Window::OnKeyPress(GLFWwindow* targetWindow, int keyCode, int code, int action, int mode)
{
	Window* currentWindow = static_cast<Window*>(glfwGetWindowUserPointer(targetWindow));

	if (action == GLFW_REPEAT)
	{
		switch (keyCode)
		{
		case GLFW_KEY_ESCAPE:
			glfwSetWindowShouldClose(targetWindow, true);
			break;
		case GLFW_KEY_W:
			currentWindow->GetWKey()->Perform(*(currentWindow->GetMainCamera()));
			currentWindow->GetWKey()->SetPressed(true);
			break;
		case GLFW_KEY_S:
			currentWindow->GetSKey()->Perform(*(currentWindow->GetMainCamera()));
			currentWindow->GetSKey()->SetPressed(true);
			break;
		case GLFW_KEY_A:
			currentWindow->GetAKey()->Perform(*(currentWindow->GetMainCamera()));
			currentWindow->GetAKey()->SetPressed(true);
			break;
		case GLFW_KEY_D:
			currentWindow->GetDKey()->Perform(*(currentWindow->GetMainCamera()));
			currentWindow->GetDKey()->SetPressed(true);
			break; 
		case GLFW_KEY_R:
			currentWindow->GetRKey()->Perform(*(currentWindow->GetMainCamera()));
			currentWindow->GetRKey()->SetPressed(true);
			break;
		case GLFW_KEY_F:
			currentWindow->GetFKey()->Perform(*(currentWindow->GetMainCamera()));
			currentWindow->GetFKey()->SetPressed(true);
			break;
		case GLFW_KEY_Q:
			currentWindow->GetQKey()->Perform(*(currentWindow->GetMainCamera()));
			currentWindow->GetQKey()->SetPressed(true);
			break;
		case GLFW_KEY_E:
			currentWindow->GetEKey()->Perform(*(currentWindow->GetMainCamera()));
			currentWindow->GetEKey()->SetPressed(true);
			break;
		case GLFW_KEY_G:
			currentWindow->GetGKey()->Perform(*(currentWindow->GetMainCamera()));
			currentWindow->GetGKey()->SetPressed(true);
			break;
		case GLFW_KEY_T:
			currentWindow->GetTKey()->Perform(*(currentWindow->GetMainCamera()));
			currentWindow->GetTKey()->SetPressed(true);
			break;
		case GLFW_KEY_UP:
			currentWindow->GetUpKey()->Perform(*(currentWindow->GetMainCamera()));
			currentWindow->GetUpKey()->SetPressed(true);
			break;
		case GLFW_KEY_DOWN:
			currentWindow->GetDownKey()->Perform(*(currentWindow->GetMainCamera()));
			currentWindow->GetDownKey()->SetPressed(true);
			break;
		case GLFW_KEY_RIGHT:
			currentWindow->GetRightKey()->Perform(*(currentWindow->GetMainCamera()));
			currentWindow->GetRightKey()->SetPressed(true);
			break;
		case GLFW_KEY_LEFT:
			currentWindow->GetLeftKey()->Perform(*(currentWindow->GetMainCamera()));
			currentWindow->GetLeftKey()->SetPressed(true);
			break;
		case GLFW_KEY_Y:
			currentWindow->GetYKey()->Perform(*(currentWindow->GetMainCamera()));
			currentWindow->GetYKey()->SetPressed(true);
			break;
		case GLFW_KEY_H:
			currentWindow->GetHKey()->Perform(*(currentWindow->GetMainCamera()));
			currentWindow->GetHKey()->SetPressed(true);
			break;
		default: break;
		}
	}

	else if (action == GLFW_RELEASE)
	{
		switch (keyCode)
		{
		case GLFW_KEY_W:
			currentWindow->GetWKey()->SetPressed(false);
			break;
		case GLFW_KEY_S:
			currentWindow->GetSKey()->SetPressed(false);
			break;
		case GLFW_KEY_A:
			currentWindow->GetAKey()->SetPressed(false);
			break;
		case GLFW_KEY_D:
			currentWindow->GetDKey()->SetPressed(false);
			break; 
		case GLFW_KEY_R:
			currentWindow->GetRKey()->SetPressed(false);
			break;
		case GLFW_KEY_F:
			currentWindow->GetFKey()->SetPressed(false);
			break;
		case GLFW_KEY_Q:
			currentWindow->GetQKey()->SetPressed(false);
			break;
		case GLFW_KEY_E:
			currentWindow->GetEKey()->SetPressed(false);
			break;
		case GLFW_KEY_G:
			currentWindow->GetGKey()->SetPressed(false);
			break;
		case GLFW_KEY_T:
			currentWindow->GetTKey()->SetPressed(false);
			break;
		case GLFW_KEY_UP:
			currentWindow->GetUpKey()->SetPressed(false);
			break;
		case GLFW_KEY_DOWN:
			currentWindow->GetDownKey()->SetPressed(false);
			break;
		case GLFW_KEY_RIGHT:
			currentWindow->GetRightKey()->SetPressed(false);
			break;
		case GLFW_KEY_LEFT:
			currentWindow->GetLeftKey()->SetPressed(false);
			break;
		case GLFW_KEY_Y:
			currentWindow->GetYKey()->SetPressed(false);
			break;
		case GLFW_KEY_H:
			currentWindow->GetHKey()->SetPressed(false);
			break;
		default: break;
		}
	}
}

void Window::MakeCurrentContext() 
{ 
	//glfwSwapInterval(m_SwapInterval);
	glfwMakeContextCurrent(m_GLWindow); 
}

// Setters
void Window::SetWindowStatus(bool newStatus) { windowStatus = newStatus; }

// Main loop
void Window::Refresh()
{
	glfwSwapBuffers(m_GLWindow);
	glfwPollEvents();
}