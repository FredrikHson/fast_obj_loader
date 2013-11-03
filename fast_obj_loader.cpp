#include "fast_obj_loader.h"
#include <stdio.h>
#include <stdlib.h>
#include "fastdynamic.h"
#ifdef _OPENMP
#include <omp.h>
#endif


obj *loadObj(char *filename)
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
    size_t linecount=0;
    unsigned int numverts=0;
#pragma omp parallel for reduction(+:linecount,numverts)
    for(size_t i=0;i<filelength;i++)
    {
        if(memoryfile[i]=='\n')
            linecount++;
        else if(memoryfile[i]=='v')
            numverts++;
    }
    printf("lines:%i\n",linecount);
    printf("numverts:%i\n",numverts);
    delete [] memoryfile;

    fclose(f);
    return output;
}
