#include "fast_obj_loader.h"
#include <stdio.h>
#include <stdlib.h>
#include "fastdynamic2.h"
#ifdef _OPENMP
#include <omp.h>
#else
#define omp_get_thread_num() 0
#define omp_get_num_threads() 1
#endif
#include <time.h>
#include <string.h>

unsigned int getNextFaceNumber(char *line, size_t &offset, unsigned char &type, unsigned char &nexttype, bool &more, bool &valid)
{
#define buflen 256
    unsigned int output = -1;
    char tmp[buflen + 1];
    type = nexttype;
    more = 1;
    unsigned int j = 0;

    for(unsigned int i = 0; i < buflen; i++)
    {
        switch(line[i + offset])
        {
            case 0:
                more = 0;
                i = buflen;
                nexttype = 0;
                break;

            case '/':
                offset += i + 1;
                i = buflen;
                nexttype++;
                break;

            case ' ':
                offset += i + 1;
                i = buflen;
                nexttype = 0;
                break;

            default:
                tmp[j] = line[i + offset];
                j++;
                break;
        }
    }

    if(j != 0)
    {
        tmp[j] = 0;
        valid = 1;
        output = atoi(tmp);
        //sscanf(tmp,"%99u",&output);  0.6seconds vs atoi 0.36seconds from 0.2seconds without face loading
    }
    else
    {
        valid = 0;
    }

    return output;
}
/*
void triangulate(obj *mesh)
{
    if(mesh == 0)
    {
        return;
    }

    // first count how many real faces will be needed

    #pragma omp parallel
    {
        size_t numTris = 0;
        #pragma omp for reduction(+:numTris)

        for(size_t i = 0; i < mesh->numfaces; i++)
        {
            if(mesh->faces[i].quad)
            {
                numTris += 2;
            }
            else
            {
                numTris += 1;
            }
        }

        #pragma omp single
        {

        }
    }
}
*/
// how much it should read at once to reduce the memory usage
#define MEMORYOVERHEAD 1048576
#define CONTAINER_SIZE 8192

obj *loadObj(const char *filename)
{
    bool hasquads = 0;
    obj *output = new obj;
    FILE *f = fopen(filename, "rb");

    if(!f)
    {
        printf("file not found %s\n", filename);
        return 0;
    }


    fseek(f, 0, SEEK_END);
    size_t filelength = ftell(f);
    fseek(f, 0, SEEK_SET);
    size_t bufferLength = filelength;

    if(bufferLength > MEMORYOVERHEAD) // read in chunks of as long as MEMORYOVERHEAD is defined
    {
        bufferLength = MEMORYOVERHEAD;
    }

    //printf("filesize=%zu bytes\n",filelength);
    char *memoryfile = new char[bufferLength];
    timespec start, stop;
    clock_gettime(CLOCK_REALTIME, &start);
    double calltime;
    size_t fileoffset = 0;


    while(fileoffset < filelength)
    {
        size_t numverts   = 0;
        size_t numnormals = 0;
        size_t numuvs     = 0;
        size_t numfaces   = 0;
        fseek(f, fileoffset, SEEK_SET);
        fread(memoryfile, bufferLength, 1, f);

        size_t linecount = 0;
        int numthreads   = 0;
        int numEnds      = 0;
        size_t vertsoffset   = output->numverts;
        size_t normalsoffset = output->numnormals;
        size_t uvsoffset     = output->numuvs;
        size_t facesoffset   = output->numfaces;
        size_t *lineends;
        FastDynamic<size_t> *tmpends;
        size_t *numtmpends;
        #pragma omp parallel // first get line ends so they can be parsed in parallel
        {
            numthreads = omp_get_num_threads();
            int threadid = omp_get_thread_num();
            #pragma omp single
            {
                tmpends = new FastDynamic<unsigned int>[numthreads];
                numtmpends = new size_t[numthreads];

                for(int i = 0; i < numthreads; i++)
                {
                    tmpends[i].SetContainer_size(CONTAINER_SIZE);
                    numtmpends[i] = 0;
                }
            }
            #pragma omp for reduction(+:linecount,numEnds)

            for(size_t i = 0; i < bufferLength; i++)
            {
                if(memoryfile[i] == '\n' || (i + fileoffset) >= filelength) // seems to work even with dos newlines \r\n
                {
                    tmpends[threadid][numEnds] = i;
                    numtmpends[threadid]++;
                    numEnds++;
                    linecount++;
                }
            }

            #pragma omp single
            {
                lineends = new size_t[numEnds + 2];
                lineends[0] = -1; // set to -1(max unsigned int) cause the read function later does a +1 to get around the \n
            }
            #pragma omp for

            for(int i = 0; i < numthreads; i++)
            {
                size_t offset = 1;

                for(int j = 0; j < i; j++)
                {
                    offset += numtmpends[j];
                }

                tmpends[i].CopyToStatic(&lineends[offset], numtmpends[i]);
            }

            #pragma omp single
            {
                fileoffset += lineends[numEnds - 1];
                delete [] numtmpends;
                delete [] tmpends;
            }
        }
        FastDynamic<vec3> *tmpverts;
        FastDynamic<vec3> *tmpnormals;
        FastDynamic<vec2> *tmpuvs;
        FastDynamic<triangle> *tmpfaces;
        size_t *numtmpverts;
        size_t *numtmpnormals;
        size_t *numtmpuvs;
        size_t *numtmpfaces;
        #pragma omp parallel
        {
            // read verts for now
            numthreads = omp_get_num_threads();
            int threadid = omp_get_thread_num();
            #pragma omp single
            {
                tmpverts = new FastDynamic<vec3>[numthreads];
                numtmpverts = new size_t[numthreads];

                for(int i = 0; i < numthreads; i++)
                {
                    tmpverts[i].SetContainer_size(CONTAINER_SIZE);
                    numtmpverts[i] = 0;
                }

                tmpnormals = new FastDynamic<vec3>[numthreads];
                numtmpnormals = new size_t[numthreads];

                for(int i = 0; i < numthreads; i++)
                {
                    tmpnormals[i].SetContainer_size(CONTAINER_SIZE);
                    numtmpnormals[i] = 0;
                }

                tmpuvs = new FastDynamic<vec2>[numthreads];
                numtmpuvs = new size_t[numthreads];

                for(int i = 0; i < numthreads; i++)
                {
                    tmpuvs[i].SetContainer_size(CONTAINER_SIZE);
                    numtmpuvs[i] = 0;
                }

                tmpfaces = new FastDynamic<triangle>[numthreads];
                numtmpfaces = new size_t[numthreads];

                for(int i = 0; i < numthreads; i++)
                {
                    tmpfaces[i].SetContainer_size(CONTAINER_SIZE);
                    numtmpfaces[i] = 0;
                }
            }
            #pragma omp for reduction(+:numverts,numfaces,numnormals,numuvs)

            for(int i = 1; i < numEnds; i++)
            {
                char line[1024] = {0};
                memcpy(&line, &memoryfile[lineends[i - 1] + 1], lineends[i] - lineends[i - 1] - 1);

                if(line[0] == 'v' && line[1] == ' ')
                {
                    char *l = line + 2;
                    char *tmpl;
                    vec3 vert;

                    vert.x = strtod(l, &tmpl);
                    l = tmpl;
                    vert.y = strtod(l, &tmpl);
                    l = tmpl;
                    vert.z = strtod(l, &tmpl);
                    tmpverts[threadid][numtmpverts[threadid]] = vert;
                    numtmpverts[threadid]++;
                    numverts++;
                }
                else if(line[0] == 'v' && line[1] == 'n' && line[2] == ' ')
                {
                    char *l = line + 3;
                    char *tmpl;
                    vec3 normal;

                    normal.x = strtod(l, &tmpl);
                    l = tmpl;
                    normal.y = strtod(l, &tmpl);
                    l = tmpl;
                    normal.z = strtod(l, &tmpl);
                    tmpnormals[threadid][numtmpnormals[threadid]] = normal;
                    numtmpnormals[threadid]++;
                    numnormals++;
                }
                else if(line[0] == 'v' && line[1] == 't' && line[2] == ' ')
                {
                    char *l = line + 3;
                    char *tmpl;
                    vec2 uv;

                    uv.x = strtod(l, &tmpl);
                    l = tmpl;
                    uv.y = strtod(l, &tmpl);
                    tmpuvs[threadid][numtmpuvs[threadid]] = uv;
                    numtmpuvs[threadid]++;
                    numuvs++;
                }
                else if(line[0] == 'f' && line[1] == ' ')
                {
                    char *data = line + 2;
                    size_t offset = 0;
                    bool more = 1;
                    unsigned char type = 0;
                    unsigned char nexttype = 0;
                    numfaces++;
                    face Face;
                    triangle &currenttri = tmpfaces[threadid][numtmpfaces[threadid]];
                    triangle tri;
                    tmpfaces[threadid][numtmpfaces[threadid]] = tri;
                    Face.quad = 0;
                    numtmpfaces[threadid]++;
                    unsigned char v[3] = {0};

                    while(more)
                    {
                        bool valid;
                        unsigned int faceidnum = getNextFaceNumber(data, offset, type, nexttype, more, valid);

                        if(valid)
                        {
                            switch(type)
                            {
                                case 0:
                                    if(v[0] == 4) // only support quads for now so so break out of loading more if it gets outside of that
                                    {
                                        hasquads = true;
                                        more = 0;
                                        break;
                                    }

                                    Face.verts[v[0]] = faceidnum;
                                    v[0]++;
                                    break;

                                case 1:
                                    if(v[1] == 4)
                                    {
                                        hasquads = true;
                                        more = 0;
                                        break;
                                    }

                                    Face.uvs[v[1]] = faceidnum;
                                    v[1]++;
                                    break;

                                case 2:
                                    if(v[2] == 4)
                                    {
                                        hasquads = true;
                                        more = 0;
                                        break;
                                    }

                                    Face.normals[v[2]] = faceidnum;
                                    v[2]++;
                                    break;
                            }
                        }
                    }


                    currenttri.verts[0] = Face.verts[0];
                    currenttri.verts[1] = Face.verts[1];
                    currenttri.verts[2] = Face.verts[2];

                    currenttri.uvs[0] = Face.uvs[0];
                    currenttri.uvs[1] = Face.uvs[1];
                    currenttri.uvs[2] = Face.uvs[2];

                    currenttri.normals[0] = Face.normals[0];
                    currenttri.normals[1] = Face.normals[1];
                    currenttri.normals[2] = Face.normals[2];

                    if(v[0] == 4)
                    {
                        triangle &currenttri2 = tmpfaces[threadid][numtmpfaces[threadid]];
                        numtmpfaces[threadid]++;
                        numfaces++;

                        currenttri2.verts[0] = Face.verts[2];
                        currenttri2.verts[1] = Face.verts[3];
                        currenttri2.verts[2] = Face.verts[0];

                        currenttri2.uvs[0] = Face.uvs[2];
                        currenttri2.uvs[1] = Face.uvs[3];
                        currenttri2.uvs[2] = Face.uvs[0];

                        currenttri2.normals[0] = Face.normals[2];
                        currenttri2.normals[1] = Face.normals[3];
                        currenttri2.normals[2] = Face.normals[0];
                    }
                }
            }

            #pragma omp single
            {
                if(numverts != 0)
                {
                    if(output->verts)
                    {
                        vec3 *tmpptr = (vec3 *)realloc(output->verts, sizeof(vec3) * (output->numverts + numverts));
                        output->verts = tmpptr;
                    }
                    else
                    {
                        output->verts = (vec3 *)malloc(sizeof(vec3) * numverts);
                    }
                }

                if(numnormals != 0)
                {
                    if(output->normals)
                    {
                        vec3 *tmpptr = (vec3 *)realloc(output->normals, sizeof(vec3) * (output->numnormals + numnormals));
                        output->normals = tmpptr;
                    }
                    else
                    {
                        output->normals = (vec3 *)malloc(sizeof(vec3) * numnormals);
                    }
                }

                if(numuvs != 0)
                {
                    if(output->uvs)
                    {
                        vec2 *tmpptr = (vec2 *)realloc(output->uvs, sizeof(vec2) * (output->numuvs + numuvs));
                        output->uvs = tmpptr;
                    }
                    else
                    {
                        output->uvs = (vec2 *)malloc(sizeof(vec2) * numuvs);
                    }
                }

                if(numfaces != 0)
                {
                    if(output->faces)
                    {
                        triangle *tmpptr = (triangle *)realloc(output->faces, sizeof(triangle) * (output->numfaces + numfaces));
                        output->faces = tmpptr;
                    }
                    else
                    {
                        output->faces = (triangle *)malloc(sizeof(triangle) * numfaces);
                    }
                }
            }
            #pragma omp for

            for(int i = 0; i < numthreads; i++)
            {
                size_t offset = vertsoffset;

                for(int j = 0; j < i; j++)
                {
                    offset += numtmpverts[j];
                }

                tmpverts[i].CopyToStatic(&(output->verts[offset]), numtmpverts[i]);
            }

            #pragma omp single
            {
                delete [] tmpverts;
                delete [] numtmpverts;
            }
            #pragma omp for

            for(int i = 0; i < numthreads; i++)
            {
                size_t offset = normalsoffset;

                for(int j = 0; j < i; j++)
                {
                    offset += numtmpnormals[j];
                }

                tmpnormals[i].CopyToStatic(&(output->normals[offset]), numtmpnormals[i]);
            }

            #pragma omp single
            {
                delete [] tmpnormals;
                delete [] numtmpnormals;
            }
            #pragma omp for

            for(int i = 0; i < numthreads; i++)
            {
                size_t offset = uvsoffset;

                for(int j = 0; j < i; j++)
                {
                    offset += numtmpuvs[j];
                }

                tmpuvs[i].CopyToStatic(&(output->uvs[offset]), numtmpuvs[i]);
            }

            #pragma omp single
            {
                delete [] tmpuvs;
                delete [] numtmpuvs;
            }
            #pragma omp for

            for(int i = 0; i < numthreads; i++)
            {
                size_t offset = facesoffset;

                for(int j = 0; j < i; j++)
                {
                    offset += numtmpfaces[j];
                }

                tmpfaces[i].CopyToStatic(&(output->faces[offset]), numtmpfaces[i]);
            }

            #pragma omp single
            {

                delete [] tmpfaces;
                delete [] numtmpfaces;
            }
            #pragma omp single
            {
                output->numverts += numverts;
                output->numuvs += numuvs;
                output->numnormals += numnormals;
                output->numfaces += numfaces;
                //printf("numfaces:%zu\t", output->numfaces);
                //printf("facesoffset:%zu\n", facesoffset);
                //printf("uvsoffset:%zu\n", uvsoffset);
                //printf("vertsoffset:%zu\n", vertsoffset);
                //printf("normalsoffset:%zu\n", normalsoffset);
            }
        }
        delete [] lineends;
    }

    if(hasquads)
    {
        printf("warning not triangulated\ntriangulate for better normalmap results\n");
        //triangulate(output);
    }

    //printf("lines:%zu\n",linecount);
    //printf("numverts:%zu\n", output->numverts);
    //printf("numuvs:%zu\n", output->numuvs);
    //printf("numnormals:%zu\n", output->numnormals);
    //printf("numfaces:%zu\n", output->numfaces);
    clock_gettime(CLOCK_REALTIME, &stop);

    delete [] memoryfile;
    fclose(f);
    return output;
}


void writeObj(const char *filename, obj &input)
{
    FILE *f = fopen(filename, "wb");

    if(!f)
    {
        return;
    }

    printf("writing obj verts:%u\n", input.numverts);

    for(int i = 0; i < input.numverts; i++)
    {
        fprintf(f, "v %f %f %f\n", input.verts[i].x, input.verts[i].y, input.verts[i].z);
    }

    for(int i = 0; i < input.numnormals; i++)
    {
        fprintf(f, "vn %f %f %f\n", input.normals[i].x, input.normals[i].y, input.normals[i].z);
    }

    for(int i = 0; i < input.numuvs; i++)
    {
        fprintf(f, "vt %f %f\n", input.uvs[i].x, input.uvs[i].y);
    }

    for(int i = 0; i < input.numfaces; i++)
    {
        if(input.numnormals == 0 && input.numuvs == 0)
        {
            fprintf(f, "f %u %u %u",
                    input.faces[i].verts[0],
                    input.faces[i].verts[1],
                    input.faces[i].verts[2]
                   );

            //if(input.faces[i].quad)
            //{
            //fprintf(f, " %u",
            //input.faces[i].verts[3]
            //);
            //}
        }
        else if(input.numnormals == 0 && input.numuvs != 0)
        {
            fprintf(f, "f %u/%u %u/%u %u/%u",
                    input.faces[i].verts[0],
                    input.faces[i].uvs[0],
                    input.faces[i].verts[1],
                    input.faces[i].uvs[1],
                    input.faces[i].verts[2],
                    input.faces[i].uvs[2]
                   );

            //if(input.faces[i].quad)
            //{
            //fprintf(f, " %u/%u",
            //input.faces[i].verts[3],
            //input.faces[i].uvs[3]
            //);
            //}
        }
        else if(input.numnormals != 0 && input.numuvs == 0)
        {
            fprintf(f, "f %u//%u %u//%u %u//%u",
                    input.faces[i].verts[0],
                    input.faces[i].normals[0],
                    input.faces[i].verts[1],
                    input.faces[i].normals[1],
                    input.faces[i].verts[2],
                    input.faces[i].normals[2]
                   );

            //if(input.faces[i].quad)
            //{
            //fprintf(f, " %u//%u",
            //input.faces[i].verts[3],
            //input.faces[i].normals[3]
            //);
            //}
        }
        else if(input.numnormals != 0 && input.numuvs != 0)
        {
            fprintf(f, "f %u/%u/%u %u/%u/%u %u/%u/%u",
                    input.faces[i].verts[0],
                    input.faces[i].uvs[0],
                    input.faces[i].normals[0],
                    input.faces[i].verts[1],
                    input.faces[i].uvs[1],
                    input.faces[i].normals[1],
                    input.faces[i].verts[2],
                    input.faces[i].uvs[2],
                    input.faces[i].normals[2]
                   );

            //if(input.faces[i].quad)
            //{
            //fprintf(f, " %u/%u/%u", input.faces[i].verts[3], input.faces[i].uvs[3], input.faces[i].normals[3]);
            //}
        }

        fprintf(f, "\n");
    }

    fclose(f);
}
