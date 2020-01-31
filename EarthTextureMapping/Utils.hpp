#include <iostream>
#include <string>
#include <fstream>
#include <jpeglib.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "glm/glm/glm.hpp"
#include "glm/glm/gtc/matrix_transform.hpp"
#include "glm/glm/gtc/type_ptr.hpp"
#include <sstream>
#include <vector>
#include <algorithm>

unsigned int idJpegTexture;

struct Vertex
{
    union {
        struct
        {
            float x, y, z;
        };
        struct
        {
            float position[3];
        };
    };

    union {
        struct
        {
            float s, t;
        };
        struct
        {
            float uv[2];
        };
    };

    float normal[3];
};

struct Triangle
{
    Vertex *vertices[3];
    float normal[3];
    int indices[3];
};

// Stores source file for all sub-shader types
struct ShaderProgramSource
{
    std::string vertexShader;
    std::string fragmentShader;
};

namespace Utils {

    // Get size of given GL type
    static unsigned int GetSizeOfType(unsigned int type)
    {
        switch (static_cast<GLenum>(type))
        {
        case GL_FLOAT:
            return 4;
        case GL_UNSIGNED_INT:
            return 4;
        case GL_INT:
            return 4;
        case GL_UNSIGNED_BYTE:
            return 1;
        case GL_BYTE:
            return 1;
        default:
            std::cout << "Error\n";
        }
        return 0;
    }

    template <typename T>
    static T Clamp(T targetValue, T minLimit, T maxLimit)
    {
        if (targetValue < minLimit)
            targetValue = minLimit;
        else if (targetValue > maxLimit)
            targetValue = maxLimit;

        return targetValue;
    }

    static std::ostream &operator<<(std::ostream &out, const glm::vec3 &vec)
    {
        out << "[" << vec.x << ", " << vec.y << ", " << vec.z << "]\n";
        return out;
    }

    static std::ostream &operator<<(std::ostream &out, const glm::vec4 &vec)
    {
        out << "[" << vec.x << ", " << vec.y << ", " << vec.z << ", " << vec.w << "]\n";
        return out;
    }

    static std::ostream &operator<<(std::ostream &out, const glm::mat4 &mat)
    {
        out << mat[0] << mat[1] << mat[2] << mat[3];
        return out;
    }

    static void SetVertexUV(Vertex *vert, int w, int h)
    {
        vert->s = vert->x / w;
        vert->t = vert->z / h;
    }

    static void SetVertexPos(Vertex *vert, float x, float y, float z)
    {
        vert->x = x;
        vert->y = y;
        vert->z = z;
    }

    static void SetTriangleVertices(Triangle *tri, std::vector<Vertex *> vs, std::vector<int> ind)
    {
        tri->vertices[0] = vs[0];
        tri->vertices[1] = vs[1];
        tri->vertices[2] = vs[2];

        glm::vec3 v0(vs[0]->x, vs[0]->y, vs[0]->z);
        glm::vec3 v1(vs[1]->x, vs[1]->y, vs[1]->z);
        glm::vec3 v2(vs[2]->x, vs[2]->y, vs[2]->z);

        glm::vec3 triNormal = glm::normalize(glm::cross(glm::normalize(v1 - v0), glm::normalize(v2 - v0)));

        tri->normal[0] = triNormal.x;
        tri->normal[1] = triNormal.y;
        tri->normal[2] = triNormal.z;

        tri->indices[0] = ind[0];
        tri->indices[1] = ind[1];
        tri->indices[2] = ind[2];
    }

    // Fills given vertex normals array with normals of the vertices
    static void CalculateTriangleNormals(Triangle *triangles, Vertex *vertices, unsigned int triangleCount, unsigned int vertexCount)
    {
        std::vector<std::vector<int>> vertexTriangles(vertexCount);

        for (unsigned int i = 0; i < triangleCount; ++i)
        {
            // Get Vertex IDs
            unsigned int p0ID = triangles[i].indices[0];
            unsigned int p1ID = triangles[i].indices[1];
            unsigned int p2ID = triangles[i].indices[2];

            // Store which triangle contains the vertices
            vertexTriangles[p0ID].push_back(i);
            vertexTriangles[p1ID].push_back(i);
            vertexTriangles[p2ID].push_back(i);

            // Get corresponding vertex positions
            glm::vec3 p0(vertices[p0ID].position[0], vertices[p0ID].position[1], vertices[p0ID].position[2]);
            glm::vec3 p1(vertices[p1ID].position[0], vertices[p1ID].position[1], vertices[p1ID].position[2]);
            glm::vec3 p2(vertices[p2ID].position[0], vertices[p2ID].position[1], vertices[p2ID].position[2]);

            // Calculate the normal of the plane ( which triangle resides on )
            // glm::vec3 result = glm::normalize(glm::cross(p2 - p0, p1 - p0));

            // std::cout << p0 << p1 << p2;

            // Assign normal values to result
            // normals[i * 3 + 0] = result.x;
            // normals[i * 3 + 1] = result.y;
            // normals[i * 3 + 2] = result.z;
        }

        /*for (int i = 0; i < vertexTriangles.size(); ++i)
            {
                for (int j = 0; j < vertexTriangles[i].size(); ++j)
                    std::cout << vertexTriangles[i][j] << " ";
                std::cout << "\n";
            }*/
        // Calculate vertex normals
        for (unsigned int i = 0; i < vertexCount; ++i)
        {
            // Sum of all triangles that share the vertex
            for (unsigned int j = 0; j < vertexTriangles[i].size(); ++j)
            {
                vertices[i].normal[0] += triangles[vertexTriangles[i][j]].normal[0];
                vertices[i].normal[1] += triangles[vertexTriangles[i][j]].normal[1];
                vertices[i].normal[2] += triangles[vertexTriangles[i][j]].normal[2];
            }

            // Average them
            vertices[i].normal[0] /= vertexTriangles[i].size();
            vertices[i].normal[1] /= vertexTriangles[i].size();
            vertices[i].normal[2] /= vertexTriangles[i].size();
        }

        for (unsigned int i = 0; i < vertexCount; ++i)
        {
            float length = glm::sqrt(vertices[i].normal[0] * vertices[i].normal[0] + vertices[i].normal[1] * vertices[i].normal[1] + vertices[i].normal[2] * vertices[i].normal[2]);

            // Average them
            vertices[i].normal[0] /= length;
            vertices[i].normal[1] /= length;
            vertices[i].normal[2] /= length;
        }
    }

    ShaderProgramSource ParseShaderFile(const std::string &filePath)
    {
        std::ifstream file(filePath);
        std::stringstream ss[2];

        enum class ShaderType
        {
            NONE = -1,
            VERTEX = 0,
            FRAGMENT = 1
        };

        ShaderType type = ShaderType::NONE;

        std::string line;
        while (std::getline(file, line))
        {
            if (line.find("#shader") != std::string::npos)
            {
                if (line.find("vertex") != std::string::npos)
                    type = ShaderType::VERTEX;
                else if (line.find("fragment") != std::string::npos)
                    type = ShaderType::FRAGMENT;
            }
            else
                ss[static_cast<int>(type)] << line << "\n";
        }

        return {ss[0].str(), ss[1].str()};
    }

    // Compile the shader with given type and source file
    unsigned int CompileShader(unsigned int type, const std::string &shaderSource)
    {
        unsigned int id = glCreateShader(type);    // Get a shader id for a type
        const char *source = shaderSource.c_str(); // Convert source to type of const char*
        glShaderSource(id, 1, &source, nullptr);   // Attain source file to the shader

        // Compile and check the status
        glCompileShader(id); // Compile Shader
        int result;
        glGetShaderiv(id, GL_COMPILE_STATUS, &result);
        if (result == GL_FALSE)
        {
            int length;
            glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length);
            char *errMessage = (char *)alloca(length * sizeof(char));
            glGetShaderInfoLog(id, length, &length, errMessage);

            std::cout << "Failed to compile " << (type == GL_VERTEX_SHADER ? "vertex" : "fragment") << " shader!\n";
            std::cout << errMessage << "\n";
            glDeleteShader(id);
            return 0;
        }
        // --

        return id;
    }

    // Create a program and attach shaders to it
    unsigned int CreateShaderProgram(const std::string &vertexShaderSource, const std::string &fragmentShaderSource)
    {
        unsigned int program = glCreateProgram(); // Get ID for a program

        // Compile shaders
        unsigned int vs = CompileShader(GL_VERTEX_SHADER, vertexShaderSource);
        unsigned int fs = CompileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);
        // --

        // Attach shaders to the program
        glAttachShader(program, vs);
        glAttachShader(program, fs);
        // --

        int result;
        // Link and check status
        glLinkProgram(program);
        glGetProgramiv(program, GL_LINK_STATUS, &result);
        if (result == GL_FALSE)
        {
            int length;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
            char *errMessage = (char *)alloca(length * sizeof(char));
            glGetProgramInfoLog(program, length, &length, errMessage);
            std::cout << "Error linking program: " << errMessage << "\n";
        }
        // --

        // Validate and check status
        glValidateProgram(program);
        glGetProgramiv(program, GL_VALIDATE_STATUS, &result);
        if (result == GL_FALSE)
        {
            int length;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
            char *errMessage = (char *)alloca(length * sizeof(char));
            glGetProgramInfoLog(program, length, &length, errMessage);
            std::cout << "Error validating program: " << errMessage << "\n";
        }
        // --

        // Delete shaders
        glDeleteShader(vs);
        glDeleteShader(fs);
        // --

        return program;
    }

    void initTexture(char *filename, int *w, int *h)
    {
        int width, height;

        unsigned char *raw_image = NULL;
        int bytes_per_pixel = 3;   /* or 1 for GRACYSCALE images */
        int color_space = JCS_RGB; /* or JCS_GRAYSCALE for grayscale images */

        /* these are standard libjpeg structures for reading(decompression) */
        struct jpeg_decompress_struct cinfo;
        struct jpeg_error_mgr jerr;

        /* libjpeg data structure for storing one row, that is, scanline of an image */
        JSAMPROW row_pointer[1];

        FILE *infile = fopen(filename, "rb");
        unsigned long location = 0;
        int i = 0, j = 0;

        if (!infile)
        {
            printf("Error opening jpeg file %s\n!", filename);
            return;
        }
        printf("Texture filename = %s\n", filename);

        /* here we set up the standard libjpeg error handler */
        cinfo.err = jpeg_std_error(&jerr);
        /* setup decompression process and source, then read JPEG header */
        jpeg_create_decompress(&cinfo);
        /* this makes the library read from infile */
        jpeg_stdio_src(&cinfo, infile);
        /* reading the image header which contains image information */
        jpeg_read_header(&cinfo, TRUE);
        /* Start decompression jpeg here */
        jpeg_start_decompress(&cinfo);

        /* allocate memory to hold the uncompressed image */
        raw_image = (unsigned char *)malloc(cinfo.output_width * cinfo.output_height * cinfo.num_components);
        /* now actually read the jpeg into the raw buffer */
        row_pointer[0] = (unsigned char *)malloc(cinfo.output_width * cinfo.num_components);
        /* read one scan line at a time */
        while (cinfo.output_scanline < cinfo.image_height)
        {
            jpeg_read_scanlines(&cinfo, row_pointer, 1);
            for (i = 0; i < cinfo.image_width * cinfo.num_components; i++)
                raw_image[location++] = row_pointer[0][i];
        }

        height = cinfo.image_height;
        width = cinfo.image_width;

        glGenTextures(1, &idJpegTexture);
        glBindTexture(GL_TEXTURE_2D, idJpegTexture);
        glActiveTexture(GL_TEXTURE0);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, raw_image);

        *w = width;
        *h = height;

        glGenerateMipmap(GL_TEXTURE_2D);
        /* wrap up decompression, destroy objects, free pointers and close open files */
        jpeg_finish_decompress(&cinfo);
        jpeg_destroy_decompress(&cinfo);
        free(row_pointer[0]);
        free(raw_image);
        fclose(infile);
    }
}