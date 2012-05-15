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
// player physics
// drawing visible sides only (still single VBO, separate IBO for each face direction for each chunk)
// block placing
// proper block selection
// HUD
// more block types
//

// lighting
// - per-vertex soft lighting - calc on CPU, upload as 3d texture, sample with linear interp for each fragment
// fluids
// underwater fog (fragment shader applied if player underwater)


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
world wld;

hitresult traceRay(vec3 origin, vec3 direction)
{
    vec3 rpos, step, closestpos;
    side_enum hit = side_none;

    std::vector <vec3> posns;

    step = direction * (1.f / direction.x * sgnf(direction.x));   //unit step in x...

    if (direction.x != 0)
    {
        step = direction * (1.f / direction.x);
        rpos = origin - step * (origin.x - roundupdown(origin.x, direction.x > 0));

        posns.push_back(rpos);
        while ((rpos - origin).length() < 20)
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

    if (direction.y != 0)
    {
        step = direction * (1.f / direction.y * sgnf(direction.y));
        rpos = origin - step * (origin.y - roundupdown(origin.y, direction.y > 0));

        while ((rpos - origin).length() < 20)
        {
            rpos = rpos + step;
            if (wld.getBlock(rpos.x, rpos.y, rpos.z))
            {
                if (hit)
                {
                    if ((rpos - origin).length2() < (closestpos - origin).length2())
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

    if (direction.z != 0)
    {
        step = direction * (1.f / direction.z * sgnf(direction.z));
        rpos = origin - step * (origin.z - roundupdown(origin.z, direction.z > 0));

        while ((rpos - origin).length() < 20)
        {
            rpos = rpos + step;
            if (wld.getBlock(rpos.x, rpos.y, rpos.z))
            {
                if (hit)
                {
                    if ((rpos - origin).length2() < (closestpos - origin).length2())
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

    return hitresult(closestpos, hit, (blocktype)wld.getBlock(rpos.x, rpos.y, rpos.z));
}

struct
{
    struct
    {
        GLuint fshader;
        GLuint vshader;
        GLuint program;
        GLuint posoffset;
        GLuint blocktexture;
        GLuint facetextureloc;
        GLuint lighttexture;
    } terrain;

    struct
    {
        GLuint fshader;
        GLuint vshader;
        GLuint program;
        GLuint posoffset;
        GLuint blocktexture;
        GLuint facetextureloc;
        GLuint lighttexture;
    } water;

    GLuint vertexbuffer;
    GLuint facetextures;

} resources;

struct
{
    struct
    {
        bool W;
        bool A;
        bool S;
        bool D;
        bool space;
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

struct
{
    vec3 pos;
    vec3 vel;
    float pitch;
    float yaw;
    vec3 eyedir;
    bool isOnGround;
} player;

void makeResources()
{
    resources.terrain.fshader = makeShader(GL_FRAGMENT_SHADER, progDirectory + "data/terrain.f.glsl");
    resources.terrain.vshader = makeShader(GL_VERTEX_SHADER, progDirectory + "data/terrain.v.glsl");
    resources.terrain.program = makeProgram(resources.terrain.vshader, resources.terrain.fshader);

    resources.terrain.posoffset = glGetUniformLocation(resources.terrain.program, "posoffset");
    resources.terrain.blocktexture = glGetUniformLocation(resources.terrain.program, "blocktexture");
    resources.terrain.facetextureloc = glGetUniformLocation(resources.terrain.program, "facetextures");
    resources.terrain.lighttexture = glGetUniformLocation(resources.terrain.program, "lighttexture");

    resources.water.fshader = makeShader(GL_FRAGMENT_SHADER, progDirectory + "data/water.f.glsl");
    resources.water.vshader = makeShader(GL_VERTEX_SHADER, progDirectory + "data/water.v.glsl");
    resources.water.program = makeProgram(resources.water.vshader, resources.water.fshader);

    resources.water.posoffset = glGetUniformLocation(resources.water.program, "posoffset");
    resources.water.blocktexture = glGetUniformLocation(resources.water.program, "blocktexture");
    resources.water.facetextureloc = glGetUniformLocation(resources.water.program, "facetextures");
    resources.water.lighttexture = glGetUniformLocation(resources.water.program, "lighttexture");


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
        case ' ':
            keys.held.space = action;
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

    player.pos = vec3(0, 0, 0);
    player.pitch = 0;
    player.yaw = 0;

    int mousex, mousey, lastmousex, lastmousey;
    mousex = lastmousex = mousey = lastmousey = 0;

    double timelastcounted = glfwGetTime();
    double time = timelastcounted;

    glfwSetKeyCallback(keyCallback);

    std::vector <vec3> oldposns;

    while(running)
    {
        ////////////////////////////////////////UPDATE
        double dt = glfwGetTime() - time;
        time = glfwGetTime();

        frame++;
        checkControls();

        if (time - timelastcounted > 1.0)
        {
            timelastcounted = glfwGetTime();
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
            player.pitch += (mousey - height / 2) * 0.01;
            player.yaw -= (mousex - width / 2) * 0.01;
            glfwSetMousePos(width / 2, height / 2);

        }
        else
        {
            glfwEnable(GLFW_MOUSE_CURSOR);
        }

        if (keys.held.W || keys.held.S)
        {
            float dist = (keys.held.W ? 1.f : -1.f) * (player.isOnGround ? 150.f : 3.f);

            player.vel = player.vel - vec3(sin(player.yaw),
                                            0,
                                            cos(player.yaw)) * (dist * dt);
        }

        if (keys.held.A || keys.held.D)
        {
            double dist;
            if (keys.held.A)
                dist = playerspeed;
            else
                dist = -playerspeed;
            player.pos.x -= dist * cos(player.yaw);
            player.pos.z -= dist * sin(-player.yaw);
        }

        player.pos = player.pos + player.vel * dt;
        player.vel = vec3(player.vel.x * pow(0.9, dt), player.vel.y * pow(0.9, dt), player.vel.z * pow(0.9, dt));

        player.vel = player.vel - vec3(0, 9.8, 0) * dt;

        if (wld.getBlock(player.pos.x, player.pos.y - 1.5f, player.pos.z) > blk_water)
        {
            player.pos.y = ceil(player.pos.y + 0.5) - 0.5;
            player.vel = vec3(player.vel.x * pow(0.0000001, dt), player.vel.y > 0 ? player.vel.y : 0, player.vel.z * pow(0.0000001, dt));
            player.isOnGround = true;
        }
        else
        {
            player.isOnGround = false;
        }

        switch (wld.getBlock(player.pos.x, player.pos.y - 1.0, player.pos.z))
        {
            case blk_water:
                player.vel = (player.vel + vec3(0, 30 * dt, 0)) * pow(0.07, dt);
                player.isOnGround = true;
                break;
        }

        if (player.isOnGround && keys.held.space)
        {
            player.vel.y = 5.0f;
        }

        player.eyedir = vec3(sin(-player.yaw) * cos(player.pitch), sin(-player.pitch), -cos(player.yaw) * cos(player.pitch));

        hitresult hitres = traceRay(vec3(player.pos.x, player.pos.y, player.pos.z), player.eyedir);


        if (hitres.hit && keys.newPress.MouseL)
        {
            wld.setBlock(hitres.pos.x, hitres.pos.y, hitres.pos.z, blk_air);
            chunk *chk = wld.getChunkAlways(hitres.pos.x / chunk_size, hitres.pos.y / chunk_size, hitres.pos.z / chunk_size);
            chk->buildmesh();
            vec3 relpos = hitres.pos - chk->chunkpos;
            relpos = vec3(floorf(relpos.x), floorf(relpos.y), floorf(relpos.z));
            if (relpos.x == 0 && chk->neighbors.xn)
                chk->neighbors.xn->buildmesh();
            else if (relpos.x == chunk_size - 1 && chk->neighbors.xp)
                chk->neighbors.xp->buildmesh();

            if (relpos.y == 0 && chk->neighbors.yn)
                chk->neighbors.yn->buildmesh();
            else if (relpos.y == chunk_size - 1 && chk->neighbors.yp)
                chk->neighbors.yp->buildmesh();

            if (relpos.z == 0 && chk->neighbors.zn)
                chk->neighbors.zn->buildmesh();
            else if (relpos.z == chunk_size - 1 && chk->neighbors.zp)
                chk->neighbors.zp->buildmesh();

        }
        else if (hitres.hit && keys.held.MouseR)
        {
            vec3 cubepos = hitres.pos;
            if (hitres.hit == side_x)
                cubepos.x -= sgnf(player.eyedir.x);
            else if (hitres.hit == side_y)
                cubepos.y -= sgnf(player.eyedir.y);
            else
                cubepos.z -= sgnf(player.eyedir.z);
            wld.setBlock(cubepos.x, cubepos.y, cubepos.z, blk_wood);
            chunk *chk = wld.getChunkAlways(cubepos.x / chunk_size, cubepos.y / chunk_size, cubepos.z / chunk_size);
            chk->buildmesh();
            vec3 relpos = cubepos - chk->chunkpos;
            relpos = vec3(floorf(relpos.x), floorf(relpos.y), floorf(relpos.z));
            if (relpos.x == 0 && chk->neighbors.xn)
                chk->neighbors.xn->buildmesh();
            else if (relpos.x == chunk_size - 1 && chk->neighbors.xp)
                chk->neighbors.xp->buildmesh();

            if (relpos.y == 0 && chk->neighbors.yn)
                chk->neighbors.yn->buildmesh();
            else if (relpos.y == chunk_size - 1 && chk->neighbors.yp)
                chk->neighbors.yp->buildmesh();

            if (relpos.z == 0 && chk->neighbors.zn)
                chk->neighbors.zn->buildmesh();
            else if (relpos.z == chunk_size - 1 && chk->neighbors.zp)
                chk->neighbors.zp->buildmesh();

            std::cout << "relpos: " << relpos.tostring() << "\n";
        }

        wld.updateLoadedChunks();

        //////////////////////////////////////////DRAW

        glDisable(GL_BLEND);

        glUseProgram(resources.terrain.program);

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
        glRotatef(player.pitch * 180 / PI, 1, 0, 0);
        glRotatef(-player.yaw * 180 / PI, 0, 1, 0);
        glTranslatef(-player.pos.x, -player.pos.y, -player.pos.z);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        glActiveTexture(GL_TEXTURE0);
        glUniform1i(resources.terrain.facetextureloc, 0);
        glBindTexture(GL_TEXTURE_2D_ARRAY, resources.facetextures);

        glBindBuffer(GL_ARRAY_BUFFER, resources.vertexbuffer);
        glEnableClientState(GL_VERTEX_ARRAY);
        glVertexPointer(3, GL_FLOAT, sizeof(packedvert), (void*)0);
        glEnableClientState(GL_NORMAL_ARRAY);
        glNormalPointer(GL_FLOAT, sizeof(packedvert), (void*)sizeof(vec3));

        glUniform1i(resources.terrain.blocktexture, 1);
        glUniform1i(resources.terrain.lighttexture, 2);



        for (std::vector<vec3>::iterator iter = chunkPositions.begin(); iter != chunkPositions.end(); iter++)
        {
            vec3 pos = *iter;
            vec3 adjustedpos = pos + player.eyedir * 1.5f;
            if (player.eyedir.dot(adjustedpos * (1.f / adjustedpos.length())) > 0.6f)
            {
                int chkcoordx = floorf(player.pos.x / chunk_size) + pos.x;
                int chkcoordy = floorf(player.pos.y / chunk_size) + pos.y;
                int chkcoordz = floorf(player.pos.z / chunk_size) + pos.z;

                glUniform3f(resources.terrain.posoffset, chkcoordx * chunk_size, chkcoordy * chunk_size, chkcoordz * chunk_size);
                chunk *chk = wld.getChunk(chkcoordx, chkcoordy, chkcoordz);
                if (chk)
                {
                    chk->draw();
                }
            }

        }

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        for (std::vector<vec3>::iterator iter = chunkPositions.begin(); iter != chunkPositions.end(); iter++)
        {
            vec3 pos = *iter;
            vec3 adjustedpos = pos + player.eyedir * 1.5f;
            if (player.eyedir.dot(adjustedpos * (1.f / adjustedpos.length())) > 0.6f)
            {
                int chkcoordx = floorf(player.pos.x / chunk_size) + pos.x;
                int chkcoordy = floorf(player.pos.y / chunk_size) + pos.y;
                int chkcoordz = floorf(player.pos.z / chunk_size) + pos.z;

                glUniform3f(resources.terrain.posoffset, chkcoordx * chunk_size, chkcoordy * chunk_size, chkcoordz * chunk_size);
                chunk *chk = wld.getChunk(chkcoordx, chkcoordy, chkcoordz);
                if (chk)
                {
                    chk->drawtranslucent();
                }
            }

        }

        glDisableClientState(GL_VERTEX_ARRAY);
        glDisableClientState(GL_NORMAL_ARRAY);

        glUseProgram(0);

        if (hitres.hit)
        {
            vec3 cubepos(floorf(hitres.pos.x), floorf(hitres.pos.y), floorf(hitres.pos.z));
            glBegin(GL_LINES);

            if (hitres.hit == side_x)
                glColor3f(1.f, 0.f, 0.f);
            else if (hitres.hit == side_y)
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
