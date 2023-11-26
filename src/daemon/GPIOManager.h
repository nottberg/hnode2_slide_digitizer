#ifndef __GPIO_MANAGER_H__
#define __GPIO_MANAGER_H__

#include <any>
#include <map>
#include <mutex>
#include <string>
#include <memory>
#include <gpiod.h>

#include <hnode2/HNReqWaitQueue.h>

typedef enum GPIOManagerResultEnum
{
    GPIOM_RESULT_SUCCESS,
    GPIOM_RESULT_FAILURE
} GPIOM_RESULT_T;

class GPIOManager
{
    public:
         GPIOManager();
        ~GPIOManager();

        GPIOM_RESULT_T start();
        GPIOM_RESULT_T stop();

        GPIOM_RESULT_T setForwardCycle();
        GPIOM_RESULT_T clearForwardCycle();

        GPIOM_RESULT_T setBackwardCycle();
        GPIOM_RESULT_T clearBackwardCycle();

    private:
	    struct gpiod_chip *m_chip;
	    struct gpiod_line *m_lineFW;
	    struct gpiod_line *m_lineRV;
};

#endif //__GPIO_MANAGER_H__
