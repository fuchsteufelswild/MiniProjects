#ifndef _WINDOW_
#define _WINDOW_

#include "KeyCommand.h"

#include <string>


#include <GL/glew.h>
#include <GLFW/glfw3.h>

class Window
{
private:
	unsigned int m_Height, m_Width; // Size of the window
	std::string m_Title; // Title of the window
	bool windowStatus = false; // Control window shutdown ( true -> close the window )
	GLFWwindow* m_SharedWindow; 	
	GLFWmonitor* m_Monitor;
	unsigned int m_SwapInterval = 1; // Swap interval to regulate control time
private:
	Key* k_W;
	Key* k_S;
	Key* k_A;
	Key* k_D;

	// Homework
	Key* k_R;
	Key* k_F;
	Key* k_Q;
	Key* k_E;
	Key* k_T;
	Key* k_G;
	Key* k_Up;
	Key* k_Down;
	Key* k_Right;
	Key* k_Left;
	Key* k_Y;
	Key* k_H;
private:
	Camera* mainCamera;
	GLFWwindow* m_GLWindow; // MainWindow
public:
	Window(unsigned int p_Height = 640, unsigned int p_Width = 480, std::string p_Title = "", Camera* p_Camera = nullptr,
		   GLFWmonitor* p_Monitor = nullptr, GLFWwindow* p_SharedWindow = nullptr);
	~Window();

	void Refresh();
	void MakeCurrentContext();
	void CreateEventCallbacks();
public:
	void SetWindowStatus(bool newStatus);
public:
	inline bool GetWindowStatus() const { return windowStatus; }
	inline unsigned int GetHeight() const { return m_Height; }
	inline unsigned int GetWidth() const { return m_Width; }
	inline std::string GetTitle() const { return m_Title; }
	inline GLFWwindow* GetWindow() { return m_GLWindow; }
	inline Camera* GetMainCamera() { return mainCamera; }

	inline Key* GetWKey() { return k_W; }
	inline Key* GetSKey() { return k_S; }
	inline Key* GetAKey() { return k_A; }
	inline Key* GetDKey() { return k_D; }
	// Homework
	inline Key* GetRKey() { return k_R; }
	inline Key* GetFKey() { return k_F; }
	inline Key* GetQKey() { return k_Q; }
	inline Key* GetEKey() { return k_E; }
	inline Key* GetGKey() { return k_G; }
	inline Key* GetTKey() { return k_T; }
	inline Key* GetUpKey() { return k_Up; }
	inline Key* GetDownKey() { return k_Down; }
	inline Key* GetRightKey() { return k_Right; }
	inline Key* GetLeftKey() { return k_Left; }
	inline Key* GetYKey() { return k_Y; }
	inline Key* GetHKey() { return k_H; }
	
public:
	static void OnKeyPress(GLFWwindow* targetWindow, int keyCode, int code, int action, int mode);
	static void OnMouseTurn(GLFWwindow* targetWindow, double mouseX, double mouseY);
};

#endif