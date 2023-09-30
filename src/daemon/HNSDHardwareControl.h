#ifndef __HNSD_HARDWARE_CONTROL_H__
#define __HNSD_HARDWARE_CONTROL_H__

#include <stdint.h>

#include <string>
#include <mutex>
#include <thread>
#include <sstream>

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

class HNSDHardwareControl 
{
    public:
        HNSDHardwareControl();
       ~HNSDHardwareControl();

        void setNotificationFD( uint notificationFD );

        HNSD_HWSTATE_T getState();

        uint getNotificationFD();

        HNSD_HC_RESULT_T startCapture();

    private:
        uint m_notifyFD;

        HNSD_HWSTATE_T  m_state;
};

#endif // __HNSD_HARDWARE_CONTROL_H__
