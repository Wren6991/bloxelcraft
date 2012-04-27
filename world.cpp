#include "world.h"

#include <iostream>
#include <cmath>


world::world()
{
    std::cout << "LOL!\n";
    std::cout << "pval " << *((int*)&chunks[0][0][0]) << "\n";
    chunksGenerated = 0;
}


chunk* world::getChunk(int x, int y, int z)
{
    if (chunkexists[x][y][z])
    {
        return chunks[x][y][z];
    }
    else
    {
        //maximum of 3 chunks generated per frame - avoid stutter.
        if (chunksGenerated >= 3)
            return 0;
        chunks[x][y][z] = new chunk(vec3(x, y, z) * chunk_size);
        chunkexists[x][y][z] = true;
        chunksGenerated++;
        return chunks[x][y][z];
    }
}

chunk* world::getChunk(float x, float y, float z)
{
    return getChunk((int)floor(x), (int)floor(y), (int)floor(z));
}


chunk* world::getChunkAlways(int x, int y, int z)
{
    if (chunkexists[x][y][z])
    {
        return chunks[x][y][z];
    }
    else
    {
        chunks[x][y][z] = new chunk(vec3(x, y, z) * chunk_size);
        chunkexists[x][y][z] = true;
        chunksGenerated++;
        return chunks[x][y][z];
    }
}

chunk* world::getChunkAlways(float x, float y, float z)
{
    return getChunkAlways((int)floor(x), (int)floor(y), (int)floor(z));
}

char world::getBlock(int x, int y, int z)
{
    chunk* chk = getChunkAlways(x / chunk_size, y / chunk_size, z / chunk_size);
    return chk->getBlock(x % chunk_size, y % chunk_size, z % chunk_size);
}

char world::getBlock(float x, float y, float z)
{
    return getBlock((int)floor(x), (int)floor(y), (int)floor(z));
}

void world::setBlock(int x, int y, int z, char blockid)
{
    chunk* chk = getChunkAlways(x / chunk_size, y / chunk_size, z / chunk_size);
    chk->setBlock(x % chunk_size, y % chunk_size, z % chunk_size, blockid);
}

void world::setBlock(float x, float y, float z, char blockid)
{
    setBlock((int)floor(x), (int)floor(y), (int)floor(z), blockid);
}

void world::updateLoadedChunks()
{
    chunksGenerated = 0;
}
