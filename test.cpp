#include <stdio.h>
#include <stdlib.h>
#include "fast_obj_loader.h"
#include <time.h>

int main(int argc, char *argv[])
{
    printf("starting to load file\n");
    timespec start,stop;
    clock_gettime(CLOCK_REALTIME, &start );
    double calltime;

    obj *testmesh=loadObj("../dragon_vrip_res2.obj");
    //obj *testmesh=loadObj("../xyzrgb_dragon.obj");

    clock_gettime(CLOCK_REALTIME, &stop );
    calltime=(stop.tv_sec-start.tv_sec)+(stop.tv_nsec-start.tv_nsec)/1000000000.0;
    printf("done loading file %lfseconds\n",calltime);
    delete testmesh;
    return 0;
}
