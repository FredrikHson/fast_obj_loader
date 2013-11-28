#include "fast_obj_loader.h"
#include <time.h>
#include <stdio.h>
#include "fastdynamic2.h"
int main(int argc, char *argv[])
{
    int timestoload = 1;

    if(argc == 2)
    sscanf(argv[1], "%20i", &timestoload);

    printf("starting to load file %i times\n", timestoload);
    timespec start, stop;
    clock_gettime(CLOCK_REALTIME, &start);
    double calltime;

    for(int i = 0; i < timestoload; i++)
    {
    //obj *testmesh=loadObj("../test.obj");
    obj *testmesh = loadObj("../dragon_vrip_res2.obj");

    //obj *testmesh=loadObj("../xyzrgb_dragon.obj");
    //if(testmesh)
    //writeObj("verification.obj", *testmesh);

    delete testmesh;
    }

    clock_gettime(CLOCK_REALTIME, &stop);
    calltime = (stop.tv_sec - start.tv_sec) + (stop.tv_nsec - start.tv_nsec) / 1000000000.0;
    printf("done loading file %lfseconds\n", calltime / timestoload);

    //FastDynamic<int> test;
    //test.SetContainer_size(1024);

    //for(int i = 0; i < 3200000; i++)
        //test[i] = i;

    //for(int i = 0; i < 3200000; i++)
        //if(test[i] != i)
            //printf("wtf");

    //printf("endtest\n");

    //int *staticarray = new int[3200000];
    //printf("end static test\n");
    //test.CopyToStatic(staticarray,3200000);
    //for(int i = 0; i < 3200000; i++)
        //if(staticarray[i] != i)
            //printf("wtf");
    //delete [] staticarray;
    return 0;
}
