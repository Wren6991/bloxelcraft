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
// render sphere of chunks instead of plane

// lighting
// - per-vertex soft lighting - calc on CPU, upload as 3d texture, sample with linear interp for each fragment
// fluids


inline float sgnf(float x)
{
    if (x > 0.f)
        return 1.f;
    else if (x < 0.f)
        return -1.f;
    else
        return 0.f;
}

inline float roundupdown(float x, bool up)
{
    return up? ceilf(x) : floorf(x);
}

const float PI = 3.14159265358979323846;

const float drawradius = 11.f;
const float drawsquashy = 1.5f;

const float playerspeed = 0.15;

std::vector <vec3> chunkPositions;
std::string progDirectory;

typedef enum {
    side_none = 0,
    side_x,
    side_y,
    side_z
} side_enum;

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
    GLuint lighttexture;
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
    resources.fshader = makeShader(GL_FRAGMENT_SHADER, progDirectory + "data/terrain.f.glsl");
    resources.vshader = makeShader(GL_VERTEX_SHADER, progDirectory + "data/terrain.v.glsl");
    resources.program = makeProgram(resources.vshader, resources.fshader);

    resources.posoffset = glGetUniformLocation(resources.program, "posoffset");
    resources.blocktexture = glGetUniformLocation(resources.program, "blocktexture");
    resources.facetextureloc = glGetUniformLocation(resources.program, "facetextures");
    resources.lighttexture = glGetUniformLocation(resources.program, "lighttexture");

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


    const char *textures = getFileContents(progDirectory + "data/tex_packed.tga").c_str() + 18;

    glGenTextures(1, &resources.facetextures);
    glBindTexture(GL_TEXTURE_2D_ARRAY, resources.facetextures);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_WRAP_BORDER);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_WRAP_BORDER);
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGB, 16, 16, blk_nof_blocks, 0, GL_BGR, GL_UNSIGNED_BYTE, textures);
    glGenerateMipmap(GL_TEXTURE_2D_ARRAY);

    for (int i = -drawradius; i <= drawradius; i++)
    {
        for (int j = -drawradius; j <= drawradius; j++)
        {
            for (int k = -drawradius; k <= drawradius; k++)
            {
                vec3 pos(i, j, k);
                if (vec3(pos.x, pos.y * drawsquashy, pos.z).length() <= drawradius)
                {
                    chunkPositions.push_back(pos);
                }
            }
        }
    }
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
    progDirectory = argv[0];
    int namestart = progDirectory.rfind("\\");
    if (namestart == -1)
        namestart = progDirectory.rfind("/");
    progDirectory = progDirectory.substr(0, namestart + 1);          //inclusive of slash.
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

    std::vector <vec3> oldposns;

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
        vec3 rpos, closestpos, step;
        side_enum hit = side_none;

        std::vector <vec3> posns;

        step = raydir * (1.f / raydir.x * sgnf(raydir.x));   //unit step in x...

        if (raydir.x != 0)
        {
            step = raydir * (1.f / raydir.x);
            rpos = campos - step * (campos.x - roundupdown(campos.x, raydir.x > 0));

            posns.push_back(rpos);
            while ((rpos - campos).length() < 20)
            {
                rpos = rpos + step;
                posns.push_back(rpos);
                if (wld.getBlock(rpos.x, rpos.y, rpos.z))
                {
                    closestpos = rpos;
                    hit = side_x;
                    break;
                }
            }
        }

        if (raydir.y != 0)
        {
            step = raydir * (1.f / raydir.y * sgnf(raydir.y));
            rpos = campos - step * (campos.y - roundupdown(campos.y, raydir.y > 0));

            while ((rpos - campos).length() < 20)
            {
                rpos = rpos + step;
                if (wld.getBlock(rpos.x, rpos.y, rpos.z))
                {
                    if (hit)
                    {
                        if ((rpos - campos).length2() < (closestpos - campos).length2())
                        {
                            closestpos = rpos;
                            hit = side_y;
                        }
                    }
                    else
                    {
                        closestpos = rpos;
                        hit = side_y;
                    }
                    break;
                }
            }
        }

        if (raydir.z != 0)
        {
            step = raydir * (1.f / raydir.z * sgnf(raydir.z));
            rpos = campos - step * (campos.z - roundupdown(campos.z, raydir.z > 0));

            while ((rpos - campos).length() < 20)
            {
                rpos = rpos + step;
                if (wld.getBlock(rpos.x, rpos.y, rpos.z))
                {
                    if (hit)
                    {
                        if ((rpos - campos).length2() < (closestpos - campos).length2())
                        {
                            closestpos = rpos;
                            hit = side_z;
                        }
                    }
                    else
                    {
                        closestpos = rpos;
                        hit = side_z;
                    }
                    break;
                }
            }
        }


        if (hit && keys.newPress.MouseL)
        {
            wld.setBlock(closestpos.x, closestpos.y, closestpos.z, blk_air);
            wld.getChunkAlways(closestpos.x / chunk_size, closestpos.y / chunk_size, closestpos.z / chunk_size)->buildmesh();
        }
        else if (hit && keys.held.MouseR)
        {
            vec3 cubepos = closestpos;
            if (hit == side_x)
                cubepos.x -= sgnf(raydir.x);
            else if (hit == side_y)
                cubepos.y -= sgnf(raydir.y);
            else
                cubepos.z -= sgnf(raydir.z);
            wld.setBlock(cubepos.x, cubepos.y, cubepos.z, blk_wood);
            wld.getChunkAlways(cubepos.x / chunk_size, cubepos.y / chunk_size, cubepos.z / chunk_size)->buildmesh();
        }

        if (glfwGetKey(GLFW_KEY_SPACE))
            oldposns = posns;


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

        glUniform1i(resources.blocktexture, 1);
        glUniform1i(resources.lighttexture, 2);


        int totaltris = 0;

        for (std::vector<vec3>::iterator iter = chunkPositions.begin(); iter != chunkPositions.end(); iter++)
        {
            vec3 pos = *iter;
            vec3 adjustedpos = pos + raydir * 1.5f;
            if (raydir.dot(adjustedpos * (1.f / adjustedpos.length())) > 0.6f)
            {
                int chkcoordx = floorf(camx / chunk_size) + pos.x;
                int chkcoordy = floorf(camy / chunk_size) + pos.y;
                int chkcoordz = floorf(camz / chunk_size) + pos.z;

                glUniform3f(resources.posoffset, chkcoordx * chunk_size, chkcoordy * chunk_size, chkcoordz * chunk_size);
                chunk *chk = wld.getChunk(chkcoordx, chkcoordy, chkcoordz);
                if (chk)
                {
                    chk->draw();
                    totaltris += chk->ntriangles;
                }
            }

        }

        std::cout << "total triangles: " << totaltris << "\n";


        glDisableClientState(GL_VERTEX_ARRAY);
        glDisableClientState(GL_NORMAL_ARRAY);

        glUseProgram(0);

        if (hit)
        {
            vec3 cubepos(floorf(closestpos.x), floorf(closestpos.y), floorf(closestpos.z));
            glBegin(GL_LINES);

            if (hit == side_x)
                glColor3f(1.f, 0.f, 0.f);
            else if (hit == side_y)
                glColor3f(0.f, 1.f, 0.f);
            else
                glColor3f(0.f, 0.f, 1.0);

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

        glPointSize(3.f);

        glBegin(GL_POINTS);
        glColor3f(1.f, 1.f, 1.f);
        for (std::vector <vec3>::iterator iter = oldposns.begin(); iter != oldposns.end(); iter++)
        {
            glVertex3f(iter->x, iter->y, iter->z);
        }
        glEnd();

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glScalef((float)height/width, 1.0, 1.0);
        glDisable(GL_DEPTH_TEST);

        glBegin(GL_LINES);
        glColor3f(1.f, 1.f, 1.f);
        glVertex3f(   0.f, 0.01f, 0.f);
        glVertex3f(   0.f,-0.01f, 0.f);
        glVertex3f (0.01f,   0.f, 0.f);
        glVertex3f(-0.01f,   0.f, 0.f);
        glEnd();

        glFlush();

        glfwSwapInterval(1);
        glfwSwapBuffers();




        // exit if ESC was pressed or window was closed
        running = glfwGetWindowParam(GLFW_OPENED);
    }

    glfwTerminate();

    return 0;
}
