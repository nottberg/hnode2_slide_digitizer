#include <iostream>
#include <thread>

#include "JPEGSerializer.h"
#include "HNSDHardwareControl.h"

HNSDHardwareControl::HNSDHardwareControl()
{
    m_notifyTrigger = NULL;

    m_state = HNSD_HWSTATE_NOTSET;

    m_opThread     = NULL;
	m_activeCapReq = NULL;

    // m_captureStateMutex;
    m_captureState = HNSDCT_STATE_IDLE;

}

HNSDHardwareControl::~HNSDHardwareControl()
{

}

void
HNSDHardwareControl::init( HNEPTrigger *notifyTrigger )
{
    m_notifyTrigger = notifyTrigger;

	// Start the camera manager
    m_cameraMgr.start();

    m_cameraMgr.initCameraList();

	m_state = HNSDCT_STATE_IDLE;
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

HNSD_HWSTATE_T 
HNSDHardwareControl::getState()
{
    return m_state;
}

HNSD_HC_RESULT_T
HNSDHardwareControl::startCapture( CaptureRequest *capReq )
{
	std::cout << "Starting capture - m_state: " << m_state << std::endl;

    if( m_state != HNSD_HWSTATE_IDLE )
        return HNSD_HC_RESULT_BUSY;

	m_activeCapReq = capReq;

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
HNSDHardwareControl::runCapture()
{
	std::shared_ptr< Camera > camPtr = NULL;

	// Temporary, just use first camera
    std::vector< std::string > idList;
	m_cameraMgr.getCameraIDList( idList );
	camPtr = m_cameraMgr.lookupCameraByID( idList[0] );

    std::cout << "Capture Cam ID: " << camPtr->getID() << std::endl;

    camPtr->acquire( m_activeCapReq, this );

	std::cout << "Capture thread - Acquired camera " << camPtr->getID() << std::endl;

    camPtr->configureForCapture();

    std::cout << "Pre Camera Start" << std::endl;

    camPtr->start();

	// Send requests to wait for auto focus to lock up, etc.
	bool scanning = true;
	bool delay = false;
	do
	{
		if( m_captureStateMutex.try_lock() == false )
		{
			std::cout << "Polling capture state - retry" << std::endl;
			sleep( 1 );
			continue;
		}

		switch( m_captureState )
		{
			// Check if we are just waiting for the completion to occur
			case HNSDCT_STATE_WAIT_COMPLETE:
			{
				std::cout << "Waiting for request complete" << std::endl;
				delay = true;
			}
			break;

			// Haven't submited the first request yet,
			// So build and submit that.
			case HNSDCT_STATE_IDLE:
			{
                if( camPtr->queueAutofocusRequest() != CM_RESULT_SUCCESS )
                {
                    m_captureStateMutex.unlock();
                    return;
                }

				m_captureState = HNSDCT_STATE_WAIT_COMPLETE;
			}
			break;

			// The camera is still scanning the autofocus
			// so resubmit the request to continue monitoring
			// for finished autofocus.
			case HNSDCT_STATE_FOCUS:
			{
			    std::cout << "Queueing polling request" << std::endl;

                if( camPtr->queueRequest() != CM_RESULT_SUCCESS )
                {
                    m_captureStateMutex.unlock();
                    return;
                }

				m_captureState = HNSDCT_STATE_WAIT_COMPLETE;
			}
			break;

			// Autofocus has completed, so exit the 
			// loop and store this image as the one.
			case HNSDCT_STATE_CAPTURED:
			{
			    std::cout << "Capture Request completed" << std::endl;
				scanning = false;
			}
			break;
		}

		// Release the mutex, since we are done modifying capture state
		m_captureStateMutex.unlock();

		// If we are waiting then delay a bit.
		if( delay == true )
		{
			sleep(1);
			delay = false;
		}

	}while( scanning == true );

    camPtr->stop();

	std::cout << "Camera stopped!" << std::endl;

	// Turn the capture into a jpeg file.
    JPEGSerializer jpgSer;

    jpgSer.serialize( m_activeCapReq );

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

	std::cout << "Capture complete" << std::endl;
}

void
HNSDHardwareControl::requestEvent( CR_EVTYPE_T event )
{
    std::cout << "HWCtrl::requestEvent - event: " << event << std::endl;

	// Aquire the lock.
	m_captureStateMutex.lock();

    switch( event )
    {
        case CR_EVTYPE_REQ_CANCELED:
        break;

        case CR_EVTYPE_REQ_FOCUSING:
    		m_captureState = HNSDCT_STATE_FOCUS;
        break;

        case CR_EVTYPE_REQ_COMPLETE:
    		m_captureState = HNSDCT_STATE_CAPTURED;
        break;
    }

	std::cout << "HWCTRL::requestEvent - capState: " << m_captureState << std::endl;

	m_captureStateMutex.unlock();
}

