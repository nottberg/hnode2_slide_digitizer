#include <iostream>
#include <thread>

#include "HNSDHardwareControl.h"

HNSDHardwareControl::HNSDHardwareControl()
{
    m_notifyTrigger = NULL;

    m_state = HNSD_HWSTATE_NOTSET;
}

HNSDHardwareControl::~HNSDHardwareControl()
{

}

void
HNSDHardwareControl::init( HNEPTrigger *notifyTrigger )
{
    m_notifyTrigger = notifyTrigger;
}


HNSD_HWSTATE_T 
HNSDHardwareControl::getState()
{
    return m_state;
}

HNSD_HC_RESULT_T
HNSDHardwareControl::startCapture()
{
    if( m_state != HNSD_HWSTATE_IDLE )
        return HNSD_HC_RESULT_BUSY;

    m_opThread = new std::thread( HNSDHardwareControl::captureThread, this );

    return HNSD_HC_RESULT_SUCCESS;
}

void 
HNSDHardwareControl::captureThread( HNSDHardwareControl *ctrl )
{
    ctrl->runCapture();
}

void
HNSDHardwareControl::runCapture()
{
    CaptureRequest request;

    std::cout << "Capture Cam ID: " << m_curCamera->getID() << std::endl;

    m_curCamera->acquire( &request, this );

	std::cout << "Capture thread - Acquired camera " << m_curCamera->getID() << std::endl;

    m_curCamera->configureForCapture();

    std::cout << "Pre Camera Start" << std::endl;

    m_curCamera->start();

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
                if( m_curCamera->queueAutofocusRequest() != CM_RESULT_SUCCESS )
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

                if( m_curCamera->queueRequest() != CM_RESULT_SUCCESS )
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

    m_curCamera->stop();

	std::cout << "Camera stopped!" << std::endl;

	// Turn the capture into a jpeg file.
    //JPEGSerializer jpgSer;

	//libcamera::StreamConfiguration const &cfg = configuration->at(0);

    //JPS_RIF_T pFormat = JPS_RIF_NOTSET;
	//if( cfg.pixelFormat == libcamera::formats::YUYV )
    //    pFormat = JPS_RIF_YUYV;
	//else if( cfg.pixelFormat == libcamera::formats::YUV420 )
    //    pFormat = JPS_RIF_YUV420;

    //jpgSer.setRawSource( pFormat, cfg.size.width, cfg.size.height, cfg.stride, bufPtr, buffer_size );

    //jpgSer.serialize();

	// Release the camera
    m_curCamera->release();

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

