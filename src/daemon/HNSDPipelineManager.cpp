#include <vector>

#include "HNSDPipeline.h"

#include "HNSDImageTransform.h"
#include "HNSDHardwareControl.h"

#include "HNSDPipelineManager.h"

HNSDPipelineManager::HNSDPipelineManager()
{
    m_storageMgr = NULL;
}

HNSDPipelineManager::~HNSDPipelineManager()
{

}

void
HNSDPipelineManager::setStorageManager( HNSDStorageManager *storageMgr )
{
    m_storageMgr = storageMgr;
}

HNSDStorageManager*
HNSDPipelineManager::getStorageManager()
{
    return m_storageMgr;
}

HNSDP_RESULT_T
HNSDPipelineManager::allocatePipeline( HNSDP_TYPE_T type, std::string ownerID, HNSDPipelineClientInterface *clientInf, HNSDPipeline **rtnPipeline )
{
    HNSDPipeline* newPipe = new HNSDPipeline;

    *rtnPipeline = NULL;

    newPipe->init( type, ownerID, m_storageMgr, clientInf );

    switch( type )
    {
        case HNSDP_TYPE_IMAGE_CAPTURE:
        {
            // Initialize the pipeline with the appropriate steps.
            HNSDPSHardwareSingleCapture* hwCapture = new HNSDPSHardwareSingleCapture( "imageCapture", ownerID );
            newPipe->addStep( hwCapture );

            HNSDPSHardwareMove* hwMove = new HNSDPSHardwareMove( "slideAdvance", ownerID );
            newPipe->addStep( hwMove );

            HNSDPSOrthogonalRotate* orthoRotate = new HNSDPSOrthogonalRotate( "orthoRotate", ownerID );
            newPipe->addStep( orthoRotate );

            HNSDPSCrop* crop = new HNSDPSCrop( "cropRaw", ownerID );
            newPipe->addStep( crop );
        }
        break;
    }

    // Have the steps fill in their supported parameters
    newPipe->initializeParameters();

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
