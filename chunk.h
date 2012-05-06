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

class chunk;

struct neighborlist
{
    chunk *xp;  //right
    chunk *xn;  //left
    chunk *yp;  //up
    chunk *yn;  //down
    chunk *zp;  //far
    chunk *zn;  //near
    neighborlist();
    neighborlist(chunk*, chunk*, chunk*, chunk*, chunk*, chunk*);
};

class chunk
{
    public:
    // position of bottom-left-near corner:
    vec3 chunkpos;
    neighborlist neighbors;

   private:

    GLuint vposbuffer;
    GLuint indexbuffer;
    GLuint waterindexbuffer;
    GLuint blocktexture;
    GLuint lighttexture;

    char blocks[chunk_size * chunk_size * chunk_size];
    char light[chunk_size * chunk_size * chunk_size];

    public:

    void buildmesh();
    void draw();
    void drawtranslucent();                         //water etc. - rendered in separate pass, so we can avoid depth buffer writes and cockups.
    char getBlock(float x, float y, float z);       //overload because we want floor for float->int instead of round towards 0.
    char getBlock(int x, int y, int z);
    void setBlock(float x, float y, float z, char blockid);       //overload because we want floor for float->int instead of round towards 0.
    void setBlock(int x, int y, int z, char blockid);
    unsigned int ntriangles;
    unsigned int ntranslucenttriangles;

    chunk();
    chunk(vec3 chunkpos_, neighborlist neighbors_);
};


#endif //_CHUNK_H_INCLUDED_
