#define GLEW_STATIC

#include <iostream>
#include <string>
#include <fstream>
#include <jpeglib.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "glm/glm/glm.hpp"
#include "glm/glm/gtc/matrix_transform.hpp"
#include "glm/glm/gtc/type_ptr.hpp"
#include "Window.h"
#include "Camera.h"
#include "Material.h"
#include <sstream>
#include <vector>
#include <algorithm>
#include "helper.h"

void SetSphereVertex(float* pos, float x, float y)
{
	pos[0] = x;
	pos[1] = y;
}


void FillSpherePositionInfo(float* vertices, int verticalSplit, int horizontalSplit)
{
	float* iterate = vertices;
	
	// for (int i = 0; i < 125; ++i)
	// {
	// 	for (int j = 0; j < 250; ++j)
	// 	{
	// 		SetSphereVertex(iterate, j, i);
	// 		SetSphereVertex(iterate + 2, j + 1, i);
	// 		SetSphereVertex(iterate + 4, j, i + 1);
	// 		SetSphereVertex(iterate + 6, j + 1, i);
	// 		SetSphereVertex(iterate + 8, j + 1, i + 1);
	// 		SetSphereVertex(iterate + 10, j, i + 1);

	// 		iterate += 12;
	// 	}
	// }
	// return;
	
	
	iterate = vertices;
	int horizontal_step = 250;
	int vertical_step = 125;
	for (int i = 0; i < horizontal_step; i++) { // vertical step is 1 here (kuzey kutbu)
		SetSphereVertex(iterate, 0, 0);
		SetSphereVertex(iterate + 2, i, 1);
		SetSphereVertex(iterate + 4, (i + 1) % 250, 1); // for the last triangle we have a circle
		iterate += 6;
	}
	for (int i = 2; i < vertical_step - 1; i++) {//deacreasing z
		for (int j = 0; j < horizontal_step; j++) { //increasing z
			//2 triangles at a time
			SetSphereVertex(iterate, j, i);
			SetSphereVertex(iterate + 2, j, i - 1);
			SetSphereVertex(iterate + 4, (j + 1) % 250, i - 1);

			SetSphereVertex(iterate + 6, j, i);
			SetSphereVertex(iterate + 8, (j + 1) % 250, i);
			SetSphereVertex(iterate + 10, (j + 1) % 250, i - 1);
			iterate += 12;
		}
	}
	for (int i = 0; i < horizontal_step; i++) { // vertical step is 124 here (güney kutbu)
		SetSphereVertex(iterate, i, 125);//güney kutup noktasý
		SetSphereVertex(iterate + 2, i, 124);
		SetSphereVertex(iterate + 4, (i + 1) % 250, 124); // for the last triangle we may  have a circle
		iterate += 6;
	}
}

int main(int argc, char* argv[])
{
    Camera cam(90.0f, -90.0f, glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	Window mainWindow(1000, 1000, "Hello", &cam);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);

    int verticalSplit = 125;
	int horizontalSplit = 250;
    unsigned int vao;
    unsigned int vbo;
    int w, h;
    unsigned int earthTextureID;
    unsigned int heightMapID;
    initTexture(argv[1], &w, &h, heightMapID);
    initTexture(argv[2], &w, &h, earthTextureID);


    glm::mat4 projectionMatrix = glm::perspective(45.0f, 1.0f, 0.1f, 1000.0f);
	glm::mat4 modelMatrix(1);

	mainWindow.GetMainCamera()->SetCameraPosition(glm::vec3(0.0f, 600.0f, .0f));

	mainWindow.GetMainCamera()->lightPosition = glm::vec3(0.0f, 1600.0f, 0.0f);

    // Material material("FlatEarthShader.shader");
	Material material("SphericalEarthShader.shader");
	material.Bind();
    unsigned int prID = material.GetProgramID();
	glUseProgram(prID);

    int loc = glGetUniformLocation(prID, "width");
	glUniform1f(loc, w);

	loc = glGetUniformLocation(prID, "height");
	glUniform1f(loc, h);
	
	loc = glGetUniformLocation(prID, "verticalSplit");
	glUniform1f(loc, verticalSplit);

	loc = glGetUniformLocation(prID, "horizontalSplit");
	glUniform1f(loc, horizontalSplit);

	glm::mat4 viewMat;

	loc = glGetUniformLocation(prID, "u_Model");
	glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(modelMatrix));

	loc = glGetUniformLocation(prID, "u_Projection");
	glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(projectionMatrix));

	float* spherePos = new float[12 * 250 * 125];
	FillSpherePositionInfo(spherePos, verticalSplit, horizontalSplit);


    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, 48 * 125 * 250, spherePos, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 8, (const void *)0);
    glUseProgram(prID);

    while (!glfwWindowShouldClose(mainWindow.GetWindow()))
	{
		mainWindow.GetMainCamera()->ChangeCamera();
		loc = glGetUniformLocation(prID, "heightFactor");
		glUniform1f(loc, mainWindow.GetMainCamera()->heightFactor);

		loc = glGetUniformLocation(prID, "heightMapOffset");
		glUniform1f(loc, mainWindow.GetMainCamera()->heightOffset);

		glm::vec3 regulatedCam = glm::vec3(mainWindow.GetMainCamera()->GetCameraPosition());
		
		loc = glGetUniformLocation(prID, "camPosition");
		glUniform3f(loc, regulatedCam.x, regulatedCam.y, regulatedCam.z);

		

		viewMat = mainWindow.GetMainCamera()->GetViewMatrix();
		loc = glGetUniformLocation(prID, "u_View");
		glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(viewMat));

		loc = glGetUniformLocation(prID, "lightPosition");
		glUniform3f(loc, mainWindow.GetMainCamera()->lightPosition.x, mainWindow.GetMainCamera()->lightPosition.y, mainWindow.GetMainCamera()->lightPosition.z);


		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glUseProgram(material.GetProgramID());
        glActiveTexture(GL_TEXTURE0);
	    glBindTexture(GL_TEXTURE_2D, heightMapID);

        glActiveTexture(GL_TEXTURE1);
	    glBindTexture(GL_TEXTURE_2D, earthTextureID);

		loc = glGetUniformLocation(prID, "heightMap");
		glUniform1i(loc, 0);
		loc = glGetUniformLocation(prID, "textureSampler");
		glUniform1i(loc, 1);

		glDrawArrays(GL_TRIANGLES, 0, 12 * w * h);

		glActiveTexture(GL_TEXTURE0);
	    glBindTexture(GL_TEXTURE_2D, 0);

		glActiveTexture(GL_TEXTURE1);
	    glBindTexture(GL_TEXTURE_2D, 0);
	    mainWindow.Refresh(); // Clear buffers
	}

    glfwTerminate();
}
