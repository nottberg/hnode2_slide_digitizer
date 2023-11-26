#include <sys/mman.h>

#include <iostream>
#include <sstream>

#include <Poco/JSON/Object.h>
#include <Poco/JSON/Parser.h>
#include <Poco/StreamCopier.h>

#include "GPIOManager.h"

namespace pjs = Poco::JSON;

GPIOManager::GPIOManager()
{

}

GPIOManager::~GPIOManager()
{

}

static const char *gChipName = "gpiochip0";

GPIOM_RESULT_T
GPIOManager::start()
{
  	// Open GPIO chip
  	m_chip = gpiod_chip_open_by_name( gChipName );

  	// Open GPIO lines
  	m_lineFW = gpiod_chip_get_line( m_chip, 21 );
  	m_lineRV = gpiod_chip_get_line( m_chip, 20 );

  	// Open LED lines for output
  	gpiod_line_request_output( m_lineFW, "forward_cycle", 0 );
  	gpiod_line_request_output( m_lineRV, "backward_cycle", 0 );

    return GPIOM_RESULT_SUCCESS;
}

GPIOM_RESULT_T
GPIOManager::stop()
{
	// Release lines and chip
	gpiod_line_release( m_lineFW );
	gpiod_line_release( m_lineRV );

	gpiod_chip_close( m_chip );

    return GPIOM_RESULT_SUCCESS;
}

GPIOM_RESULT_T
GPIOManager::setForwardCycle()
{
    gpiod_line_set_value( m_lineFW, 1 );
    return GPIOM_RESULT_SUCCESS;
}

GPIOM_RESULT_T
GPIOManager::clearForwardCycle()
{
    gpiod_line_set_value( m_lineFW, 0 );
    return GPIOM_RESULT_SUCCESS;
}

GPIOM_RESULT_T
GPIOManager::setBackwardCycle()
{
    gpiod_line_set_value( m_lineRV, 1 );
    return GPIOM_RESULT_SUCCESS;
}

GPIOM_RESULT_T
GPIOManager::clearBackwardCycle()
{
    gpiod_line_set_value( m_lineRV, 0 );
    return GPIOM_RESULT_SUCCESS;
}
