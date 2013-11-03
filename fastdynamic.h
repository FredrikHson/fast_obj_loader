/*************************************************************************
 *
 * This file is part of the Tesla game engine.
 *
 * Tesla game engine is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 3
 * only, as published by the Free Software Foundation.
 *
 * Tesla game engine is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License version 3 for more details
 * (a copy is included in the LICENSE file that accompanied this code).
 *
 * You should have received a copy of the GNU Lesser General Public License
 * version 3 along with Tesla game engine.  If not, see
 * <http://www.gnu.org/licenses/>
 * for a copy of the LGPLv3 License.
 *
 ************************************************************************/

#ifndef _FASTDYNAMIC_H
#define _FASTDYNAMIC_H

#include "memtracker.h"
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
