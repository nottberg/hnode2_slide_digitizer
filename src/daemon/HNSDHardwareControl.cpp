#include <iostream>
#include <thread>

#include "JPEGSerializer.h"
#include "HNSDHardwareControl.h"

HNSDHardwareOperation::HNSDHardwareOperation( std::string id, HNHW_OPTYPE_T type )
{
	m_id = id;
	m_opType = type;
	m_opState = HNHW_OPSTATE_PENDING;

	m_moveDir = HNHW_MDIR_FORWARD;
	m_moveCompletedStepCnt = 0;
	m_moveStepCnt = 0;
}

HNSDHardwareOperation::~HNSDHardwareOperation()
{

}

std::string
HNSDHardwareOperation::getID()
{
	return m_id;
}

HNHW_OPTYPE_T
HNSDHardwareOperation::getType()
{
	return m_opType;
}

void
HNSDHardwareOperation::setState( HNHW_OPSTATE_T newState )
{
	m_opState = newState;
}

HNHW_OPSTATE_T
HNSDHardwareOperation::getState()
{
	return m_opState;
}

CaptureRequest*
HNSDHardwareOperation::getCaptureRequestPtr()
{
	return &m_captureRequest;
}

void
HNSDHardwareOperation::setMoveParameters( HNHW_MDIR_T direction, uint stepCnt )
{
	m_moveDir = direction;
	m_moveStepCnt = stepCnt;
	m_moveCompletedStepCnt = 0;
}

HNHW_MDIR_T 
HNSDHardwareOperation::getMoveDirection()
{
	return m_moveDir;
}

uint
HNSDHardwareOperation::getMoveCyclesRequired()
{
	if( m_moveCompletedStepCnt > m_moveStepCnt )
		return 0;

	return (m_moveStepCnt - m_moveCompletedStepCnt);
}

void
HNSDHardwareOperation::recordMoveCycleComplete()
{
	m_moveCompletedStepCnt += 1;
}

HNSDHardwareControl::HNSDHardwareControl()
{
    m_notifyTrigger = NULL;

    m_opThread = NULL;
	m_activeOp = NULL;

    // m_captureStateMutex;
    m_opState = HNSD_HWSTATE_NOTSET;

}

HNSDHardwareControl::~HNSDHardwareControl()
{

}

void
HNSDHardwareControl::init( HNEPTrigger *notifyTrigger )
{
	m_opState = HNSD_HWSTATE_INIT;

    m_notifyTrigger = notifyTrigger;

	// Start the gpio manager
	m_gpioMgr.start();

	// Start the camera manager
    m_cameraMgr.start();

    m_cameraMgr.initCameraList();

	updateOperationState( HNSD_HWSTATE_IDLE );
}

void
HNSDHardwareControl::getCameraIDList( std::vector< std::string > &idList )
{
	m_cameraMgr.getCameraIDList( idList );
}

bool
HNSDHardwareControl::hasCameraWithID( std::string cameraID )
{
	std::shared_ptr<Camera> camPtr;
	
	camPtr = m_cameraMgr.lookupCameraByID( cameraID );

	return (camPtr == nullptr) ? false : true;
}
        
std::string
HNSDHardwareControl::getCameraLibraryInfoJSONStr( std::string cameraID )
{
	std::shared_ptr<Camera> camPtr;
	
	camPtr = m_cameraMgr.lookupCameraByID( cameraID );

	return (camPtr == nullptr) ? "" : camPtr->getLibraryInfoJSONStr();
}

HNSD_HC_RESULT_T
HNSDHardwareControl::startOperation( HNSDHardwareOperation *op )
{

    if( getOperationState() != HNSD_HWSTATE_IDLE )
	{
		std::cout << "Failed to start operation - id: " << op->getID() << "  m_state: " << getOperationState() << std::endl;
        return HNSD_HC_RESULT_BUSY;
	}

	m_activeOp = op;

	m_activeOp->setState( HNHW_OPSTATE_ACTIVE );

	std::cout << "Starting operation - id: " << m_activeOp->getID() << "  m_state: " << getOperationState() << std::endl;

	updateOperationState( HNSD_HWSTATE_OPERATION_START );

	switch( m_activeOp->getType() )
	{
		case HNHW_OPTYPE_SINGLE_CAPTURE:
		{
			std::cout << "Starting capture thread" << std::endl;
    		m_opThread = new std::thread( HNSDHardwareControl::captureThread, this );
			m_opThread->detach();
		}
		break;

		case HNHW_OPTYPE_MOVE:
		{
			std::cout << "Starting move thread" << std::endl;
    		m_opThread = new std::thread( HNSDHardwareControl::carouselMoveThread, this );
			m_opThread->detach();
		}
		break;
	}

    return HNSD_HC_RESULT_SUCCESS;
}

void
HNSDHardwareControl::cancelOperation()
{
	HNSD_HWSTATE_T opState = getOperationState();
    if( opState == HNSD_HWSTATE_IDLE )
	{
		std::cout << "Failed to cancel operation - no active op" << std::endl;
        return;
	}

	std::cout << "Canceling operation - id: " << m_activeOp->getID() << "  m_state: " << opState << std::endl;
}

void
HNSDHardwareControl::finishOperation()
{
	HNSD_HWSTATE_T opState = getOperationState();
	if( (opState != HNSD_HWSTATE_OPERATION_COMPLETE) && (opState != HNSD_HWSTATE_OPERATION_FAILURE) )
	{
		std::cout << "Failed to finish operation - op is not in ending state - state: " << opState << std::endl;
        return;
	}

	std::cout << "Finishing operation - id: " << m_activeOp->getID() << "  m_state: " << opState << std::endl;

	m_activeOp->setState( HNHW_OPSTATE_COMPLETE );

	m_activeOp = NULL;

	updateOperationState( HNSD_HWSTATE_IDLE );
}

void 
HNSDHardwareControl::captureThread( HNSDHardwareControl *ctrl )
{
	std::cout << "Calling runCapture()" << std::endl;

    ctrl->runCapture();
}

void
HNSDHardwareControl::updateOperationState( HNSD_HWSTATE_T newState )
{
	m_opStateMutex.lock();

	if( m_opState != newState )
	{
		std::cout << "HNSDHardwareControl::updateOperationState - newState: " << newState << std::endl;
		m_opState = newState;

		// Notify upper layer of state change
		if( m_notifyTrigger )
			m_notifyTrigger->trigger();
	}

	m_opStateMutex.unlock();
}

HNSD_HWSTATE_T
HNSDHardwareControl::getOperationState()
{
	HNSD_HWSTATE_T rtnState;

	m_opStateMutex.lock();
	rtnState = m_opState;
	m_opStateMutex.unlock();

	return rtnState;
}

void
HNSDHardwareControl::runCapture()
{
	HNSD_HWSTATE_T curState = HNSD_HWSTATE_NOTSET;
	std::shared_ptr< Camera > camPtr = NULL;

	// Temporary, just use first camera
    std::vector< std::string > idList;
	m_cameraMgr.getCameraIDList( idList );
	camPtr = m_cameraMgr.lookupCameraByID( idList[0] );

    std::cout << "Capture Cam ID: " << camPtr->getID() << std::endl;

	updateOperationState( HNSD_HWSTATE_CAPTURE_START );

    camPtr->acquire( m_activeOp->getCaptureRequestPtr(), this );

	std::cout << "Capture thread - Acquired camera " << camPtr->getID() << std::endl;

    camPtr->configureForCapture();

    std::cout << "Pre Camera Start" << std::endl;

    camPtr->start();

	// Send requests to wait for auto focus to lock up, etc.
	bool scanning = true;
	do
	{
		curState = getOperationState();

		switch( curState )
		{
			// Haven't submited the first request yet,
			// So build and submit that.
			case HNSD_HWSTATE_CAPTURE_START:
			{
				updateOperationState( HNSD_HWSTATE_CAPTURE_WAIT_REQ );

				m_opStateMutex.lock();
                CM_RESULT_T result = camPtr->queueAutofocusRequest();
                m_opStateMutex.unlock();

				if( result != CM_RESULT_SUCCESS )
				{
					updateOperationState( HNSD_HWSTATE_CAPTURE_FOCUS_FAILURE );
                }
			}
			break;

			// The camera is still scanning the autofocus
			// so resubmit the request to continue monitoring
			// for finished autofocus.
			case HNSD_HWSTATE_CAPTURE_FOCUS_WAIT:
			{
			    std::cout << "Queueing polling request" << std::endl;

				updateOperationState( HNSD_HWSTATE_CAPTURE_WAIT_REQ );

				m_opStateMutex.lock();
                CM_RESULT_T result = camPtr->queueRequest();
                m_opStateMutex.unlock();

				if( result != CM_RESULT_SUCCESS )
				{
					updateOperationState( HNSD_HWSTATE_CAPTURE_FOCUS_FAILURE );
                }
			}
			break;

			// Autofocus has completed, so exit the 
			// loop and store this image as the one.
			case HNSD_HWSTATE_CAPTURE_RAW_IMAGE_AQUIRED:
			{
			    std::cout << "Capture Request completed" << std::endl;
				scanning = false;
			}
			break;

			// Autofocus has failed
			case HNSD_HWSTATE_CAPTURE_FOCUS_FAILURE:
			{
			    std::cout << "Capture Request failed" << std::endl;
				scanning = false;
			}
			break;
		}

		// If we are waiting then delay a bit.
		if( curState == HNSD_HWSTATE_CAPTURE_WAIT_REQ )
		{
			std::cout << "===== Waiting =====" << std::endl;
			sleep(2);
		}

	}while( scanning == true );

    camPtr->stop();

	std::cout << "Camera stopped!" << std::endl;

	// If the capture was successful then attempt to
	// save the raw image.
	if( curState == HNSD_HWSTATE_CAPTURE_RAW_IMAGE_AQUIRED )
	{ 
		// Turn the capture into a jpeg file.
    	JPEGSerializer jpgSer;

    	jpgSer.serialize( m_activeOp->getCaptureRequestPtr() );
	}

	// Release the camera
    camPtr->release();

	std::cout << "Capture complete" << std::endl;

	// Temporary
	if( curState == HNSD_HWSTATE_CAPTURE_FOCUS_FAILURE )
		updateOperationState( HNSD_HWSTATE_OPERATION_FAILURE );
	else
		updateOperationState( HNSD_HWSTATE_OPERATION_COMPLETE );

}

void
HNSDHardwareControl::requestEvent( CR_EVTYPE_T event )
{
    std::cout << "HWCtrl::requestEvent - event: " << event << std::endl;

	if( getOperationState() != HNSD_HWSTATE_CAPTURE_WAIT_REQ )
	{
		std::cout << "ERROR: unexpected requestEvent - ignoring" << std::endl;
		return;
	}

    switch( event )
    {
        case CR_EVTYPE_REQ_CANCELED:
		case CR_EVTYPE_REQ_FAILURE:
			updateOperationState( HNSD_HWSTATE_CAPTURE_FOCUS_FAILURE );
        break;

        case CR_EVTYPE_REQ_FOCUSING:
			updateOperationState( HNSD_HWSTATE_CAPTURE_FOCUS_WAIT );
        break;

        case CR_EVTYPE_REQ_COMPLETE:
			updateOperationState( HNSD_HWSTATE_CAPTURE_RAW_IMAGE_AQUIRED );
        break;
    }

	std::cout << "HWCTRL::requestEvent - capState: " << getOperationState() << std::endl;
}

void 
HNSDHardwareControl::carouselMoveThread( HNSDHardwareControl *ctrl )
{
	std::cout << "Calling runCarouselMove()" << std::endl;

    ctrl->runCarouselMove();
}

void
HNSDHardwareControl::runCarouselMove()
{
	HNSD_HWSTATE_T curState = HNSD_HWSTATE_NOTSET;

	// updateOperationState( HNSD_HWSTATE_CAPTURE_START );

//	bool scanning = true;
//	do
//	{
//		curState = getOperationState();
//
//		switch( curState )
//		{
			// Haven't submited the first request yet,
			// So build and submit that.
//			case HNSD_HWSTATE_CAPTURE_START:
//			{
//		}

  	while( m_activeOp->getMoveCyclesRequired() )
  	{
    	switch( m_activeOp->getMoveDirection() )
    	{
			case HNHW_MDIR_FORWARD:
			{
			    // Cycle the FW line for 0.5 sec
      			printf( "Execute forward cycle..." );
      			m_gpioMgr.setForwardCycle();
      			usleep( 500000 );
      			m_gpioMgr.clearForwardCycle();
      			printf( "done\n" );

				m_activeOp->recordMoveCycleComplete();
			}
			break;

			case HNHW_MDIR_BACKWARD:
			{
      			printf( "Execute backward cycle..." );
      			m_gpioMgr.setBackwardCycle();
      			usleep( 500000 );
      			m_gpioMgr.clearBackwardCycle();
      			printf( "done\n" );

				m_activeOp->recordMoveCycleComplete();
			}
			break;
	   	}

		// Wait 1 sec between move operations
		usleep( 1000000 );
	}

	std::cout << "Move complete" << std::endl;

	updateOperationState( HNSD_HWSTATE_OPERATION_COMPLETE );

}

HNSDPSHardwareSingleCapture::HNSDPSHardwareSingleCapture( std::string instance )
: HNSDPipelineStepBase( instance )
{

}

HNSDPSHardwareSingleCapture::~HNSDPSHardwareSingleCapture()
{

}

HNSD_PSTEP_TYPE_T
HNSDPSHardwareSingleCapture::getType()
{
    return HNSD_PSTEP_TYPE_HW_SPLIT_STEP;
}

void 
HNSDPSHardwareSingleCapture::initSupportedParameters( HNSDPipelineClientInterface *capture )
{
    // Get access to the parameters
    HNSDPipeline *pipeline = capture->getPipelinePtr();

    // Add the parameters that apply to this transform
    pipeline->addParameter( getInstance(), "enable", "1", "desc" );
    pipeline->addParameter( getInstance(), "bulk_rotate_degrees", "270", "desc" );
    pipeline->addParameter( getInstance(), "result_image_index", "", "desc" );
}

bool
HNSDPSHardwareSingleCapture::doesStepApply( HNSDPipelineClientInterface *capture )
{
    return true;
}

HNSDP_RESULT_T
HNSDPSHardwareSingleCapture::applyStep( HNSDPipelineClientInterface *capture )
{
    std::cout << "HNSDPSHardwareSingleCapture::applyStep - start" << std::endl;

    std::string outFile = capture->registerNextFilename( "imageCapture" );

    //if( cv::imwrite( outFile, rotImage ) == false )
    //{
    //    printf("Failed to write output file\n");
    //    return HNSDP_RESULT_FAILURE;
    //}

    return HNSDP_RESULT_SUCCESS;
}

HNSDP_RESULT_T
HNSDPSHardwareSingleCapture::completeStep( HNSDPipelineClientInterface *capture )
{
    std::cout << "HNSDPSHardwareSingleCapture::completeStep - start" << std::endl;

    return HNSDP_RESULT_SUCCESS;
}

HNSDP_RESULT_T
HNSDPSHardwareSingleCapture::createHardwareOperation( HNSDPipelineClientInterface *capture, HNSDHardwareOperation **rtnPtr )
{
    std::cout << "HNSDPSHardwareSingleCapture::createHardwareOperation() - Start hardware capture" << std::endl;

    HNSDHardwareOperation *opPtr = new HNSDHardwareOperation( getInstance(), HNHW_OPTYPE_SINGLE_CAPTURE );

    CaptureRequest *crPtr = opPtr->getCaptureRequestPtr();

    crPtr->setImageFormat( CS_STILLMODE_YUV420, 4624, 3472 );

    crPtr->setFileAndPath( capture->registerNextFilename( "capture" ) );

	*rtnPtr = opPtr;

	return HNSDP_RESULT_SUCCESS;
}

HNSDPSHardwareMove::HNSDPSHardwareMove( std::string instance )
: HNSDPipelineStepBase( instance )
{

}

HNSDPSHardwareMove::~HNSDPSHardwareMove()
{

}

HNSD_PSTEP_TYPE_T
HNSDPSHardwareMove::getType()
{
    return HNSD_PSTEP_TYPE_HW_SPLIT_STEP;
}

void 
HNSDPSHardwareMove::initSupportedParameters( HNSDPipelineClientInterface *capture )
{
    // Get access to the parameters
    HNSDPipeline *pipeline = capture->getPipelinePtr();

    // Add the parameters that apply to this transform
    pipeline->addParameter( getInstance(), "enable", "1", "desc" );
    pipeline->addParameter( getInstance(), "bulk_rotate_degrees", "270", "desc" );
    pipeline->addParameter( getInstance(), "result_image_index", "", "desc" );
}

bool
HNSDPSHardwareMove::doesStepApply( HNSDPipelineClientInterface *capture )
{
    return true;
}

HNSDP_RESULT_T
HNSDPSHardwareMove::applyStep( HNSDPipelineClientInterface *capture )
{
    std::cout << "HNSDPSHardwareMove::applyStep - start" << std::endl;

    return HNSDP_RESULT_SUCCESS;
}

HNSDP_RESULT_T
HNSDPSHardwareMove::completeStep( HNSDPipelineClientInterface *capture )
{
    std::cout << "HNSDPSHardwareMove::completeStep - start" << std::endl;

    return HNSDP_RESULT_SUCCESS;
}


HNSDP_RESULT_T
HNSDPSHardwareMove::createHardwareOperation( HNSDPipelineClientInterface *capture, HNSDHardwareOperation **rtnPtr )
{
    std::cout << "HNSDPSHardwareMove::createHardwareOperation() - Start hardware advance" << std::endl;

    HNSDHardwareOperation *opPtr = new HNSDHardwareOperation( getInstance(), HNHW_OPTYPE_MOVE );

    opPtr->setMoveParameters( HNHW_MDIR_FORWARD, 1 );

	*rtnPtr = opPtr;

	return HNSDP_RESULT_SUCCESS;
}
