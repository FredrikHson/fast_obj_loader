#include "fast_obj_loader.h"
#include <stdio.h>
#include <stdlib.h>
#include "fastdynamic.h"
#ifdef _OPENMP
    #include <omp.h>
#endif
#include <time.h>
//bool getcstr(char *out,unsigned short bufflen,char *input,size_t &len) // bufflen = output buffer size // input has to be a copy of the original pointer
//{
//    if(len<=0)
//        return false;


//    unsigned short i=0;
//    while
//}

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
    printf("filesize=%d bytes\n",filelength);
    char *memoryfile=new char[filelength];

    fread(memoryfile,filelength,1,f);
    timespec start,stop;
    clock_gettime(CLOCK_REALTIME, &start );
    double calltime;


    size_t linecount=0;
    int numthreads=0;
    FastDynamic<size_t> *tmpends;
    size_t *numtmpends;
    size_t *lineends;
    int numEnds=0;
    lineends.SetContainer_size(8192);
    #pragma omp parallel
    {
        numthreads=omp_get_num_threads();
        int threadid=omp_get_thread_num();

        #pragma omp single
        {
            tmpends=new FastDynamic<int>[numthreads];
            for(int i=0;i<numthreads;i++)
                tmpends[i].SetContainer_size(8192);
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
    }
    printf("lines:%i\n",linecount);
    printf("numthreads:%i\n",numthreads);

    clock_gettime(CLOCK_REALTIME, &stop );
    calltime=(stop.tv_sec-start.tv_sec)+(stop.tv_nsec-start.tv_nsec)/1000000000.0;
    printf("done parsing file %lfseconds\n",calltime);

    delete [] memoryfile;

    fclose(f);
    return output;
}
