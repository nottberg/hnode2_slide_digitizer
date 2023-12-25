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

HNSDPipeline::HNSDPipeline()
{

}

HNSDPipeline::~HNSDPipeline()
{

}

HNSDP_RESULT_T
HNSDPipeline::init( HNSDP_TYPE_T type )
{
    return HNSDP_RESULT_SUCCESS;
}

void
HNSDPipeline::addStep( HNSDPipelineStepBase *newStep )
{
    m_pipeline.push_back( newStep );
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

