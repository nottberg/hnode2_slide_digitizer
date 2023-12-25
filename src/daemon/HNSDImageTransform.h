#ifndef __HNSD_IMAGE_TRANSFORM_H__
#define __HNSD_IMAGE_TRANSFORM_H__

#include <any>
#include <map>
#include <mutex>
#include <string>
#include <memory>

#include "HNSDPipeline.h"

class HNSDPSOrthogonalRotate : public HNSDPipelineStepBase
{
    public:
        HNSDPSOrthogonalRotate( std::string instance );
       ~HNSDPSOrthogonalRotate();

    private:

        virtual HNSD_PSTEP_TYPE_T getType();

        virtual void initSupportedParameters( HNSDPipelineManagerInterface *capture );

        virtual bool doesStepApply( HNSDPipelineManagerInterface *capture );

        virtual HNSDP_RESULT_T applyStep( HNSDPipelineManagerInterface *capture );

        virtual HNSDP_RESULT_T completeStep( HNSDPipelineManagerInterface *capture );
};

class HNSDPSCrop : public HNSDPipelineStepBase
{
    public:
        HNSDPSCrop( std::string instance );
       ~HNSDPSCrop();

    private:

        virtual HNSD_PSTEP_TYPE_T getType();

        virtual void initSupportedParameters( HNSDPipelineManagerInterface *capture );

        virtual bool doesStepApply( HNSDPipelineManagerInterface *capture );

        virtual HNSDP_RESULT_T applyStep( HNSDPipelineManagerInterface *capture );

        virtual HNSDP_RESULT_T completeStep( HNSDPipelineManagerInterface *capture );
};

#endif //__HNSD_IMAGE_TRANSFORM_H__
