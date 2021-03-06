#ifndef __FAST_OBJ_LOADER__
#define __FAST_OBJ_LOADER__

#include <stdlib.h>

struct vec2
{
    float x, y;
};

struct vec3
{
    float x, y, z;
};

struct face
{
    unsigned int verts[4];
    unsigned int normals[4];
    unsigned int uvs[4];
    bool quad;
};
struct triangle
{
    unsigned int verts[3];
    unsigned int normals[3];
    unsigned int uvs[3];
};
struct obj
{
    unsigned int numverts;
    unsigned int numnormals;
    unsigned int numuvs;
    unsigned int numfaces;

    vec3* verts;
    vec3* normals;
    vec2* uvs;
    triangle* faces;

    obj()
    {
        numverts   = 0;
        numnormals = 0;
        numuvs     = 0;
        numfaces   = 0;

        verts   = 0;
        normals = 0;
        uvs     = 0;
        faces   = 0;
    }
    ~obj()
    {
        if(verts)
        {
            free(verts);
        }

        if(normals)
        {
            free(normals);
        }

        if(uvs)
        {
            free(uvs);
        }

        if(faces)
        {
            free(faces);
        }
    }
};

obj* loadObj(const char* filename);
void writeObj(const char* filename, obj& input);
obj* ObjMakeUniqueFullVerts(const obj* orig);

#endif
