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
    chunk* getChunkAlways(int x, int y, int z);             //For forcing chunk generation: may cause framerate drop, but necessary for block tracing algo.
    chunk* getChunkNoGenerate(int x, int y, int z);
    chunk* getChunkNoGenerate(float x, float y, float z);   //Don't try to generate the chunk if it doesn't exist: used for adding neighbours to chunks at generation, as generating neighbours would lead to recursion.

    char getBlock(int x, int y, int z);
    char getBlock(float x, float y, float z);               //overload because we want floorf() for float->int instead of round towards 0 (straight cast).
    void setBlock(int x, int y, int z, char blockid);
    void setBlock(float x, float y, float z, char blockid);

    void updateLoadedChunks();

    world();
};

