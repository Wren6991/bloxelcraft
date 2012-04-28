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
    int chkx, chky, chkz;
    chkx = floorf((float)x / chunk_size);
    chky = floorf((float)y / chunk_size);
    chkz = floorf((float)z / chunk_size);
    chunk* chk = getChunkAlways(chkx, chky, chkz);
    return chk->getBlock(x  - chkx * chunk_size, y - chky *chunk_size, z - chkz * chunk_size);
    // we can't just modulo these numbers with the chunk size.
    // consider the case x = -1;
    // - chunk = -1
    // - x % chunk_size = -1
    // - world pos = chunkpos * chunk_size + x (== -33)
    // - this is wrong!
}

char world::getBlock(float x, float y, float z)
{
    return getBlock((int)floor(x), (int)floor(y), (int)floor(z));
}

void world::setBlock(int x, int y, int z, char blockid)
{

    int chkx, chky, chkz;
    chkx = floorf((float)x / chunk_size);
    chky = floorf((float)y / chunk_size);
    chkz = floorf((float)z / chunk_size);
    chunk* chk = getChunkAlways(chkx, chky, chkz);
    chk->setBlock(x  - chkx * chunk_size, y - chky *chunk_size, z - chkz * chunk_size, blockid);
}

void world::setBlock(float x, float y, float z, char blockid)
{
    std::cout << "Float coords: " << x << ", " << y << ", " << z << "  ";
    setBlock((int)floor(x), (int)floor(y), (int)floor(z), blockid);
}

void world::updateLoadedChunks()
{
    chunksGenerated = 0;
}
