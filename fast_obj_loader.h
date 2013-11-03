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

struct obj
{
    int numverts;

    vec3 *verts;
    obj()
    {
        verts=0;
        numverts=0;
    }
};

obj *loadObj(char *filename);

#endif
