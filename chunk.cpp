#include "chunk.h"
#include <vector>
#include <stdlib.h>
#include <iostream>

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
    srand(12345678);
    std::cout << "creating chunk at " << chunkpos_.x << " " << chunkpos_.y << " " << chunkpos_.z << "\n";
    chunkpos = chunkpos_;
    chunkpos.tostring();
    std::vector <vec3> positions;

    for (int i = 0; i <= chunk_size; i++)                           //array size is actually chunk_size + 1, as we have edge vertices to account for.
    {
        for (int j = 0; j <= chunk_size; j++)
        {
            for (int k = 0; k <= chunk_size; k++)
            {
                positions.push_back(chunkpos + vec3(i, j, k));

            }
        }
    }

    for (int i = 0; i < chunk_size; i++)
    {
        for (int j = 0; j < chunk_size; j++)
        {
            for (int k = 0; k < chunk_size; k++)
            {
                if (j - 10 - (rand() % 5) < 0)
                    blocks[blkindx(i, j, k)] = blk_stone;
                else
                    blocks[blkindx(i, j, k)] = blk_air;
            }
        }
    }


    glGenBuffers(1, &vposbuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vposbuffer);
    glBufferData(GL_ARRAY_BUFFER, (chunk_size + 1)*(chunk_size + 1)*(chunk_size + 1) * sizeof(vec3), &positions[0], GL_STATIC_DRAW);

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
                        vertices.push_back(indexfrompos(i    , j    , k    ));  //face: -k
                        vertices.push_back(indexfrompos(i    , j + 1, k    ));
                        vertices.push_back(indexfrompos(i + 1, j    , k    ));
                        vertices.push_back(indexfrompos(i + 1, j    , k    ));
                        vertices.push_back(indexfrompos(i    , j + 1, k    ));
                        vertices.push_back(indexfrompos(i + 1, j + 1, k    ));
                    }

                    if (k == chunk_size - 1 || !blocks[blkindx(i, j, k + 1)])
                    {
                        vertices.push_back(indexfrompos(i + 1, j    , k + 1));  //face: +k
                        vertices.push_back(indexfrompos(i    , j + 1, k + 1));
                        vertices.push_back(indexfrompos(i    , j    , k + 1));
                        vertices.push_back(indexfrompos(i + 1, j + 1, k + 1));
                        vertices.push_back(indexfrompos(i    , j + 1, k + 1));
                        vertices.push_back(indexfrompos(i + 1, j    , k + 1));
                    }

                    if (j == 0 || !blocks[blkindx(i, j - 1, k)])
                    {
                        vertices.push_back(indexfrompos(i    , j    , k    ));  //face: -j
                        vertices.push_back(indexfrompos(i + 1, j    , k    ));
                        vertices.push_back(indexfrompos(i    , j    , k + 1));
                        vertices.push_back(indexfrompos(i + 1, j    , k    ));
                        vertices.push_back(indexfrompos(i + 1, j    , k + 1));
                        vertices.push_back(indexfrompos(i    , j    , k + 1));
                    }

                    if (j == chunk_size - 1 || !blocks[blkindx(i, j + 1, k)])
                    {
                        vertices.push_back(indexfrompos(i    , j + 1, k + 1));  //face: +j
                        vertices.push_back(indexfrompos(i + 1, j + 1, k    ));
                        vertices.push_back(indexfrompos(i    , j + 1, k    ));
                        vertices.push_back(indexfrompos(i    , j + 1, k + 1));
                        vertices.push_back(indexfrompos(i + 1, j + 1, k + 1));
                        vertices.push_back(indexfrompos(i + 1, j + 1, k    ));
                    }

                    if (i == 0 || !blocks[blkindx(i - 1, j, k)])
                    {
                        vertices.push_back(indexfrompos(i    , j    , k    ));  //face: -i
                        vertices.push_back(indexfrompos(i    , j    , k + 1));
                        vertices.push_back(indexfrompos(i    , j + 1, k    ));
                        vertices.push_back(indexfrompos(i    , j    , k + 1));
                        vertices.push_back(indexfrompos(i    , j + 1, k + 1));
                        vertices.push_back(indexfrompos(i    , j + 1, k    ));
                    }

                    if (i == chunk_size - 1 || !blocks[blkindx(i + 1, j, k)])
                    {
                        vertices.push_back(indexfrompos(i + 1, j + 1, k    ));  //face: +i
                        vertices.push_back(indexfrompos(i + 1, j    , k + 1));
                        vertices.push_back(indexfrompos(i + 1, j    , k    ));
                        vertices.push_back(indexfrompos(i + 1, j + 1, k    ));
                        vertices.push_back(indexfrompos(i + 1, j + 1, k + 1));
                        vertices.push_back(indexfrompos(i + 1, j    , k + 1));
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
}


void chunk::draw()
{
    glBindBuffer(GL_ARRAY_BUFFER, vposbuffer);
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, 3 * sizeof(GLfloat), (void*)0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexbuffer);
    glDrawElements(GL_TRIANGLES, ntriangles * 3, GL_UNSIGNED_INT, (void*)0);
    glDisableClientState(GL_VERTEX_ARRAY);
}