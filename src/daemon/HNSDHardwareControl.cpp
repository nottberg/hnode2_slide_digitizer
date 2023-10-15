#include <iostream>
#include <thread>

#include "JPEGSerializer.h"
#include "HNSDHardwareControl.h"

HNSDHardwareControl::HNSDHardwareControl()
{
    m_notifyTrigger = NULL;

    m_opThread     = NULL;
	m_activeCapReq = NULL;

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
HNSDHardwareControl::startCapture( CaptureRequest *capReq )
{
	std::cout << "Starting capture - m_state: " << getOperationState() << std::endl;

    if( getOperationState() != HNSD_HWSTATE_IDLE )
        return HNSD_HC_RESULT_BUSY;

	m_activeCapReq = capReq;

	updateOperationState( HNSD_HWSTATE_OPERATION_START );

	std::cout << "Starting capture thread" << std::endl;
    m_opThread = new std::thread( HNSDHardwareControl::captureThread, this );
	m_opThread->detach();

    return HNSD_HC_RESULT_SUCCESS;
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

    camPtr->acquire( m_activeCapReq, this );

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
			std::cout << "Waiting for request complete" << std::endl;
			sleep(1);
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

    	jpgSer.serialize( m_activeCapReq );
	}

	//libcamera::StreamConfiguration const &cfg = configuration->at(0);

    //JPS_RIF_T pFormat = JPS_RIF_NOTSET;
	//if( cfg.pixelFormat == libcamera::formats::YUYV )
    //    pFormat = JPS_RIF_YUYV;
	//else if( cfg.pixelFormat == libcamera::formats::YUV420 )
    //    pFormat = JPS_RIF_YUV420;

    //jpgSer.setRawSource( pFormat, cfg.size.width, cfg.size.height, cfg.stride, bufPtr, buffer_size );

    //jpgSer.serialize();

	// Release the camera
    camPtr->release();

	// Temporary
	updateOperationState( HNSD_HWSTATE_OPERATION_COMPLETE );

	std::cout << "Capture complete" << std::endl;
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

