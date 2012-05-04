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
    return chunk_size * (chunk_size * x + y) + z;
}


chunk::chunk(vec3 chunkpos_)
{
    std::cout << "creating chunk at " << chunkpos_.x << " " << chunkpos_.y << " " << chunkpos_.z << "\n";
    chunkpos = chunkpos_;

    float heightfield[chunk_size][chunk_size];
    for (int i = 0; i < chunk_size; i++)
    {
        for (int j = 0; j < chunk_size; j++)
        {
            heightfield[i][j] = 5.f + 16.f * (glm::simplex(glm::vec2((i + chunkpos.x) * 0.003f, (j + chunkpos.z) * 0.003f)) * 0.25f + 0.75f) * fsimplex(glm::vec2((i + chunkpos.x) * 0.005f, (j + chunkpos.z) * 0.005f), 4, 0.5f);//20.0 * fbm((i + chunkpos.x) * 0.01f, (j + chunkpos.z) * 0.01f, 4, 2.f, 0.5f, 12345678);
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

    indexbuffer = 0;

    buildmesh();
    std::cout << "mesh built\n";
}

chunk::chunk()
{
    chunk(vec3(0, 0, 0));
}

void chunk::buildmesh()
{
    std::vector <GLuint> vertices;

    for (int i = 0; i < chunk_size; i++)
    {
        for (int j = 0; j < chunk_size; j++)
        {
            for (int k = 0; k < chunk_size; k++)
            {
                if (blocks[blkindx(i, j, k)])
                {
                    if (k == 0 || !blocks[blkindx(i, j, k - 1)])
                    {
                        vertices.push_back(f_near * nvertindices + indexfrompos(i    , j    , k    ));  //face: -k
                        vertices.push_back(f_near * nvertindices + indexfrompos(i    , j + 1, k    ));
                        vertices.push_back(f_near * nvertindices + indexfrompos(i + 1, j    , k    ));
                        vertices.push_back(f_near * nvertindices + indexfrompos(i + 1, j    , k    ));
                        vertices.push_back(f_near * nvertindices + indexfrompos(i    , j + 1, k    ));
                        vertices.push_back(f_near * nvertindices + indexfrompos(i + 1, j + 1, k    ));
                    }

                    if (k == chunk_size - 1 || !blocks[blkindx(i, j, k + 1)])
                    {
                        vertices.push_back(f_far * nvertindices + indexfrompos(i + 1, j    , k + 1));  //face: +k
                        vertices.push_back(f_far * nvertindices + indexfrompos(i    , j + 1, k + 1));
                        vertices.push_back(f_far * nvertindices + indexfrompos(i    , j    , k + 1));
                        vertices.push_back(f_far * nvertindices + indexfrompos(i + 1, j + 1, k + 1));
                        vertices.push_back(f_far * nvertindices + indexfrompos(i    , j + 1, k + 1));
                        vertices.push_back(f_far * nvertindices + indexfrompos(i + 1, j    , k + 1));
                    }

                    if (j == 0 || !blocks[blkindx(i, j - 1, k)])
                    {
                        vertices.push_back(f_down * nvertindices + indexfrompos(i    , j    , k    ));  //face: -j
                        vertices.push_back(f_down * nvertindices + indexfrompos(i + 1, j    , k    ));
                        vertices.push_back(f_down * nvertindices + indexfrompos(i    , j    , k + 1));
                        vertices.push_back(f_down * nvertindices + indexfrompos(i + 1, j    , k    ));
                        vertices.push_back(f_down * nvertindices + indexfrompos(i + 1, j    , k + 1));
                        vertices.push_back(f_down * nvertindices + indexfrompos(i    , j    , k + 1));
                    }

                    if (j == chunk_size - 1 || !blocks[blkindx(i, j + 1, k)])
                    {
                        vertices.push_back(f_up * nvertindices + indexfrompos(i    , j + 1, k + 1));  //face: +j
                        vertices.push_back(f_up * nvertindices + indexfrompos(i + 1, j + 1, k    ));
                        vertices.push_back(f_up * nvertindices + indexfrompos(i    , j + 1, k    ));
                        vertices.push_back(f_up * nvertindices + indexfrompos(i    , j + 1, k + 1));
                        vertices.push_back(f_up * nvertindices + indexfrompos(i + 1, j + 1, k + 1));
                        vertices.push_back(f_up * nvertindices + indexfrompos(i + 1, j + 1, k    ));
                    }

                    if (i == 0 || !blocks[blkindx(i - 1, j, k)])
                    {
                        vertices.push_back(f_left * nvertindices + indexfrompos(i    , j    , k    ));  //face: -i
                        vertices.push_back(f_left * nvertindices + indexfrompos(i    , j    , k + 1));
                        vertices.push_back(f_left * nvertindices + indexfrompos(i    , j + 1, k    ));
                        vertices.push_back(f_left * nvertindices + indexfrompos(i    , j    , k + 1));
                        vertices.push_back(f_left * nvertindices + indexfrompos(i    , j + 1, k + 1));
                        vertices.push_back(f_left * nvertindices + indexfrompos(i    , j + 1, k    ));
                    }

                    if (i == chunk_size - 1 || !blocks[blkindx(i + 1, j, k)])
                    {
                        vertices.push_back(f_right * nvertindices + indexfrompos(i + 1, j + 1, k    ));  //face: +i
                        vertices.push_back(f_right * nvertindices + indexfrompos(i + 1, j    , k + 1));
                        vertices.push_back(f_right * nvertindices + indexfrompos(i + 1, j    , k    ));
                        vertices.push_back(f_right * nvertindices + indexfrompos(i + 1, j + 1, k    ));
                        vertices.push_back(f_right * nvertindices + indexfrompos(i + 1, j + 1, k + 1));
                        vertices.push_back(f_right * nvertindices + indexfrompos(i + 1, j    , k + 1));
                    }
                }
            }
        }
    }

    glDeleteBuffers(1, &indexbuffer);

    glGenBuffers(1, &indexbuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexbuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, vertices.size() * sizeof(GLuint), &vertices[0], GL_STATIC_DRAW);
    ntriangles = vertices.size() / 3;
    std::cout << "Chunk has " << ntriangles << " triangles.\n";

    glBindTexture(GL_TEXTURE_3D, blocktexture);
    glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, chunk_size, chunk_size, chunk_size, GL_RED, GL_UNSIGNED_BYTE, (void*) blocks);
}


void chunk::draw()
{
    glBindTexture(GL_TEXTURE_3D, blocktexture);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexbuffer);
    glDrawElements(GL_TRIANGLES, ntriangles * 3, GL_UNSIGNED_INT, (void*)0);
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

