#include <sys/mman.h>
#include <sys/time.h>

#include <iostream>
#include <sstream>

#include <Poco/JSON/Object.h>
#include <Poco/JSON/Parser.h>
#include <Poco/StreamCopier.h>

#include "HNSDPipeline.h"

#include "HNSDImageTransform.h"
#include "HNSDHardwareControl.h"

#include "HNSDPipelineManager.h"

namespace pjs = Poco::JSON;

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

void
HNSDPipelineManager::releasePipeline( HNSDPipeline **pipeline )
{
    delete *pipeline;
    *pipeline = NULL;
}
