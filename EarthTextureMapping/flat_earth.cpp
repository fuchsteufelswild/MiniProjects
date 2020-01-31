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


void SetPosition(float* pos, float x, float y, float z, int w, int h)
{
	pos[0] = x;
	pos[1] = y;
	pos[2] = z;
	pos[3] = x / w;
	pos[4] = z / h;

    //std::cout << pos[3] << " " << pos[4] << std::endl;
}


void FillPositionInfo(float* vertices, int w, int h) 
{
	float* iterate = vertices;
	for (int i = 0; i < h; i++) {//increasing z
		for (int j = 0; j < w; j++) { //increasing x

			SetPosition(iterate, j, 0, i, w, h);
			SetPosition(iterate + 5, j + 1, 0, i, w, h);
			SetPosition(iterate + 10, j, 0, i + 1, w, h);
			SetPosition(iterate + 15, j + 1, 0, i, w, h);
			SetPosition(iterate + 20, j + 1, 0, i + 1, w, h);
			SetPosition(iterate + 25, j, 0, i + 1, w, h);
            
			//set 2 triangles for each 
			iterate += 30;
		}
	}
}


int main(int argc, char* argv[])
{
    Camera cam(90.0f, 0.0f, glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	Window mainWindow(1000, 1000, "Hello", &cam);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);

    unsigned int vao;
    unsigned int vbo;
    int w, h;
    unsigned int earthTextureID;
    unsigned int heightMapID;
    initTexture(argv[1], &w, &h, heightMapID);
    initTexture(argv[2], &w, &h, earthTextureID);


    glm::mat4 projectionMatrix = glm::perspective(45.0f, 1.0f, 0.1f, 1000.0f);
	glm::mat4 modelMatrix(1);

    mainWindow.GetMainCamera()->SetCameraPosition(glm::vec3(w / 2.0f, w / 10.0f, - w / 4.0f));

    mainWindow.GetMainCamera()->lightPosition = glm::vec3(w / 2.0f, 100.0f, h / 2.0f);

    Material material("FlatEarthShader.shader");
	// Material material("SphericalEarthShader.shader");
	material.Bind();
    unsigned int prID = material.GetProgramID();
	glUseProgram(prID);

    int loc = glGetUniformLocation(prID, "width");
	glUniform1f(loc, w);

	loc = glGetUniformLocation(prID, "height");
	glUniform1f(loc, h);
	
	glm::mat4 viewMat;

	loc = glGetUniformLocation(prID, "u_Model");
	glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(modelMatrix));

	loc = glGetUniformLocation(prID, "u_Projection");
	glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(projectionMatrix));

	
    float* posss = new float[30 * w * h];
	FillPositionInfo(posss, w, h);

	mainWindow.GetMainCamera()->heightFactor = 10;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, 120 * w * h, posss, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 20, (const void *)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 20, (const void *)12);
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

		
	    mainWindow.Refresh(); // Clear buffers

        glActiveTexture(GL_TEXTURE0);
	    glBindTexture(GL_TEXTURE_2D, 0);

		glActiveTexture(GL_TEXTURE1);
	    glBindTexture(GL_TEXTURE_2D, 0);
	}

    glfwTerminate();
}
