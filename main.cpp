#define GLFW_DLL

#include <GL/glew.h>
#include <GL/glfw.h>
#include <iostream>
#include <cmath>
#include <sstream>
#include "util.h"
#include "chunk.h"
#include "world.h"

const float PI = 3.14159265358979323846;

struct
{
    GLuint fshader;
    GLuint vshader;
    GLuint program;
} resources;

void makeResources()
{
    resources.fshader = makeShader(GL_FRAGMENT_SHADER, "J:/bloxelcraft/f.glsl");
    resources.vshader = makeShader(GL_VERTEX_SHADER, "J:/bloxelcraft/v.glsl");
    resources.program = makeProgram(resources.vshader, resources.fshader);
}


int main()
{
    int     width, height;
    int     frame = 0;
    bool    running = true;

    glfwInit();

    if( !glfwOpenWindow( 512, 512, 0, 0, 0, 0, 0, 0, GLFW_WINDOW ) )
    {
        glfwTerminate();
        return 0;
    }

    glfwSetWindowTitle("BloxelCraft");

    GLenum err = glewInit();

    if (GLEW_OK != err)
    {
        std::cout << "GLEW init failed: \"" << glewGetErrorString(err) << "\". Shutting down...\n";
        char c;
        std::cin >> c;
        glfwTerminate();
        return 0;
    }
    else
    {
        std::cout << "GLEW OK! OGL version: " << GLEW_VERSION_MAJOR << "." << GLEW_VERSION_MINOR << "\n";
    }

    makeResources();

    float camx = 0;
    float camy = 0;
    float camz = 0;
    float pitch = 0;
    float yaw = 0;

    int mousex, mousey, lastmousex, lastmousey;
    mousex = lastmousex = mousey = lastmousey = 0;

    world wld;


    double time = glfwGetTime();

    chunk chk(vec3(0, -16, 0));

    while(running)
    {

        ////////////////////////////////////////UPDATE

        frame++;
        if (glfwGetTime() - time > 1.0)
        {
            time = glfwGetTime();
            std::stringstream ss;
            ss << "BloxelCraft (" << frame << " FPS)";
            glfwSetWindowTitle(ss.str().c_str());
            frame = 0;
        }

        lastmousex = mousex;
        lastmousey = mousey;
        glfwGetMousePos(&mousex, &mousey);

        if (glfwGetMouseButton(GLFW_MOUSE_BUTTON_LEFT))
        {
            pitch += (mousey - lastmousey) * 0.01;
            yaw -= (mousex - lastmousex) * 0.01;
        }
        else if (glfwGetMouseButton(GLFW_MOUSE_BUTTON_RIGHT))
        {
            double dist = (mousey - lastmousey) * 0.05;
            camx += dist * sin(yaw) * cos(pitch);
            camy += dist * sin(pitch);
            camz += dist * cos(yaw) * cos(pitch);
        }

        wld.updateLoadedChunks();

        //////////////////////////////////////////DRAW

        glUseProgram(resources.program);

        glfwGetWindowSize( &width, &height );
        height = height > 0 ? height : 1;

        glViewport( 0, 0, width, height );

        glClearColor(0.53f, 0.81f, 0.92f, 1.0f);
        glClearDepth(1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);

        glEnable(GL_CULL_FACE);

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glFrustum(-1, 1, -1, 1, 1, 1000);
        glRotatef(pitch * 180 / PI, 1, 0, 0);
        glRotatef(-yaw * 180 / PI, 0, 1, 0);
        glTranslatef(-camx, -camy, -camz);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();


        for (int i = -3; i <= 3; i++)
        {
            for (int j = -3; j <= 3; j++)
            {
                chunk *chk = wld.getChunk(i + camx / chunk_size + 0.5, 0, j + camz / chunk_size + 0.5);
                if (chk)
                    chk->draw();
            }
        }

        glFlush();

        glfwSwapInterval(1);
        glfwSwapBuffers();

        glUseProgram(0);

        // exit if ESC was pressed or window was closed
        running = !glfwGetKey(GLFW_KEY_ESC) && glfwGetWindowParam( GLFW_OPENED);
    }

    glfwTerminate();

    return 0;
}
