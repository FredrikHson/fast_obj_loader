#ifndef _FASTDYNAMIC_H
#define _FASTDYNAMIC_H

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
    size_t bucket_size;
    size_t bucket_count;
    FastDynamic(size_t bucket_size)
    {
        this->bucket_size=bucket_size;
        bucket_count=1;
        buckets=new bucket*[1];
        buckets[0]=new bucket;
        (*buckets[0]).contents=new T[bucket_size];
    }
    FastDynamic()
    {
        this->bucket_size=DEFAULT_CONTAINER_SIZE;
        bucket_count=1;
        buckets=new bucket*[1];
        buckets[0]=new bucket;
        (*buckets[0]).contents=new T[bucket_size];
    }
    void SetContainer_size(size_t newsize) // will delete all contents use constructor instead if possible
    {
        for(size_t i=0;i<bucket_count;i++)
        {
            delete [] (*buckets[i]).contents;
            delete buckets[i];
        }
        delete [] buckets;

        this->bucket_size=newsize;
        bucket_count=1;
        buckets=new bucket*[1];
        buckets[0]=new bucket;
        (*buckets[0]).contents=new T[bucket_size];
    }
    ~FastDynamic()
    {
        for(size_t i=0;i<bucket_count;i++)
        {
            delete [] (*buckets[i]).contents;
            delete buckets[i];
        }
        delete [] buckets;
    }
    void Clear()
    {
        SetContainer_size(bucket_size);
    }
    T &operator[](size_t index)
    {
        size_t bucket_index=index/bucket_size;
        size_t entry=index%bucket_size;
        if(bucket_index+1>bucket_count) // resize array
        {
            bucket **tmparray=buckets;
            buckets=new bucket*[bucket_index+1];
            for(size_t i=0;i<bucket_count;i++)
            {
                buckets[i]=tmparray[i];
            }
            delete [] tmparray;
            for(size_t i=bucket_count;i<bucket_index+1;i++)
            {
                buckets[i]=new bucket;
                (*buckets[i]).contents=new T[bucket_size];
            }
            bucket_count=bucket_index+1;
        }
        return (*buckets[bucket_index]).contents[entry];
    }
    T *GetBucket(int bucketnum)
    {
        return (*buckets[bucketnum]).contents;
    }
};
#endif
