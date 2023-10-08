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

typedef enum HNSDHardwareStateEnum
{
    HNSD_HWSTATE_NOTSET,
    HNSD_HWSTATE_INIT,
    HNSD_HWSTATE_IDLE,
    HNSD_HWSTATE_CAPTURING,
    HNSD_HWSTATE_IMGPROC,
    HNSD_HWSTATE_CAROSEL_CAPTURING,
    HNSD_HWSTATE_CAROSEL_IMGPROC,
    HNSD_HWSTATE_CAROSEL_ADVANCING
}HNSD_HWSTATE_T;

typedef enum HNSDCaptureThreadStateEnum
{
    HNSDCT_STATE_IDLE,
    HNSDCT_STATE_WAIT_COMPLETE,
    HNSDCT_STATE_FOCUS,
    HNSDCT_STATE_CAPTURED
}HNSDCT_STATE_T;

class HNSDHardwareControl 
{
    public:
        HNSDHardwareControl();
       ~HNSDHardwareControl();

        void init( HNEPTrigger *notifyTrigger );

        HNSD_HWSTATE_T getState();

        HNSD_HC_RESULT_T startCapture();

    private:
        HNEPTrigger *m_notifyTrigger;

        HNSD_HWSTATE_T  m_state;

        std::thread *m_opThread;

        Camera *m_curCamera;

        std::mutex  m_captureStateMutex;
        HNSDCT_STATE_T m_captureState;

        static void captureThread( HNSDHardwareControl *ctrl );
        void runCapture();

        //void requestComplete( libcamera::Request *request );
};

#endif // __HNSD_HARDWARE_CONTROL_H__
