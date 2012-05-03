#define GLFW_DLL

#include <GL/glew.h>
#include <GL/glfw.h>
#include "J:/CodeBlocks/MinGW/include/glm/gtc/noise.hpp"
#include <iostream>
#include <cmath>
#include <sstream>
#include <vector>
#include "util.h"
#include "chunk.h"
#include "world.h"



// TODO:
// proper terrain generation
// controls
// player physics
// drawing visible sides only (still single VBO, separate IBO for each face direction for each chunk)
// block placing
// proper block selection
// HUD
// more block types
// proper view frustum culling (or view cone?)

// lighting
// - per-vertex soft lighting - calc on CPU, upload as 3d texture, sample with linear interp for each fragment
// fluids


const float PI = 3.14159265358979323846;

const float playerspeed = 0.15;

struct
{
    GLuint fshader;
    GLuint vshader;
    GLuint program;
    GLuint vertexbuffer;
    GLuint posoffset;
    GLuint blocktexture;
    GLuint facetextures;
    GLuint facetextureloc;
} resources;

struct
{
    struct
    {
        bool W;
        bool A;
        bool S;
        bool D;
        bool MouseL;
        bool MouseR;
    } held;

    struct
    {
        /*bool W;
        bool A;
        bool S;
        bool D;*/
        bool MouseL;
        bool MouseR;
    } newPress;

} keys;

void makeResources()
{
    resources.fshader = makeShader(GL_FRAGMENT_SHADER, "J:/bloxelcraft/f.glsl");
    resources.vshader = makeShader(GL_VERTEX_SHADER, "J:/bloxelcraft/v.glsl");
    resources.program = makeProgram(resources.vshader, resources.fshader);

    resources.posoffset = glGetUniformLocation(resources.program, "posoffset");
    resources.blocktexture = glGetUniformLocation(resources.program, "blocktexture");
    resources.facetextureloc = glGetUniformLocation(resources.program, "facetextures");

    std::cout << "posoffset " << resources.posoffset << "\n";

    std::vector <packedvert> verts;
    vec3 normal;

    for (int face = 0; face < 6; face++)                                //one set of vertices for each face direction - allows us to pack in normals, specify all information in index buffer.
    {
        switch (face)
        {
            case f_left:
                normal = vec3(-1, 0, 0);
                break;
            case f_right:
                normal = vec3(1, 0, 0);
                break;
            case f_down:
                normal = vec3(0, -1, 0);
                break;
            case f_up:
                normal = vec3(0, 1, 0);
                break;
            case f_near:
                normal = vec3(0, 0, -1);
                break;
            case f_far:
                normal = vec3(0, 0, 1);
                break;
            default:
                break;
        }
        for (int i = 0; i <= chunk_size; i++)                           //array size is actually chunk_size + 1, as we have edge vertices to account for.
        {
            for (int j = 0; j <= chunk_size; j++)
            {
                for (int k = 0; k <= chunk_size; k++)
                {
                    packedvert v;
                    v.pos = vec3(i, j, k);
                    v.normal = normal;
                    verts.push_back(v);
                }
            }
        }
    }

    glGenBuffers(1, &resources.vertexbuffer);
    glBindBuffer(GL_ARRAY_BUFFER, resources.vertexbuffer);
    glBufferData(GL_ARRAY_BUFFER, 6 * (chunk_size + 1)*(chunk_size + 1)*(chunk_size + 1) * sizeof(packedvert), &verts[0], GL_STATIC_DRAW);


    const char *textures = getFileContents("J:/bloxelcraft/data/tex_packed.tga").c_str() + 18;

    glGenTextures(1, &resources.facetextures);
    glBindTexture(GL_TEXTURE_2D_ARRAY, resources.facetextures);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_WRAP_BORDER);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_WRAP_BORDER);
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGB, 16, 16, blk_nof_blocks, 0, GL_BGR, GL_UNSIGNED_BYTE, textures);
    glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
}

void GLFWCALL keyCallback(int character, int action)
{
    switch(character)
    {
        case 'W':
            keys.held.W = action;
            break;
        case 'S':
            keys.held.S = action;
            break;
        case 'A':
            keys.held.A = action;
            break;
        case 'D':
            keys.held.D = action;
            break;
        default:
            break;
    }
    std::cout << "W: " << keys.held.W << "\n";
}

void checkControls()
{
    if(glfwGetMouseButton(GLFW_MOUSE_BUTTON_LEFT))
    {
            keys.newPress.MouseL = !keys.held.MouseL;
            keys.held.MouseL = true;
    }
    else
    {
        keys.newPress.MouseL = false;
        keys.held.MouseL = false;
    }

    if(glfwGetMouseButton(GLFW_MOUSE_BUTTON_RIGHT))
    {
            keys.newPress.MouseR = !keys.held.MouseR;
            keys.held.MouseR = true;
    }
    else
    {
        keys.newPress.MouseR = false;
        keys.held.MouseR = false;
    }

}


int main(int argc, char **argv)
{
    int     width, height;
    int     frame = 0;
    bool    running = true;

    std::cout << (int)0.5f << " " << (int) 0.9f << " " << (int) -0.5f;

    glfwInit();

    if( !glfwOpenWindow( 640, 480, 0, 0, 0, 0, 0, 0, GLFW_WINDOW ) )
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

    glfwSetKeyCallback(keyCallback);

    while(running)
    {
        ////////////////////////////////////////UPDATE

        frame++;
        checkControls();

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

        if (!glfwGetKey(GLFW_KEY_ESC) && glfwGetWindowParam(GLFW_ACTIVE))
        {
            glfwDisable(GLFW_MOUSE_CURSOR);
            pitch += (mousey - height / 2) * 0.01;
            yaw -= (mousex - width / 2) * 0.01;
            glfwSetMousePos(width / 2, height / 2);

        }
        else
        {
            glfwEnable(GLFW_MOUSE_CURSOR);
        }

        if (keys.held.W || keys.held.S)
        {
            double dist;
            if (keys.held.W)
                dist = playerspeed;
            else
                dist = -playerspeed;
            camx -= dist * sin(yaw) * cos(pitch);
            camy -= dist * sin(pitch);
            camz -= dist * cos(yaw) * cos(pitch);
        }

        if (keys.held.A || keys.held.D)
        {
            double dist;
            if (keys.held.A)
                dist = playerspeed;
            else
                dist = -playerspeed;
            camx -= dist * cos(yaw);
            camz -= dist * sin(-yaw);
        }


        vec3 campos(camx, camy, camz);
        vec3 raydir(sin(-yaw) * cos(pitch), sin(-pitch), -cos(yaw) * cos(pitch));
        vec3 rpos;
        bool hit = false;
        for(int i = 0; i < 20; i++)
        {
            rpos = campos + raydir * i;
            if (wld.getBlock(rpos.x, rpos.y, rpos.z))
            {
                hit = true;
                break;
            }
        }

        if (hit && keys.newPress.MouseL)
        {
            wld.setBlock(rpos.x, rpos.y, rpos.z, blk_air);
            wld.getChunkAlways(rpos.x / chunk_size, rpos.y / chunk_size, rpos.z / chunk_size)->buildmesh();
        }
        else if (hit && keys.newPress.MouseR)
        {
            wld.setBlock(rpos.x - raydir.x, rpos.y - raydir.y, rpos.z - raydir.z, blk_wood);
            wld.getChunkAlways((rpos.x - raydir.x) / chunk_size, (rpos.y - raydir.y) / chunk_size, (rpos.z - raydir.z) / chunk_size)->buildmesh();
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
        glFrustum(-0.5 * width/float(height), 0.5 * width/float(height), -0.5, 0.5, 0.5, 1000);
        glRotatef(pitch * 180 / PI, 1, 0, 0);
        glRotatef(-yaw * 180 / PI, 0, 1, 0);
        glTranslatef(-camx, -camy, -camz);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        glActiveTexture(GL_TEXTURE0);
        glUniform1i(resources.facetextureloc, 0);
        glBindTexture(GL_TEXTURE_2D_ARRAY, resources.facetextures);

        glBindBuffer(GL_ARRAY_BUFFER, resources.vertexbuffer);
        glEnableClientState(GL_VERTEX_ARRAY);
        glVertexPointer(3, GL_FLOAT, sizeof(packedvert), (void*)0);
        glEnableClientState(GL_NORMAL_ARRAY);
        glNormalPointer(GL_FLOAT, sizeof(packedvert), (void*)sizeof(vec3));

        vec2 camdir(-sin(yaw), -cos(yaw));

        for (int i = -5; i <= 5; i++)
        {
            for (int j = -5; j <= 5; j++)
            {
                vec2 adjustedpos = vec2(i, j) + camdir * 3;
                if (camdir.dot(adjustedpos) > 0.3)
                {
                    int chkcoordx, chkcoordy, chkcoordz;
                    chkcoordx = floor(i + camx / chunk_size);// + 0.5;
                    chkcoordy = 0;
                    chkcoordz =  floor(j + camz / chunk_size);// + 0.5;
                    glUniform3f(resources.posoffset, chkcoordx * chunk_size, chkcoordy * chunk_size, chkcoordz * chunk_size);
                    glActiveTexture(GL_TEXTURE1);
                    glUniform1i(resources.blocktexture, 1);
                    chunk *chk = wld.getChunk(chkcoordx, chkcoordy, chkcoordz);
                    if (chk)
                    {
                        chk->draw();
                    }
                }

            }
        }

        glDisableClientState(GL_VERTEX_ARRAY);
        glDisableClientState(GL_NORMAL_ARRAY);

        glUseProgram(0);

        if (hit)
        {
            vec3 cubepos(floorf(rpos.x), floorf(rpos.y), floorf(rpos.z));
            glBegin(GL_LINES);
            glVertex3f(cubepos.x    , cubepos.y    , cubepos.z    );
            glVertex3f(cubepos.x + 1, cubepos.y    , cubepos.z    );
            glVertex3f(cubepos.x    , cubepos.y + 1, cubepos.z    );
            glVertex3f(cubepos.x + 1, cubepos.y + 1, cubepos.z    );
            glVertex3f(cubepos.x    , cubepos.y + 1, cubepos.z + 1);
            glVertex3f(cubepos.x + 1, cubepos.y + 1, cubepos.z + 1);
            glVertex3f(cubepos.x    , cubepos.y    , cubepos.z + 1);
            glVertex3f(cubepos.x + 1, cubepos.y    , cubepos.z + 1);

            glVertex3f(cubepos.x    , cubepos.y    , cubepos.z    );
            glVertex3f(cubepos.x    , cubepos.y + 1, cubepos.z    );
            glVertex3f(cubepos.x + 1, cubepos.y    , cubepos.z    );
            glVertex3f(cubepos.x + 1, cubepos.y + 1, cubepos.z    );
            glVertex3f(cubepos.x + 1, cubepos.y    , cubepos.z + 1);
            glVertex3f(cubepos.x + 1, cubepos.y + 1, cubepos.z + 1);
            glVertex3f(cubepos.x    , cubepos.y    , cubepos.z + 1);
            glVertex3f(cubepos.x    , cubepos.y + 1, cubepos.z + 1);

            glVertex3f(cubepos.x    , cubepos.y    , cubepos.z    );
            glVertex3f(cubepos.x    , cubepos.y    , cubepos.z + 1);
            glVertex3f(cubepos.x + 1, cubepos.y    , cubepos.z    );
            glVertex3f(cubepos.x + 1, cubepos.y    , cubepos.z + 1);
            glVertex3f(cubepos.x + 1, cubepos.y + 1, cubepos.z    );
            glVertex3f(cubepos.x + 1, cubepos.y + 1, cubepos.z + 1);
            glVertex3f(cubepos.x    , cubepos.y + 1, cubepos.z    );
            glVertex3f(cubepos.x    , cubepos.y + 1, cubepos.z + 1);
            glEnd();


        }

        glFlush();

        glfwSwapInterval(1);
        glfwSwapBuffers();




        // exit if ESC was pressed or window was closed
        running = glfwGetWindowParam(GLFW_OPENED);
    }

    glfwTerminate();

    return 0;
}
