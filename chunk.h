#ifndef _CHUNK_H_INCLUDED_
#define _CHUNK_H_INCLUDED_

#include <GL/glew.h>
#include "util.h"

const int chunk_size = 32;
const int nvertindices = (chunk_size + 1) * (chunk_size + 1) * (chunk_size + 1);

typedef enum blocktype
{
    blk_air = 0,
    blk_water,
    blk_stone,
    blk_dirt,
    blk_wood,
    blk_grass,
    blk_sand,
    blk_nof_blocks
} blocktype;

typedef enum face_orientation
{
    f_left = 0,
    f_right,
    f_down,
    f_up,
    f_near,
    f_far
} face_orientation;


class chunk
{
    private:

    // position of bottom-left-near corner:
    public: vec3 chunkpos;
    private:

    GLuint vposbuffer;
    GLuint indexbuffer;
    GLuint blocktexture;
    unsigned int ntriangles;

    char blocks[chunk_size * chunk_size * chunk_size];

    public:

    void buildmesh();
    void draw();
    char getBlock(float x, float y, float z);       //overload because we want floor for float->int instead of round towards 0.
    char getBlock(int x, int y, int z);
    void setBlock(float x, float y, float z, char blockid);       //overload because we want floor for float->int instead of round towards 0.
    void setBlock(int x, int y, int z, char blockid);

    chunk();
    chunk(vec3 chunkpos_);
};

#endif //_CHUNK_H_INCLUDED_
