#include "fast_obj_loader.h"
#include <stdio.h>
#include <stdlib.h>
#include "fastdynamic.h"
#ifdef _OPENMP
#include <omp.h>
#endif
#include <time.h>
#include <string.h>
//bool getcstr(char *out,unsigned short bufflen,char *input,size_t &len) // bufflen = output buffer size // input has to be a copy of the original pointer
//{
//    if(len<=0)
//        return false;


//    unsigned short i=0;
//    while
//}

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
        valid = 0;
    return output;
}
// how much it should read at once to reduce the memory usage
#define MEMORYOVERHEAD 1048576
obj *loadObj(const char *filename)
{
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
        bufferLength = MEMORYOVERHEAD;
    //printf("filesize=%zu bytes\n",filelength);
    char *memoryfile = new char[bufferLength];

    fread(memoryfile, bufferLength, 1, f);
    timespec start, stop;
    clock_gettime(CLOCK_REALTIME, &start);
    double calltime;


    size_t linecount  = 0;
    int numthreads    = 0;
    int numEnds       = 0;
    size_t numverts   = 0;
    size_t numnormals = 0;
    size_t numuvs     = 0;
    size_t numfaces   = 0;
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
                tmpends[i].SetContainer_size(8192);
                numtmpends[i] = 0;
            }
        }

        #pragma omp for reduction(+:linecount,numEnds)
        for(size_t i = 0; i < bufferLength; i++)
        {
            //printf("%i\n",i);
            if(memoryfile[i] == '\n') // seems to work even with dos newlines \r\n
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
            lineends[numEnds + 1] = bufferLength;
            numEnds += 2;
        }
        #pragma omp for
        for(int i = 0; i < numthreads; i++)
        {
            int offset = 1;
            for(int j = 0; j < i; j++)
                offset += numtmpends[j];
            tmpends[i].CopyToStatic(&lineends[offset], numtmpends[i]);
        }
        #pragma omp single
        {
            delete [] numtmpends;
            delete [] tmpends;
        }
    }
    FastDynamic<vec3> *tmpverts;
    FastDynamic<vec3> *tmpnormals;
    FastDynamic<vec2> *tmpuvs;
    FastDynamic<face> *tmpfaces;
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
                tmpverts[i].SetContainer_size(8192);
                numtmpverts[i] = 0;
            }

            tmpnormals = new FastDynamic<vec3>[numthreads];
            numtmpnormals = new size_t[numthreads];
            for(int i = 0; i < numthreads; i++)
            {
                tmpnormals[i].SetContainer_size(8192);
                numtmpnormals[i] = 0;
            }

            tmpuvs = new FastDynamic<vec2>[numthreads];
            numtmpuvs = new size_t[numthreads];
            for(int i = 0; i < numthreads; i++)
            {
                tmpuvs[i].SetContainer_size(8192);
                numtmpuvs[i] = 0;
            }

            tmpfaces = new FastDynamic<face>[numthreads];
            numtmpfaces = new size_t[numthreads];
            for(int i = 0; i < numthreads; i++)
            {
                tmpfaces[i].SetContainer_size(8192);
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
                face Face = {0};
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
                                    more = 0;
                                    break;
                                }
                                Face.verts[v[0]] = faceidnum;
                                v[0]++;
                                break;
                            case 1:
                                if(v[1] == 4)
                                {
                                    more = 0;
                                    break;
                                }
                                Face.uvs[v[1]] = faceidnum;
                                v[1]++;
                                break;
                            case 2:
                                if(v[2] == 4)
                                {
                                    more = 0;
                                    break;
                                }
                                Face.normals[v[2]] = faceidnum;
                                v[2]++;
                                break;
                        }
                    }
                }
                //    printf("\n");
            }
        }
        #pragma omp single
        {
            if(numverts != 0)
                output->verts = new vec3[numverts];
            if(numnormals != 0)
                output->normals = new vec3[numnormals];
            if(numuvs != 0)
                output->uvs = new vec2[numuvs];
            if(numfaces != 0)
                output->faces = new face[numfaces];
        }
        #pragma omp for
        for(int i = 0; i < numthreads; i++)
        {
            int offset = 0;
            for(int j = 0; j < i; j++)
                offset += numtmpverts[j];
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
            int offset = 0;
            for(int j = 0; j < i; j++)
                offset += numtmpnormals[j];
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
            int offset = 0;
            for(int j = 0; j < i; j++)
                offset += numtmpuvs[j];
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
            int offset = 0;
            for(int j = 0; j < i; j++)
                offset += numtmpfaces[j];
            tmpfaces[i].CopyToStatic(&(output->faces[offset]), numtmpfaces[i]);
        }
        #pragma omp single
        {
            delete [] tmpfaces;
            delete [] numtmpfaces;
        }
    }

    delete [] lineends;
    //printf("lines:%zu\n",linecount);
    //printf("numthreads:%i\n",numthreads);
    //printf("numverts:%zu\n",numverts);
    //printf("numuvs:%zu\n",numuvs);
    //printf("numnormals:%zu\n",numnormals);
    //printf("numfaces:%zu\n",numfaces);

    clock_gettime(CLOCK_REALTIME, &stop);
    calltime = (stop.tv_sec - start.tv_sec) + (stop.tv_nsec - start.tv_nsec) / 1000000000.0;
    //    printf("done parsing file %lfseconds\n",calltime);

    delete [] memoryfile;

    fclose(f);
    return output;
}
