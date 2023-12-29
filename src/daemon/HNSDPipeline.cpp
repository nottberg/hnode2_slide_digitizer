#include <sys/mman.h>
#include <sys/time.h>

#include <iostream>
#include <sstream>

#include <Poco/JSON/Object.h>
#include <Poco/JSON/Parser.h>
#include <Poco/StreamCopier.h>


#include "HNSDPipeline.h"
#include "HNSDPipelineManager.h"

namespace pjs = Poco::JSON;

HNSDPipelineParameter::HNSDPipelineParameter()
{

}

HNSDPipelineParameter::~HNSDPipelineParameter()
{

}

void
HNSDPipelineParameter::setName( std::string instance, std::string name )
{
    m_name = instance + "." + name;
}

void
HNSDPipelineParameter::setDesc( std::string description )
{
    m_description = description;
}

void
HNSDPipelineParameter::setDefaultValue( std::string defaultValue )
{
    m_defaultValue = defaultValue;
}

void
HNSDPipelineParameter::setActualValue( std::string actualValue )
{
    m_actualValue = actualValue;
}

std::string
HNSDPipelineParameter::getName()
{
    return m_name;
}

std::string
HNSDPipelineParameter::getDesc()
{
    return m_description;
}

std::string
HNSDPipelineParameter::getDefaultValue()
{
    return m_defaultValue;
}

std::string
HNSDPipelineParameter::getActualValueAsStr()
{
    return m_actualValue;
}

HNSDPipelineParameterMap::HNSDPipelineParameterMap()
{

}

HNSDPipelineParameterMap::~HNSDPipelineParameterMap()
{

}

void
HNSDPipelineParameterMap::addParameter( std::string instance, std::string name, std::string defaultValue, std::string description )
{

}

HNSDPipelineStepBase::HNSDPipelineStepBase( std::string instance )
{
    m_instance = instance;
}

HNSDPipelineStepBase::~HNSDPipelineStepBase()
{

}

std::string
HNSDPipelineStepBase::getInstance()
{
    return m_instance;
}

HNSDP_RESULT_T
HNSDPipelineStepBase::executeInline( HNSDPipelineClientInterface *capture )
{
    return HNSDP_RESULT_NOT_IMPLEMENTED;
}

HNSDP_RESULT_T
HNSDPipelineStepBase::createHardwareOperation( HNSDPipelineClientInterface *capture, HNSDHardwareOperation **rtnPtr )
{
    return HNSDP_RESULT_NOT_IMPLEMENTED;
}

HNSDPipeline::HNSDPipeline()
{
    m_activeStepIndex = 0;
    m_activeStep = NULL;

    m_execState = HNSDP_EXEC_STATE_NOTSET;
}

HNSDPipeline::~HNSDPipeline()
{

}

HNSDP_RESULT_T
HNSDPipeline::init( HNSDP_TYPE_T type, HNSDPipelineClientInterface *clientInf )
{
    m_clientInf = clientInf;

    return HNSDP_RESULT_SUCCESS;
}

void
HNSDPipeline::addStep( HNSDPipelineStepBase *newStep )
{
    m_pipeline.push_back( newStep );
}

void
HNSDPipeline::addParameter( std::string instance, std::string name, std::string defaultValue, std::string description )
{
    m_paramMap.addParameter( instance, name, defaultValue, description );
}

uint 
HNSDPipeline::getStepCount()
{
    return m_pipeline.size();
}

HNSDPipelineStepBase*
HNSDPipeline::getStepByIndex( uint index )
{
    if( index >= m_pipeline.size() )
        return NULL;

    return m_pipeline[ index ];
}

void
HNSDPipeline::waitingForExecution()
{
    m_execState = HNSDP_EXEC_STATE_PENDING;

    m_clientInf->makePending();
}

void
HNSDPipeline::prepareForExecution()
{
    m_activeStepIndex = 0;
    m_activeStep = NULL;

    m_execState = HNSDP_EXEC_STATE_READY;

    m_clientInf->makeActive();
}

void
HNSDPipeline::finishExecution()
{
    m_activeStepIndex = 0;
    m_activeStep = NULL;

    m_execState = HNSDP_EXEC_STATE_COMPLETED;

    m_clientInf->makeComplete();
}

HNSDP_RESULT_T
HNSDPipeline::initiateNextStep()
{
    // Search for the next enabled step
    while( m_activeStepIndex < m_pipeline.size() )
    {
        HNSDPipelineStepBase *step = m_pipeline[ m_activeStepIndex ];

        if( step->doesStepApply( m_clientInf ) == true )
        {
            m_activeStep = step;
            return HNSDP_RESULT_SUCCESS;
        }

        m_activeStepIndex += 1;
    }

    m_execState = HNSDP_EXEC_STATE_COMPLETED;

    return HNSDP_RESULT_PIPELINE_COMPLETE;
}

void
HNSDPipeline::startedStep()
{

}

void
HNSDPipeline::completedStep()
{

}

HNSDP_ACTION_T
HNSDPipeline::checkForStepAction()
{
#if 0
    switch( m_executionState )
    {
        case HNSDCAP_EXEC_STATE_PENDING:
        case HNSDCAP_EXEC_STATE_CAPTURE_WAIT:
        case HNSDCAP_EXEC_STATE_MOVE_WAIT:
        case HNSDCAP_EXEC_STATE_IMAGE_PROCESS_WAIT:
            return HNSDCAP_ACTION_WAIT;

        case HNSDCAP_EXEC_STATE_CAPTURE:
            return HNSDCAP_ACTION_START_CAPTURE;

        case HNSDCAP_EXEC_STATE_MOVE:
            return HNSDCAP_ACTION_START_ADVANCE;

        case HNSDCAP_EXEC_STATE_IMAGE_PROCESS:
            return HNSDCAP_ACTION_START_PIPELINE_STEP;

        case HNSDCAP_EXEC_STATE_NOTSET:
        case HNSDCAP_EXEC_STATE_COMPLETE:
            break;
    }
#endif
    return HNSDP_ACTION_COMPLETE;
}

void
HNSDPipeline::executeInlineStep()
{
    std::cout << "HNSDPipeline::executeInlineStep - start" << std::endl;
}

HNSDP_RESULT_T
HNSDPipeline::createHardwareOperation( HNSDHardwareOperation **rtnPtr )
{
    return HNSDP_RESULT_SUCCESS;
}

void
HNSDPipeline::hardwareOperationCompleted( HNSDHardwareOperation *opPtr )
{
    std::cout << "HNSDPipeline::hardwareOperationCompleted - start" << std::endl;
}
