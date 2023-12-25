#ifndef __HNSD_PIPELINE_MANAGER_H__
#define __HNSD_PIPELINE_MANAGER_H__

#include <any>
#include <map>
#include <mutex>
#include <string>
#include <memory>

#include <hnode2/HNReqWaitQueue.h>

class HNSDPipelineManager
{
    public:
        HNSDPipelineManager();
       ~HNSDPipelineManager();

        HNSDPipeline *allocatePipeline( HNSDP_TYPE_T type );

        void releasePipeline( HNSDPipeline *pipeline );

        HNSDP_RESULT_T allocatePipeline( HNSDP_TYPE_T type, HNSDPipeline **rtnPipeline );
        void releasePipeline( HNSDPipeline **pipeline );

};

#endif //__HNSD_PIPELINE_MANAGER_H__
