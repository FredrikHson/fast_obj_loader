#include "fast_obj_loader.h"
#include <time.h>
#include <stdio.h>
#include "fastdynamic2.h"


int main(int argc, char* argv[])
{
    int timestoload = 1;

    if(argc == 1)
    {
        printf("usage testobjloader filename.obj 5\n");
        return 0;
    }

    if(argc == 3)
    {
        sscanf(argv[2], "%20i", &timestoload);
    }

    printf("starting to load %s %i times\n", argv[1], timestoload);
    timespec start, stop;
    clock_gettime(CLOCK_REALTIME, &start);
    double calltime;

    for(int i = 0; i < timestoload; i++)
    {
        obj* testmesh = loadObj(argv[1]);

        delete testmesh;
    }

    clock_gettime(CLOCK_REALTIME, &stop);
    calltime = (stop.tv_sec - start.tv_sec) + (stop.tv_nsec - start.tv_nsec) / 1000000000.0;
    printf("done loading file %lfseconds\n", calltime / timestoload);

    return 0;
}
