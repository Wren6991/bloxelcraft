#ifndef _UTIL_H_INCLUDED_
#define _UTIL_H_INCLUDED_


#include <GL/glew.h>
#include <GL/gl.h>
#include <string>

struct vec3
{
    float x, y, z;
    vec3() {}
    vec3(float x_, float y_, float z_) {x = x_; y = y_; z = z_;}
    vec3 operator+(const vec3&);
    vec3 operator-(const vec3&);
    vec3 operator*(float);
    std::string tostring();
};

struct vec2
{
    float x, y;
    vec2() {}
    vec2(float x_, float y_) {x = x_; y = y_;}
    vec2 operator+(vec2);
    vec2 operator-(vec2);
    vec2 operator*(float);
    float dot(vec2);
};

struct packedvert
{
    vec3 pos;
    vec3 normal;
    float u, v;         //32 byte aligned :D
};

struct texdata
{
    char *data;
    int width;
    int height;
};

texdata loadTGA(std::string filename);

std::string getFileContents(std::string filename);
GLuint makeShader(GLenum shadertype, std::string source);
GLuint makeProgram(GLuint vshader, GLuint fshader);
GLuint makeBuffer(GLenum type, const char* data, GLsizei length);
GLuint makeTexture(std::string filename);

class error
{
    public:
    std::string err;
    error();
    error(std::string);
};

#endif //_UTIL_H_INCLUDED_
