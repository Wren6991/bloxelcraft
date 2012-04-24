#ifndef _CHUNK_H_INCLUDED_
#define _CHUNK_H_INCLUDED_

#include <GL/glew.h>
#include "util.h"

const int chunk_size = 32;

typedef enum blocktype
{
    blk_air = 0,
    blk_stone,
    blk_dirt,
    blk_wood
} blocktype;


class chunk
{
    private:

    // position of bottom-left-near corner:
    vec3 chunkpos;

    GLuint vposbuffer;
    GLuint indexbuffer;
    GLuint blockidtexture;
    unsigned int ntriangles;

    char blocks[chunk_size * chunk_size * chunk_size];

    public:

    void buildmesh();
    void draw();

    chunk();
    chunk(vec3 chunkpos_);
};

#endif //_CHUNK_H_INCLUDED_