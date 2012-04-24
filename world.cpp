#include "world.h"

#include <iostream>


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
        chunks[x][y][z] = new chunk(vec3(x, y, z) * chunk_size - vec3(0, 16, 0));
        chunkexists[x][y][z] = true;
        chunksGenerated++;
        return chunks[x][y][z];
    }
}

void world::updateLoadedChunks()
{
    chunksGenerated = 0;
}
