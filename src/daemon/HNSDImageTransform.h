#ifndef __HNSD_IMAGE_TRANSFORM_H__
#define __HNSD_IMAGE_TRANSFORM_H__

#include <string>

#include "HNSDPipeline.h"

class HNSDPSOrthogonalRotate : public HNSDPipelineStepBase
{
    public:
        HNSDPSOrthogonalRotate( std::string instance );
       ~HNSDPSOrthogonalRotate();

    private:

        virtual HNSD_PSTEP_TYPE_T getType();

        virtual void initSupportedParameters( HNSDPipelineClientInterface *capture );

        virtual bool doesStepApply( HNSDPipelineClientInterface *capture );

        virtual HNSDP_RESULT_T executeInline( HNSDPipelineClientInterface *capture );

        virtual HNSDP_RESULT_T completeStep( HNSDPipelineClientInterface *capture );
};

class HNSDPSCrop : public HNSDPipelineStepBase
{
    public:
        HNSDPSCrop( std::string instance );
       ~HNSDPSCrop();

    private:

        virtual HNSD_PSTEP_TYPE_T getType();

        virtual void initSupportedParameters( HNSDPipelineClientInterface *capture );

        virtual bool doesStepApply( HNSDPipelineClientInterface *capture );

        virtual HNSDP_RESULT_T executeInline( HNSDPipelineClientInterface *capture );

        virtual HNSDP_RESULT_T completeStep( HNSDPipelineClientInterface *capture );
};

#endif //__HNSD_IMAGE_TRANSFORM_H__
