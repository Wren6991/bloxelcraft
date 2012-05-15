#include "chunk.h"

#include <vector>
#include <stdlib.h>
#include <iostream>
#include <cmath>

//#include "noise.h"

#include "J:/CodeBlocks/MinGW/include/glm/gtc/noise.hpp"

inline float fsimplex(glm::vec2 pos, int octaves, float persistence)
{
    if (octaves > 0)
        return glm::simplex(pos) + persistence * fsimplex(pos * 2.f, octaves - 1, persistence);
    else
        return 0;
}

inline float fsimplex(glm::vec3 pos, int octaves, float persistence)
{
    if (octaves > 0)
        return glm::simplex(pos) + persistence * fsimplex(pos * 2.f, octaves - 1, persistence);
    else
        return 0;
}


inline int indexfrompos(int x, int y, int z)
{
    return (chunk_size + 1)*((chunk_size + 1) * x + y) + z;              // x * sz^2 + y * sz + z: maps cube to linear "snake space" order of vertices.  sz is factored out of the first two terms.
}

inline int blkindx(int x, int y, int z)
{
    return chunk_size * (chunk_size * z + y) + x;                       // zyx to avoid swizzle mistake in fragment shader (so that opengl reads the texture correctly)
}


chunk::chunk(vec3 chunkpos_, neighborlist neighbors_)
{
    //std::cout << "creating chunk at " << chunkpos_.x << " " << chunkpos_.y << " " << chunkpos_.z << "\n";
    chunkpos = chunkpos_;
    neighbors = neighbors_;

    if (neighbors.xn)
        neighbors.xn->neighbors.xp = this;
    if (neighbors.xp)
        neighbors.xp->neighbors.xn = this;
    if (neighbors.yn)
        neighbors.yn->neighbors.yp = this;
    if (neighbors.yp)
        neighbors.yp->neighbors.yn = this;
    if (neighbors.zn)
        neighbors.zn->neighbors.zp = this;
    if (neighbors.zp)
        neighbors.zp->neighbors.zn = this;

    float heightfield[chunk_size][chunk_size];
    for (int i = 0; i < chunk_size; i++)
    {
        for (int j = 0; j < chunk_size; j++)
        {
            heightfield[i][j] = 5.f + 20.f * (glm::simplex(glm::vec2((i + chunkpos.x) * 0.003f, (j + chunkpos.z) * 0.003f)) * 0.25f + 0.75f) * fsimplex(glm::vec2((i + chunkpos.x) * 0.005f, (j + chunkpos.z) * 0.005f), 4, 0.5f);//20.0 * fbm((i + chunkpos.x) * 0.01f, (j + chunkpos.z) * 0.01f, 4, 2.f, 0.5f, 12345678);
        }
    }

    float offsx = chunkpos.x;
    float offsy = chunkpos.y;
    float offsz = chunkpos.z;

    for (int i = 0; i < chunk_size; i++)
    {
        float x = i + offsx;
        for (int j = 0; j < chunk_size; j++)
        {
            float y = j + offsy;
            for (int k = 0; k < chunk_size; k++)
            {
                float z = k + offsz;
                float density = heightfield[i][k] - y;
                if (density > 0)
                {
                    blocks[blkindx(i, j, k)] =  y < -5.f? blk_stone : y < 3 ? blk_sand : heightfield[i][k] - y < 1 ? blk_grass : blk_dirt;
                }
                else
                {
                    blocks[blkindx(i, j, k)] = y <= 0 ? blk_water : blk_air;
                }
            }
        }
    }

    glGenTextures(1, &blocktexture);
    glBindTexture(GL_TEXTURE_3D, blocktexture);

    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP);

    glTexImage3D(GL_TEXTURE_3D, 0, GL_R8, chunk_size, chunk_size, chunk_size, 0, GL_RED, GL_UNSIGNED_BYTE, (void*) 0);  //don't upload it yet - we'll do that in the buildmesh method.

    glGenTextures(1, &lighttexture);
    glBindTexture(GL_TEXTURE_3D, lighttexture);

    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP);

    glTexImage3D(GL_TEXTURE_3D, 0, GL_R8, chunk_size, chunk_size, chunk_size, 0, GL_RED, GL_UNSIGNED_BYTE, (void*) 0);

    indexbuffer = 0;

    buildmesh();
    //std::cout << "mesh built\n";

    if (neighbors.xn)
        neighbors.xn->buildmesh();
    if (neighbors.xp)
        neighbors.xp->buildmesh();
    if (neighbors.yn)
        neighbors.yn->buildmesh();
    if (neighbors.yp)
        neighbors.yp->buildmesh();
    if (neighbors.zn)
        neighbors.zn->buildmesh();
    if (neighbors.zp)
        neighbors.zp->buildmesh();
}

chunk::chunk()
{
    chunk(vec3(0, 0, 0), neighborlist());
}

void chunk::buildmesh()
{
    std::vector <GLuint> vertices;
    std::vector <GLuint> translucentverts;

    for (int i = 0; i < chunk_size; i++)
    {
        for (int j = 0; j < chunk_size; j++)
        {
            for (int k = 0; k < chunk_size; k++)
            {
                char blk = blocks[blkindx(i, j, k)];
                if (blk > blk_water)
                {
                    if (k == 0?  !neighbors.zn || neighbors.zn->blocks[blkindx(i, j, chunk_size - 1)] <= blk_water  :  blocks[blkindx(i, j, k - 1)] <= blk_water)       //emit face if: On the edge and no neigbour || on the edge and neighbouring chunk has no adjacent block || inside chunk and no adjacent block.
                    {
                        vertices.push_back(f_near * nvertindices + indexfrompos(i    , j    , k    ));  //face: -k
                        vertices.push_back(f_near * nvertindices + indexfrompos(i    , j + 1, k    ));
                        vertices.push_back(f_near * nvertindices + indexfrompos(i + 1, j    , k    ));
                        vertices.push_back(f_near * nvertindices + indexfrompos(i + 1, j    , k    ));
                        vertices.push_back(f_near * nvertindices + indexfrompos(i    , j + 1, k    ));
                        vertices.push_back(f_near * nvertindices + indexfrompos(i + 1, j + 1, k    ));
                    }

                    if (k == chunk_size - 1?  !neighbors.zp || neighbors.zp->blocks[blkindx(i, j, 0)] <= blk_water  :  blocks[blkindx(i, j, k + 1)] <= blk_water)
                    {
                        vertices.push_back(f_far * nvertindices + indexfrompos(i + 1, j    , k + 1));  //face: +k
                        vertices.push_back(f_far * nvertindices + indexfrompos(i    , j + 1, k + 1));
                        vertices.push_back(f_far * nvertindices + indexfrompos(i    , j    , k + 1));
                        vertices.push_back(f_far * nvertindices + indexfrompos(i + 1, j + 1, k + 1));
                        vertices.push_back(f_far * nvertindices + indexfrompos(i    , j + 1, k + 1));
                        vertices.push_back(f_far * nvertindices + indexfrompos(i + 1, j    , k + 1));
                    }

                    if (j == 0?  !neighbors.yn || neighbors.yn->blocks[blkindx(i, chunk_size - 1, k)] <= blk_water  :  blocks[blkindx(i, j - 1, k)] <= blk_water)
                    {
                        vertices.push_back(f_down * nvertindices + indexfrompos(i    , j    , k    ));  //face: -j
                        vertices.push_back(f_down * nvertindices + indexfrompos(i + 1, j    , k    ));
                        vertices.push_back(f_down * nvertindices + indexfrompos(i    , j    , k + 1));
                        vertices.push_back(f_down * nvertindices + indexfrompos(i + 1, j    , k    ));
                        vertices.push_back(f_down * nvertindices + indexfrompos(i + 1, j    , k + 1));
                        vertices.push_back(f_down * nvertindices + indexfrompos(i    , j    , k + 1));
                    }

                    if (j == chunk_size - 1?  !neighbors.yp || neighbors.yp->blocks[blkindx(i, 0, k)] <= blk_water  :  blocks[blkindx(i, j + 1, k)] <= blk_water)
                    {
                        vertices.push_back(f_up * nvertindices + indexfrompos(i    , j + 1, k + 1));  //face: +j
                        vertices.push_back(f_up * nvertindices + indexfrompos(i + 1, j + 1, k    ));
                        vertices.push_back(f_up * nvertindices + indexfrompos(i    , j + 1, k    ));
                        vertices.push_back(f_up * nvertindices + indexfrompos(i    , j + 1, k + 1));
                        vertices.push_back(f_up * nvertindices + indexfrompos(i + 1, j + 1, k + 1));
                        vertices.push_back(f_up * nvertindices + indexfrompos(i + 1, j + 1, k    ));
                    }

                    if (i == 0?  !neighbors.xn || neighbors.xn->blocks[blkindx(chunk_size - 1, j, k)] <= blk_water  :  blocks[blkindx(i - 1, j, k)] <= blk_water)
                    {
                        vertices.push_back(f_left * nvertindices + indexfrompos(i    , j    , k    ));  //face: -i
                        vertices.push_back(f_left * nvertindices + indexfrompos(i    , j    , k + 1));
                        vertices.push_back(f_left * nvertindices + indexfrompos(i    , j + 1, k    ));
                        vertices.push_back(f_left * nvertindices + indexfrompos(i    , j    , k + 1));
                        vertices.push_back(f_left * nvertindices + indexfrompos(i    , j + 1, k + 1));
                        vertices.push_back(f_left * nvertindices + indexfrompos(i    , j + 1, k    ));
                    }

                    if (i == chunk_size - 1?  !neighbors.xp || neighbors.xp->blocks[blkindx(0, j, k)] <= blk_water  :  blocks[blkindx(i + 1, j, k)] <= blk_water)
                    {
                        vertices.push_back(f_right * nvertindices + indexfrompos(i + 1, j + 1, k    ));  //face: +i
                        vertices.push_back(f_right * nvertindices + indexfrompos(i + 1, j    , k + 1));
                        vertices.push_back(f_right * nvertindices + indexfrompos(i + 1, j    , k    ));
                        vertices.push_back(f_right * nvertindices + indexfrompos(i + 1, j + 1, k    ));
                        vertices.push_back(f_right * nvertindices + indexfrompos(i + 1, j + 1, k + 1));
                        vertices.push_back(f_right * nvertindices + indexfrompos(i + 1, j    , k + 1));
                    }
                }
                else if (blk)           //handle translucent blocks in a separate buffer.
                {
                    if (k == 0?  !neighbors.zn || neighbors.zn->blocks[blkindx(i, j, chunk_size - 1)] < blk  :  blocks[blkindx(i, j, k - 1)] < blk)       //emit face if: On the edge and no neigbour || on the edge and neighbouring chunk has no adjacent block || inside chunk and no adjacent block.
                    {
                        translucentverts.push_back(f_near * nvertindices + indexfrompos(i    , j    , k    ));  //face: -k
                        translucentverts.push_back(f_near * nvertindices + indexfrompos(i    , j + 1, k    ));
                        translucentverts.push_back(f_near * nvertindices + indexfrompos(i + 1, j    , k    ));
                        translucentverts.push_back(f_near * nvertindices + indexfrompos(i + 1, j    , k    ));
                        translucentverts.push_back(f_near * nvertindices + indexfrompos(i    , j + 1, k    ));
                        translucentverts.push_back(f_near * nvertindices + indexfrompos(i + 1, j + 1, k    ));
                    }

                    if (k == chunk_size - 1?  !neighbors.zp || neighbors.zp->blocks[blkindx(i, j, 0)] < blk  :  blocks[blkindx(i, j, k + 1)] < blk)
                    {
                        translucentverts.push_back(f_far * nvertindices + indexfrompos(i + 1, j    , k + 1));  //face: +k
                        translucentverts.push_back(f_far * nvertindices + indexfrompos(i    , j + 1, k + 1));
                        translucentverts.push_back(f_far * nvertindices + indexfrompos(i    , j    , k + 1));
                        translucentverts.push_back(f_far * nvertindices + indexfrompos(i + 1, j + 1, k + 1));
                        translucentverts.push_back(f_far * nvertindices + indexfrompos(i    , j + 1, k + 1));
                        translucentverts.push_back(f_far * nvertindices + indexfrompos(i + 1, j    , k + 1));
                    }

                    if (j == 0?  !neighbors.yn || neighbors.yn->blocks[blkindx(i, chunk_size - 1, k)] < blk  :  blocks[blkindx(i, j - 1, k)] < blk)
                    {
                        translucentverts.push_back(f_down * nvertindices + indexfrompos(i    , j    , k    ));  //face: -j
                        translucentverts.push_back(f_down * nvertindices + indexfrompos(i + 1, j    , k    ));
                        translucentverts.push_back(f_down * nvertindices + indexfrompos(i    , j    , k + 1));
                        translucentverts.push_back(f_down * nvertindices + indexfrompos(i + 1, j    , k    ));
                        translucentverts.push_back(f_down * nvertindices + indexfrompos(i + 1, j    , k + 1));
                        translucentverts.push_back(f_down * nvertindices + indexfrompos(i    , j    , k + 1));
                    }

                    if (j == chunk_size - 1?  !neighbors.yp || neighbors.yp->blocks[blkindx(i, 0, k)] < blk  :  blocks[blkindx(i, j + 1, k)] < blk)
                    {
                        translucentverts.push_back(f_up * nvertindices + indexfrompos(i    , j + 1, k + 1));  //face: +j
                        translucentverts.push_back(f_up * nvertindices + indexfrompos(i + 1, j + 1, k    ));
                        translucentverts.push_back(f_up * nvertindices + indexfrompos(i    , j + 1, k    ));
                        translucentverts.push_back(f_up * nvertindices + indexfrompos(i    , j + 1, k + 1));
                        translucentverts.push_back(f_up * nvertindices + indexfrompos(i + 1, j + 1, k + 1));
                        translucentverts.push_back(f_up * nvertindices + indexfrompos(i + 1, j + 1, k    ));
                    }

                    if (i == 0?  !neighbors.xn || neighbors.xn->blocks[blkindx(chunk_size - 1, j, k)] < blk  :  blocks[blkindx(i - 1, j, k)] < blk)
                    {
                        translucentverts.push_back(f_left * nvertindices + indexfrompos(i    , j    , k    ));  //face: -i
                        translucentverts.push_back(f_left * nvertindices + indexfrompos(i    , j    , k + 1));
                        translucentverts.push_back(f_left * nvertindices + indexfrompos(i    , j + 1, k    ));
                        translucentverts.push_back(f_left * nvertindices + indexfrompos(i    , j    , k + 1));
                        translucentverts.push_back(f_left * nvertindices + indexfrompos(i    , j + 1, k + 1));
                        translucentverts.push_back(f_left * nvertindices + indexfrompos(i    , j + 1, k    ));
                    }

                    if (i == chunk_size - 1?  !neighbors.xp || neighbors.xp->blocks[blkindx(0, j, k)] < blk  :  blocks[blkindx(i + 1, j, k)] < blk)
                    {
                        translucentverts.push_back(f_right * nvertindices + indexfrompos(i + 1, j + 1, k    ));  //face: +i
                        translucentverts.push_back(f_right * nvertindices + indexfrompos(i + 1, j    , k + 1));
                        translucentverts.push_back(f_right * nvertindices + indexfrompos(i + 1, j    , k    ));
                        translucentverts.push_back(f_right * nvertindices + indexfrompos(i + 1, j + 1, k    ));
                        translucentverts.push_back(f_right * nvertindices + indexfrompos(i + 1, j + 1, k + 1));
                        translucentverts.push_back(f_right * nvertindices + indexfrompos(i + 1, j    , k + 1));
                    }
                }

                /*if(blocks[blkindx(i, j, k)] > blk_water)
                {
                    light[blkindx(i, j, k)] = 8;
                }
                else
                {
                    light[blkindx(i, j, k)] = 15;
                }*/

            }
        }
    }

    glDeleteBuffers(1, &indexbuffer);

    glGenBuffers(1, &indexbuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexbuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, vertices.size() * sizeof(GLuint), &vertices[0], GL_STATIC_DRAW);
    ntriangles = vertices.size() / 3;
    //std::cout << "Chunk has " << ntriangles << " triangles.\n";

    glGenBuffers(1, &waterindexbuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, waterindexbuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, translucentverts.size() * sizeof(GLuint), &translucentverts[0], GL_STATIC_DRAW);
    ntranslucenttriangles = translucentverts.size() / 3;

    glBindTexture(GL_TEXTURE_3D, blocktexture);
    glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, chunk_size, chunk_size, chunk_size, GL_RED, GL_UNSIGNED_BYTE, (void*) blocks);

    //glBindTexture(GL_TEXTURE_3D, lighttexture);
    //glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, chunk_size, chunk_size, chunk_size, GL_RED, GL_UNSIGNED_BYTE, (void*) light);
}


void chunk::draw()
{
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_3D, blocktexture);
    //glActiveTexture(GL_TEXTURE2);
    //glBindTexture(GL_TEXTURE_3D, lighttexture);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexbuffer);
    glDrawElements(GL_TRIANGLES, ntriangles * 3, GL_UNSIGNED_INT, (void*)0);
}

void chunk::drawtranslucent()
{
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_3D, blocktexture);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, waterindexbuffer);
    glDrawElements(GL_TRIANGLES, ntranslucenttriangles * 3, GL_UNSIGNED_INT, (void*)0);
}

char chunk::getBlock(int x, int y, int z)
{
    return blocks[blkindx(x, y, z)];
}

char chunk::getBlock(float x, float y, float z)
{
    return blocks[blkindx(floor(x), floor(y), floor(z))];
}

void chunk::setBlock(int x, int y, int z, char blockid)
{
    std::cout << "setting block at location " << chunkpos.x + x << ", " << chunkpos.y + y << ", " << chunkpos.z + z << " to " << blockid << "\n";
    blocks[blkindx(x, y, z)] = blockid;
}

void chunk::setBlock(float x, float y, float z, char blockid)
{
    std::cout << "setting block at location " << chunkpos.x + x << ", " << chunkpos.y + y << ", " << chunkpos.z + z << " to " << blockid << "\n";
    blocks[blkindx(floor(x), floor(y), floor(z))] = blockid;
}

neighborlist::neighborlist()
{
    xp = xn = yp = yn = zp = zn = NULL;
}

neighborlist::neighborlist(chunk *xp_, chunk *xn_, chunk *yp_, chunk *yn_, chunk *zp_, chunk *zn_)
{
    xp = xp_;
    xn = xn_;
    yp = yp_;
    yn = yn_;
    zp = zp_;
    zn = zn_;
}

hitresult::hitresult (vec3 pos_, side_enum hit_, blocktype type_)
{
    pos = pos_;
    hit = hit_;
    type = type_;
}


