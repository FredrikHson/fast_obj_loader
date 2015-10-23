#ifndef _FASTDYNAMIC_H
#define _FASTDYNAMIC_H
#include <memory.h>
#include <stdlib.h>
#define DEFAULT_CONTAINER_SIZE 4 // use larger numbers when using small objects or you will run out of memory quickly
// should be 100% compatible with the old fastdynamic one other than setting the size
// will not delete anything just change the increaseSize variable

template <class T>
class FastDynamic //array that expands when it runs out of bounds
{
    T* contents;
public:
    size_t increaseSize; // in bytes so numnew*contentssize in the SetContainer_size
    size_t currentLength; // not how many entries have been written just how many have been allocated
    size_t currentByteLength;
    explicit FastDynamic(size_t increaseSize)
    {
        this->increaseSize = increaseSize;
        contents = (T*)malloc(this->increaseSize * sizeof(T));
        currentLength = increaseSize;
        currentByteLength = this->increaseSize * sizeof(T);

    }
    explicit FastDynamic(const FastDynamic<T>& old) // this will differ as much as that it will copy while the old one didn't
    {
        this->increaseSize = old.increaseSize;
        contents = (T*)malloc(old.currentByteLength);
        currentLength = old.currentLength;
        currentByteLength = old.currentByteLength;
        old.CopyToStatic(contents, currentLength);
    }
    FastDynamic()
    {
        increaseSize = DEFAULT_CONTAINER_SIZE;
        contents = (T*)malloc(increaseSize * sizeof(T));
        currentLength = DEFAULT_CONTAINER_SIZE;
        currentByteLength = increaseSize * sizeof(T);
    }
    void SetContainer_size(size_t newsize) // size is in objects
    {
        increaseSize = newsize;
    }
    ~FastDynamic()
    {
        if(contents)
        {
            free(contents);
        }
    }
    void Clear()
    {
        if(contents)
        {
            free(contents);
        }

        contents = (T*)malloc(increaseSize * sizeof(T));
        currentLength = increaseSize;
        currentByteLength = increaseSize * sizeof(T);
    }
    T& operator[](size_t index)
    {
        if(index >= currentLength)
        {
            size_t newsize = (index / increaseSize) + 1;
            newsize *= increaseSize;
            //printf("currentLength:%u  newsize:%u index:%u increaseSize:%i\n", currentLength, newsize, index, increaseSize);
            T* tmp = (T*)realloc(contents, newsize * sizeof(T));
            contents = tmp;
            currentLength = newsize;
            currentByteLength = newsize * sizeof(T);
        }

        return contents[index];
    }
    T* GetBucket(int bucketnum) // tbh worthless in this implementation
    {
        return contents[bucketnum / increaseSize / sizeof(T)];
    }
    void CopyToStatic(T* staticarray, size_t count)
    {
        if(count == 0)
        {
            return;
        }

        if(count > currentLength)
        {
            count = currentLength;
        }

        memcpy(staticarray, contents, sizeof(T)*count);
    }
};
#endif
