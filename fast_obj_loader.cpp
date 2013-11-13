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

unsigned int getNextFaceNumber(char *line,size_t &offset,unsigned char &type,unsigned char &nexttype,bool &more,bool &valid)
{
#define buflen 256
    unsigned int output=-1;
    char tmp[buflen+1];
    type=nexttype;
    more = 1;
    unsigned int j=0;
    for(unsigned int i=0;i<buflen;i++)
    {
        switch(line[i+offset])
        {
            case 0:
                more = 0;
                i=buflen;
                nexttype=0;
                break;

            case '/':
                offset+=i+1;
                i=buflen;
                nexttype++;
                break;
            case ' ':
                offset+=i+1;
                i=buflen;
                nexttype=0;
                break;
            default:
                tmp[j]=line[i+offset];
                j++;
                break;

        }
    }
    if(j!=0)
    {
        tmp[j]=0;
        valid=1;
        output=atoi(tmp);
        //sscanf(tmp,"%99u",&output);  0.6seconds vs atoi 0.36seconds from 0.2seconds without face loading
    }
    else
    {
        valid=0;
    }
    return output;
}

obj *loadObj(const char *filename)
{
    obj *output=new obj;

    FILE *f=fopen(filename,"rb");
    if(!f)
    {
        printf("file not found %s\n",filename);
        return 0;
    }
    fseek(f,0,SEEK_END);
    size_t filelength=ftell(f);
    fseek(f,0,SEEK_SET);
    //printf("filesize=%zu bytes\n",filelength);
    char *memoryfile=new char[filelength];

    fread(memoryfile,filelength,1,f);
    timespec start,stop;
    clock_gettime(CLOCK_REALTIME, &start );
    double calltime;


    size_t linecount=0;
    int numthreads=0;
    int numEnds=0;
    size_t numverts=0;
    size_t *lineends;
    FastDynamic<size_t> *tmpends;
    size_t *numtmpends;
    #pragma omp parallel // first get line ends so they can be parsed in parallel
    {
        numthreads=omp_get_num_threads();
        int threadid=omp_get_thread_num();

        #pragma omp single
        {
            tmpends=new FastDynamic<unsigned int>[numthreads];
            numtmpends=new size_t[numthreads];
            for(int i=0;i<numthreads;i++)
            {
                tmpends[i].SetContainer_size(8192);
                numtmpends[i]=0;
            }
        }

        #pragma omp for reduction(+:linecount,numEnds)
        for(size_t i=0;i<filelength;i++)
        {
            //printf("%i\n",i);
            if(memoryfile[i]=='\n')
            {
                tmpends[threadid][numEnds]=i;
                numtmpends[threadid]++;
                numEnds++;
                linecount++;
            }
        }
        #pragma omp single
        {
            lineends=new size_t[numEnds+1];
            lineends[numEnds+1]=filelength;
        }
        #pragma omp for
        for(int i=0;i<numthreads;i++)
        {
            int offset=0;
            for(int j=0;j<i;j++)
            {
                offset+=numtmpends[j];
            }
            tmpends[i].CopyToStatic(&lineends[offset],numtmpends[i]);
        }
        #pragma omp single
        {
            delete [] numtmpends;
            delete [] tmpends;
        }
    }
    FastDynamic<vec3> *tmpverts;
    size_t *numtmpverts;
    #pragma omp parallel
    {
        // read verts for now
        numthreads=omp_get_num_threads();
        int threadid=omp_get_thread_num();

        #pragma omp single
        {
            tmpverts=new FastDynamic<vec3>[numthreads];
            numtmpverts=new size_t[numthreads];
            for(int i=0;i<numthreads;i++)
            {
                tmpverts[i].SetContainer_size(8192);
                numtmpverts[i]=0;
            }
        }

        #pragma omp single
        {
            // read first line here
        }
        #pragma omp for reduction(+:numverts)
        for (int i = 1; i < numEnds; i++)
        {
            char line[1024]={0};
            memcpy(&line,&memoryfile[lineends[i-1]+1],lineends[i]-lineends[i-1]-1);
            if(line[0]=='v' && line[1]==' ')
            {
                vec3 vert;
                sscanf(line,"v %99f %99f %99f",&vert.x,&vert.y,&vert.z);
                tmpverts[threadid][numtmpverts[threadid]]=vert;
                numtmpverts[threadid]++;
                numverts++;
            }
            if(line[0]=='f' && line[1]==' ')
            {
                char *data=line+2;
                size_t offset=0;
                bool more=1;
                unsigned char type=0;
                unsigned char nexttype=0;
                while(more)
                {
                    bool valid;
                    unsigned int faceidnum=getNextFaceNumber(data,offset,type,nexttype,more,valid);
                    if(valid)
                    {
                    switch(type)
                    {
                        case 0:
                  //          printf("p:%u ",faceidnum);
                            break;
                        case 1:
                //            printf("u:%u ",faceidnum);
                            break;
                        case 2:
              //              printf("n:%u ",faceidnum);
                            break;
                    }
                    }
                }
            //    printf("\n");
            }
        }
        #pragma omp single
        {
            output->verts=new vec3[numverts];
        }
        #pragma omp for
        for(int i=0;i<numthreads;i++)
        {
            int offset=0;
            for(int j=0;j<i;j++)
            {
                offset+=numtmpverts[j];
            }
            tmpverts[i].CopyToStatic(&(output->verts[offset]),numtmpverts[i]);
        }

        #pragma omp single
        {
            delete [] tmpverts;
            delete [] numtmpverts;
        }
    }

    delete [] lineends;
    //printf("lines:%zu\n",linecount);
    //printf("numthreads:%i\n",numthreads);
    //printf("numverts:%zu\n",numverts);

    clock_gettime(CLOCK_REALTIME, &stop );
    calltime=(stop.tv_sec-start.tv_sec)+(stop.tv_nsec-start.tv_nsec)/1000000000.0;
//    printf("done parsing file %lfseconds\n",calltime);

    delete [] memoryfile;

    fclose(f);
    return output;
}
