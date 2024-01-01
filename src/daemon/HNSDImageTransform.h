#ifndef __HNSD_IMAGE_TRANSFORM_H__
#define __HNSD_IMAGE_TRANSFORM_H__

#include <string>

#include "HNSDPipeline.h"

class HNSDPSOrthogonalRotate : public HNSDPipelineStepBase
{
    public:
        HNSDPSOrthogonalRotate( std::string instance, std::string ownerID );
       ~HNSDPSOrthogonalRotate();

    private:

        virtual HNSD_PSTEP_TYPE_T getType();

        virtual void initSupportedParameters( HNSDPipelineParameterMap *paramMap );

        virtual bool doesStepApply( HNSDPipelineParameterMap *paramMap );

        virtual HNSDP_RESULT_T executeInline( HNSDPipelineParameterMap *paramMap, HNSDStorageManager *storageMgr );

        virtual HNSDP_RESULT_T completeStep( HNSDPipelineParameterMap *paramMap, HNSDStorageManager *storageMgr );
};

class HNSDPSCrop : public HNSDPipelineStepBase
{
    public:
        HNSDPSCrop( std::string instance, std::string ownerID );
       ~HNSDPSCrop();

    private:

        virtual HNSD_PSTEP_TYPE_T getType();

        virtual void initSupportedParameters( HNSDPipelineParameterMap *paramMap );

        virtual bool doesStepApply( HNSDPipelineParameterMap *paramMap );

        virtual HNSDP_RESULT_T executeInline( HNSDPipelineParameterMap *paramMap, HNSDStorageManager *storageMgr );

        virtual HNSDP_RESULT_T completeStep( HNSDPipelineParameterMap *paramMap, HNSDStorageManager *storageMgr );
};

#endif //__HNSD_IMAGE_TRANSFORM_H__
