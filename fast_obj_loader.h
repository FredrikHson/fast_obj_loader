#ifndef __FAST_OBJ_LOADER__
#define __FAST_OBJ_LOADER__

struct vec2
{
    float x,y;
};

struct vec3
{
    float x,y,z;
};

struct face
{
    unsigned int verts[4];
    unsigned int normals[4];
    unsigned int uvs[4];
    bool quad;
};

struct obj
{
    unsigned int numverts;

    vec3 *verts;

    unsigned int numfaces;

    obj()
    {
        verts=0;
        numverts=0;
    }
};

obj *loadObj(char *filename);

#endif
