#include <vector>

#include "HNSDPipeline.h"

#include "HNSDImageTransform.h"
#include "HNSDHardwareControl.h"

#include "HNSDPipelineManager.h"

HNSDPipelineManager::HNSDPipelineManager()
{

}

HNSDPipelineManager::~HNSDPipelineManager()
{

}

HNSDP_RESULT_T
HNSDPipelineManager::allocatePipeline( HNSDP_TYPE_T type, HNSDPipelineClientInterface *clientInf, HNSDPipeline **rtnPipeline )
{
    HNSDPipeline* newPipe = new HNSDPipeline;

    *rtnPipeline = NULL;

    newPipe->init( type, clientInf );

    switch( type )
    {
        case HNSDP_TYPE_IMAGE_CAPTURE:
        {
            // Initialize the pipeline with the appropriate steps.
            HNSDPSHardwareSingleCapture* hwCapture = new HNSDPSHardwareSingleCapture( "imageCapture" );
            newPipe->addStep( hwCapture );

            HNSDPSHardwareMove* hwMove = new HNSDPSHardwareMove( "slideAdvance" );
            newPipe->addStep( hwMove );

            HNSDPSOrthogonalRotate* orthoRotate = new HNSDPSOrthogonalRotate( "orthoRotate" );
            newPipe->addStep( orthoRotate );

            HNSDPSCrop* crop = new HNSDPSCrop( "cropRaw" );
            newPipe->addStep( crop );
        }
        break;
    }

    *rtnPipeline = newPipe;

    return HNSDP_RESULT_SUCCESS;
}

HNSDP_RESULT_T 
HNSDPipelineManager::submitPipelineForExecution( HNSDPipeline *pipeline )
{
    m_pendingQueue.push_back( pipeline );
    pipeline->waitingForExecution();
    return HNSDP_RESULT_SUCCESS;
}

HNSDPipeline* 
HNSDPipelineManager::getNextPendingPipeline()
{
    HNSDPipeline *rtnPipeline;

    if( m_pendingQueue.empty() == true )
        return NULL;

    rtnPipeline = m_pendingQueue.front();
    m_pendingQueue.pop_front();

    return rtnPipeline;
}

void
HNSDPipelineManager::releasePipeline( HNSDPipeline **pipeline )
{
    delete *pipeline;
    *pipeline = NULL;
}
