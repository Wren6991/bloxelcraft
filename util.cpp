#include <iostream>
#include <fstream>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "util.h"


vec3 vec3::operator+(const vec3& v)
{
    return vec3(x + v.x, y + v.y, z + v.z);
}

vec3 vec3::operator-(const vec3& v)
{
    return vec3(x - v.x, y - v.y, z - v.z);
}

vec3 vec3::operator*(float s)
{
    return vec3(x * s, y * s, z * s);
}

std::string vec3::tostring()
{
    std::cout << x << " " << y << " " << z << "\n";
    return std::string();
}

std::string getFileContents(std::string filename)
{
    std::fstream file(filename.c_str(), std::ios::binary | std::ios::in);
    if (!file.is_open())
    {
        std::cerr << "Could not open file " << filename.c_str() << "\n";
        return "";
    }

    file.seekg(0, std::ios::end);
    int length = file.tellg();
    file.seekg(0, std::ios::beg);

    char *data = new char[length];
    file.read(data, length);
    std::string contents(data, length);
    delete data;
    return contents;
}

/*texdata loadTGA(std::string filename)
{
    struct tga_header
    {
        char id_length;
        char color_map_type;
        char data_type_code;
        unsigned char color_map_origin[2];
        unsigned char color_map_length[2];
        char color_map_depth;
        unsigned char x_origin[2];
        unsigned char y_origin[2];
        unsigned char width[2];
        unsigned char height[2];
        char bits_per_pixel;
        char image_descriptor;
    } header;

    std::string imgdata = getFileContents(filename);

    header = *(tga_header*)imgdata.c_str();

    if (imgdata.length() < sizeof(header))
        throw(error(filename + "has incomplete TGA header"));

}*/

static short le_short(unsigned char *bytes)
{
    return bytes[0] | ((char)bytes[1] << 8);
}

texdata loadTGA(std::string filename)
{
    texdata t;
    struct tga_header
    {
        char id_length;
        char color_map_type;
        char data_type_code;
        unsigned char color_map_origin[2];
        unsigned char color_map_length[2];
        char color_map_depth;
        unsigned char x_origin[2];
        unsigned char y_origin[2];
        unsigned char width[2];
        unsigned char height[2];
        char bits_per_pixel;
        char image_descriptor;
    } header;
    int i, color_map_size, pixels_size;
    FILE *f;
    size_t read;
    void *pixels;

    f = fopen(filename.c_str(), "rb");

    if (!f)
    {
        fprintf(stderr, "Unable to open %s for reading\n", filename.c_str());
        throw(error());
    }

    read = fread(&header, 1, sizeof(header), f);

    if (read != sizeof(header))
    {
        fprintf(stderr, "%s has incomplete tga header\n", filename.c_str());
        fclose(f);
        throw(error());
    }
    if (header.data_type_code != 2)
    {
        fprintf(stderr, "%s is not an uncompressed RGB tga file\n", filename.c_str());
        fclose(f);
        throw(error());
    }
    if (header.bits_per_pixel != 24)
    {
        fprintf(stderr, "%s is not a 24-bit uncompressed RGB tga file\n", filename.c_str());
        fclose(f);
        throw(error());
    }

    for (i = 0; i < header.id_length; ++i)
        if (getc(f) == EOF)
        {
            fprintf(stderr, "%s has incomplete id string\n", filename.c_str());
            fclose(f);
            throw(error());
        }

    color_map_size = le_short(header.color_map_length) * (header.color_map_depth/8);
    for (i = 0; i < color_map_size; ++i)
        if (getc(f) == EOF)
        {
            fprintf(stderr, "%s has incomplete color map\n", filename.c_str());
            fclose(f);
            throw(error());
        }

    t.width = le_short(header.width);
    t.height = le_short(header.height);
    pixels_size = t.width * t.height * (header.bits_per_pixel/8);
    pixels = malloc(pixels_size);

    read = fread(pixels, 1, pixels_size, f);

    if (read != pixels_size)
    {
        fprintf(stderr, "%s has incomplete image\n", filename.c_str());
        fclose(f);
        free(pixels);
        throw(error());
    }
    t.data = (char*)pixels;
    return t;
}

static void showInfoLog(
    GLuint object,
    PFNGLGETSHADERIVPROC glGet__iv,
    PFNGLGETSHADERINFOLOGPROC glGet__InfoLog
)
{
    GLint log_length;

    glGet__iv(object, GL_INFO_LOG_LENGTH, &log_length);
    char* log = new char[log_length];
    glGet__InfoLog(object, log_length, NULL, log);
    std::cerr << log;
    delete log;
}

GLuint makeShader(GLenum shadertype, std::string filename)
{
    std::string source = getFileContents(filename);
    GLint shader = glCreateShader(shadertype);
    const char *sourceptr = source.c_str();
    GLint length = source.length();
    glShaderSource(shader, 1, (const GLchar**) &sourceptr, &length);
    glCompileShader(shader);

    GLint shaderOK;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &shaderOK);

    if (!shaderOK)
    {
        std::cerr << "Failed to compile shader unnamed\nPrinting log:\n";
        showInfoLog(shader, glGetShaderiv, glGetShaderInfoLog);
        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

GLuint makeProgram(GLuint vshader, GLuint fshader)
{

    GLuint program = glCreateProgram();
    glAttachShader(program, fshader);
    glAttachShader(program, vshader);
    glLinkProgram(program);

    GLint programOK;
    glGetProgramiv(program, GL_LINK_STATUS, &programOK);

    if(!programOK)
    {
        std::cerr << "Failed to link shader program.\n";
        showInfoLog(program, glGetProgramiv, glGetProgramInfoLog);
        glDeleteProgram(program);
        return 0;
    }

    return program;


}

GLuint makeBuffer(GLenum type, const char* data, GLsizei length)
{
    GLuint bufferhandle;
    glGenBuffers(1, &bufferhandle);
    glBindBuffer(type, bufferhandle);
    glBufferData(type, length, data, GL_STATIC_DRAW);
    return bufferhandle;
}

GLuint makeTexture(std::string filename)
{
    try
    {
        texdata texture = loadTGA(filename);
        GLuint texhandle;
        glGenTextures(1, &texhandle);
        glBindTexture(GL_TEXTURE_2D, texhandle);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_WRAP_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_WRAP_BORDER);
        //                        LOD, internal format,                    border, ext. fmt, type
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, texture.width, texture.height, 0, GL_BGR, GL_UNSIGNED_BYTE, texture.data);
        delete texture.data;
        return texhandle;
    }
    catch (error e)
    {
        std::cerr << e.err;
        return 0;
    }
}


error::error()
{
    err = std::string("Whoops! An error occurred.");
}

error::error(std::string err_)
{
    err = err_;
}


