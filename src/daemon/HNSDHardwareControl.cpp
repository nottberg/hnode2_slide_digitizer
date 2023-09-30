
#include "HNSDHardwareControl.h"

HNSDHardwareControl::HNSDHardwareControl( uint notificationFD )
{
    m_notifyFD = notificationFD;

    m_state = HNSD_HWSTATE_NOTSET;
}

HNSDHardwareControl::~HNSDHardwareControl()
{

}

HNSD_HWSTATE_T 
HNSDHardwareControl::getState()
{
    return m_state;
}

HNSD_HC_RESULT_T
HNSDHardwareControl::startCapture()
{
    return HNSD_HC_RESULT_SUCCESS;
}
