#ifndef _FASTDYNAMIC_H
#define _FASTDYNAMIC_H
#include <memory.h>
#define DEFAULT_CONTAINER_SIZE 4 // use larger numbers when using small objects or you will run out of memory quickly

template <class T>
class FastDynamic //array that expands when it runs out of bounds
{
    struct bucket
    {
        T *contents;
    };
    bucket **buckets;
public:
    size_t increaseSize; // in bytes so numnew*contentssize in the SetContainer_size
    size_t currentLength;
    size_t contentSize;
    FastDynamic(size_t increaseSize)
    {
    }
    FastDynamic(const FastDynamic<T> &old)
    {
    }
    FastDynamic()
    {
    }
    void SetContainer_size(size_t newsize)
    {
    }
    ~FastDynamic()
    {
    }
    void Clear()
    {
    T &operator[](size_t index)
    {
    }
    T *GetBucket(int bucketnum)
    {
    }
    void CopyToStatic(T *staticarray, size_t count)
    {

    }
};
#endif
