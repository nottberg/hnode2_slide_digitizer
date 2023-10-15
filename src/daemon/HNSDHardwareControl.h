#ifndef __HNSD_HARDWARE_CONTROL_H__
#define __HNSD_HARDWARE_CONTROL_H__

#include <stdint.h>

#include <string>
#include <mutex>
#include <thread>
#include <sstream>

#include <hnode2/HNEPLoop.h>

#include "CameraManager.h"

typedef enum HNSDHardwareControlResult
{
    HNSD_HC_RESULT_SUCCESS,
    HNSD_HC_RESULT_FAILURE,
    HNSD_HC_RESULT_BUSY
}HNSD_HC_RESULT_T;

typedef enum HNSDHardwareOperationStateEnum
{
    HNSD_HWSTATE_NOTSET,
    HNSD_HWSTATE_INIT,
    HNSD_HWSTATE_IDLE,
    HNSD_HWSTATE_CAPTURE_START,
    HNSD_HWSTATE_CAPTURE_FOCUS_WAIT,
    HNSD_HWSTATE_CAPTURE_FOCUS_FAILURE,
    HNSD_HWSTATE_CAPTURE_POLL_REQUEST,
    HNSD_HWSTATE_CAPTURE_WAIT_REQ,
    HNSD_HWSTATE_CAPTURE_RAW_IMAGE_AQUIRED,
    HNSD_HWSTATE_CAPTURE_POST_PROCESS,
    HNSD_HWSTATE_CAPTURE_COMPLETE,
    HNSD_HWSTATE_CAROSEL_CAPTURING,
    HNSD_HWSTATE_CAROSEL_IMGPROC,
    HNSD_HWSTATE_CAROSEL_ADVANCING,
    HNSD_HWSTATE_OPERATION_START,
    HNSD_HWSTATE_OPERATION_COMPLETE
}HNSD_HWSTATE_T;

typedef enum HNSDCaptureThreadStateEnum
{
    HNSDCT_STATE_IDLE,
    HNSDCT_STATE_WAIT_COMPLETE,
    HNSDCT_STATE_FOCUS,
    HNSDCT_STATE_CAPTURED,
    HNSDCT_STATE_FAILURE
}HNSDCT_STATE_T;

class HNSDHardwareControl : public CameraEventInf
{
    public:
        HNSDHardwareControl();
       ~HNSDHardwareControl();

        void init( HNEPTrigger *notifyTrigger );

        HNSD_HWSTATE_T getOperationState();

        void getCameraIDList( std::vector< std::string > &idList );

        bool hasCameraWithID( std::string cameraID );
        
        std::string getCameraLibraryInfoJSONStr( std::string cameraID );

        HNSD_HWSTATE_T getState();

        HNSD_HC_RESULT_T startCapture( CaptureRequest *capReq );

        virtual void requestEvent( CR_EVTYPE_T event );

    private:
        CameraManager m_cameraMgr;

        HNEPTrigger *m_notifyTrigger;

        std::thread *m_opThread;

        CaptureRequest *m_activeCapReq;

        std::mutex  m_opStateMutex;
        HNSD_HWSTATE_T m_opState;

        void updateOperationState( HNSD_HWSTATE_T newState );

        static void captureThread( HNSDHardwareControl *ctrl );
        void runCapture();

        //void requestComplete( libcamera::Request *request );
};

#endif // __HNSD_HARDWARE_CONTROL_H__
