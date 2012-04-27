#include <map>

#include "chunk.h"

class world
{
    private:

    std::map <int, std::map <int, std::map <int, bool > > > chunkexists;
    std::map <int, std::map <int, std::map <int, chunk*> > > chunks;

    int chunksGenerated;

    public:

    chunk* getChunk(float x, float y, float z);
    chunk* getChunk(int x, int y, int z);
    chunk* getChunkAlways(float x, float y, float z);
    chunk* getChunkAlways(int x, int y, int z);     //For forcing chunk generation: may cause framerate drop, but necessary for block tracing algo.
    char getBlock(int x, int y, int z);
    char getBlock(float x, float y, float z);       //overload because we want floor for float->int instead of round towards 0.
    void setBlock(int x, int y, int z, char blockid);
    void setBlock(float x, float y, float z, char blockid);

    void updateLoadedChunks();

    world();
};

