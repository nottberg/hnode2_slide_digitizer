#ifndef __HNSD_IMAGE_MANAGER_H__
#define __HNSD_IMAGE_MANAGER_H__

#include <any>
#include <map>
#include <mutex>
#include <string>
#include <memory>

#include <hnode2/HNReqWaitQueue.h>

typedef enum HNSDImageManagerResultEnum
{
    IMM_RESULT_SUCCESS,
    IMM_RESULT_FAILURE
} IMM_RESULT_T;

class HNSDImageManager
{
    public:
         HNSDImageManager();
        ~HNSDImageManager();

        IMM_RESULT_T start();
        IMM_RESULT_T stop();

    private:

};

#endif //__HNSD_IMAGE_MANAGER_H__
