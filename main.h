const float PI = 3.14159265358979323846;

const float drawradius = 11.f;
const float drawsquashy = 1.5f;

const float playerspeed = 0.15;

std::vector <vec3> chunkPositions;
std::string progDirectory;
world wld;

struct
{
    struct
    {
        GLuint fshader;
        GLuint vshader;
        GLuint program;
        GLuint posoffset;
        GLuint blocktexture;
        GLuint facetextureloc;
        GLuint lighttexture;
    } terrain;

    struct
    {
        GLuint fshader;
        GLuint vshader;
        GLuint program;
        GLuint posoffset;
        GLuint blocktexture;
        GLuint facetextureloc;
        GLuint lighttexture;
    } water;

    GLuint vertexbuffer;
    GLuint facetextures;

} resources;

struct
{
    struct
    {
        bool W;
        bool A;
        bool S;
        bool D;
        bool space;
        bool MouseL;
        bool MouseR;
    } held;

    struct
    {
        /*bool W;
        bool A;
        bool S;
        bool D;*/
        bool MouseL;
        bool MouseR;
    } newPress;

} keys;

struct
{
    vec3 pos;
    vec3 vel;
    float pitch;
    float yaw;
    vec3 eyedir;
    bool isOnGround;
} player;

