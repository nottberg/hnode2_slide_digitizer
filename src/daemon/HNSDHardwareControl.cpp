
#include "HNSDHardwareControl.h"

HNSDHardwareControl::HNSDHardwareControl()
{
    m_notifyFD = 0;

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
    return HNSD_HC_RESULT_SUCCESS;
}
