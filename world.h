#include <map>

#include "chunk.h"

class world
{
    private:

    std::map <int, std::map <int, std::map <int, bool > > > chunkexists;
    std::map <int, std::map <int, std::map <int, chunk*> > > chunks;

    int chunksGenerated;

    public:

    chunk* getChunk(int x, int y, int z);

    void updateLoadedChunks();

    world();
};

