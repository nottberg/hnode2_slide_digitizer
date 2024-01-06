#include <iostream>

#include "HNSDPipeline.h"

HNSDPipelineParameter::HNSDPipelineParameter()
{

}

HNSDPipelineParameter::~HNSDPipelineParameter()
{

}

std::string
HNSDPipelineParameter::generateParameterID( std::string instance, std::string name )
{
    return (instance + "." + name);
}

void
HNSDPipelineParameter::setName( std::string instance, std::string name )
{
    m_name = HNSDPipelineParameter::generateParameterID( instance, name );
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
    clear();
}

void
HNSDPipelineParameterMap::clear()
{
    // Cycle through each entry and free the parameter objects
    for( std::map< std::string, HNSDPipelineParameter* >::iterator it = m_nvPairs.begin(); it != m_nvPairs.end(); it++ )
    {
        delete it->second;
    }

    // clear the map
    m_nvPairs.clear();
}

HNSDP_RESULT_T
HNSDPipelineParameterMap::findParameter( std::string instance, std::string name, HNSDPipelineParameter **rtnParam )
{
    std::string pid = HNSDPipelineParameter::generateParameterID( instance, name );

    std::map< std::string, HNSDPipelineParameter* >::iterator it = m_nvPairs.find( pid );

    *rtnParam = NULL;

    if( it == m_nvPairs.end() )
        return HNSDP_RESULT_NOT_FOUND;

    *rtnParam = it->second;

    return HNSDP_RESULT_SUCCESS;
}

HNSDP_RESULT_T
HNSDPipelineParameterMap::addParameter( std::string instance, std::string name, std::string defaultValue, std::string description )
{
    HNSDPipelineParameter *param = NULL;

    if( addParameter( instance, name, &param ) != HNSDP_RESULT_SUCCESS )
        return HNSDP_RESULT_FAILURE;

    param->setDesc( description );
    param->setDefaultValue( defaultValue );

    return HNSDP_RESULT_SUCCESS;
}

HNSDP_RESULT_T
HNSDPipelineParameterMap::addParameter( std::string instance, std::string name, HNSDPipelineParameter **rtnParam )
{
    HNSDPipelineParameter *param = NULL;

    if( findParameter( instance, name, &param ) == HNSDP_RESULT_NOT_FOUND )
    {
        param = new HNSDPipelineParameter;
        param->setName( instance, name );
        m_nvPairs.insert( std::pair< std::string, HNSDPipelineParameter* >( param->getName(), param ) );
    }

    *rtnParam = param;

    return HNSDP_RESULT_SUCCESS;
}

HNSDP_RESULT_T
HNSDPipelineParameterMap::updatePreviousFileID( std::string purpose, std::string fileID )
{
    HNSDPipelineParameter *param = NULL;

    if( addParameter( "previous_file", purpose, &param ) != HNSDP_RESULT_SUCCESS )
        return HNSDP_RESULT_FAILURE;

    param->setActualValue( fileID );

    return HNSDP_RESULT_SUCCESS;
}

HNSDP_RESULT_T
HNSDPipelineParameterMap::getPreviousFileID( std::string purpose, std::string &fileID )
{
    HNSDPipelineParameter *param = NULL;

    if( findParameter( "previous_file", purpose, &param ) != HNSDP_RESULT_SUCCESS )
        return HNSDP_RESULT_FAILURE;

    fileID = param->getActualValueAsStr();

    return HNSDP_RESULT_SUCCESS;
}

HNSDPipelineStepBase::HNSDPipelineStepBase( std::string instance, std::string ownerID )
{
    m_instance = instance;
    m_ownerID = ownerID;
}

HNSDPipelineStepBase::~HNSDPipelineStepBase()
{

}

std::string
HNSDPipelineStepBase::getInstance()
{
    return m_instance;
}

std::string
HNSDPipelineStepBase::getOwnerID()
{
    return m_ownerID;
}

HNSDP_RESULT_T
HNSDPipelineStepBase::executeInline( HNSDPipelineParameterMap *paramMap, HNSDStorageManager *storageMgr )
{
    return HNSDP_RESULT_NOT_IMPLEMENTED;
}

HNSDP_RESULT_T
HNSDPipelineStepBase::createHardwareOperation( HNSDPipelineParameterMap *paramMap, HNSDStorageManager *storageMgr, HNSDHardwareOperation **rtnPtr )
{
    return HNSDP_RESULT_NOT_IMPLEMENTED;
}

HNSDP_RESULT_T 
HNSDPipelineStepBase::completedHardwareOperation( HNSDPipelineParameterMap *paramMap, HNSDStorageManager *storageMgr, HNSDHardwareOperation **rtnPtr )
{
    return HNSDP_RESULT_NOT_IMPLEMENTED;
}

HNSDPipeline::HNSDPipeline()
{
    m_activeStepIndex = 0;
    m_activeStep = NULL;

    setExecutionState( HNSDP_EXEC_STATE_NOTSET );
}

HNSDPipeline::~HNSDPipeline()
{

}

HNSDP_RESULT_T
HNSDPipeline::init( HNSDP_TYPE_T type, std::string ownerID, HNSDStorageManager *storageMgr, HNSDPipelineClientInterface *clientInf )
{
    m_ownerID = ownerID;
    m_storageManager = storageMgr;
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

HNSDP_RESULT_T
HNSDPipeline::initializeParameters()
{
    // Clear any existing configuration info
    m_paramMap.clear();
    
    // Let each step init its parameters.
    for( std::vector< HNSDPipelineStepBase* >::iterator it = m_pipeline.begin(); it != m_pipeline.end(); it++ )
    {
        (*it)->initSupportedParameters( &m_paramMap );
    }

    return HNSDP_RESULT_SUCCESS;
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
HNSDPipeline::setExecutionState( HNSDP_EXEC_STATE_T newState )
{
    // If change to same state then exit
    if( newState == m_execState )
        return;

    m_execState = newState;

    if( newState != HNSDP_EXEC_STATE_NOTSET )
    {
        std::cout << "=== HNSDPipeline - State Change - newState: " << getExecutionStateAsStr();
        std::cout << ",  stepIndex: " << m_activeStepIndex;
        std::cout << ",  stepInstance: " << ((m_activeStep != NULL) ? m_activeStep->getInstance() : "");
        std::cout << " ===" << std::endl;
    }
}

std::string
HNSDPipeline::getExecutionStateAsStr()
{
    switch( m_execState )
    {
        case HNSDP_EXEC_STATE_NOTSET:
            return "NotSet";
        case HNSDP_EXEC_STATE_PENDING:
            return "Pending";
        case HNSDP_EXEC_STATE_READY:
            return "Ready";
        case HNSDP_EXEC_STATE_RUNNING_STEP_START:
            return "StepStart";
        case HNSDP_EXEC_STATE_RUNNING_STEP_WAIT:
            return "StepWait";
        case HNSDP_EXEC_STATE_RUNNING_STEP_COMPLETE:
            return "StepComplete";
        case HNSDP_EXEC_STATE_INITIATE_NEXT_STEP:
            return "InitiateNextStep";
        case HNSDP_EXEC_STATE_COMPLETED:
            return "Completed";
    }

    return "Unknown";
}

void
HNSDPipeline::waitingForExecution()
{
    setExecutionState( HNSDP_EXEC_STATE_PENDING );

    m_clientInf->makePending();
}

void
HNSDPipeline::prepareForExecution()
{
    m_activeStepIndex = 0;
    m_activeStep = NULL;

    setExecutionState( HNSDP_EXEC_STATE_READY );

    m_clientInf->makeActive();
}

void
HNSDPipeline::finishExecution()
{
    m_activeStepIndex = 0;
    m_activeStep = NULL;

    setExecutionState( HNSDP_EXEC_STATE_COMPLETED );

    m_clientInf->makeComplete();
}

HNSDP_RESULT_T
HNSDPipeline::initiateNextStep()
{
    // Search for the next enabled step
    while( m_activeStepIndex < m_pipeline.size() )
    {
        HNSDPipelineStepBase *step = m_pipeline[ m_activeStepIndex ];

        if( step->doesStepApply( &m_paramMap ) == true )
        {
            m_activeStep = step;
            return HNSDP_RESULT_SUCCESS;
        }

        m_activeStepIndex += 1;
    }

    setExecutionState( HNSDP_EXEC_STATE_COMPLETED );

    return HNSDP_RESULT_PIPELINE_COMPLETE;
}

void
HNSDPipeline::startedStep()
{
    setExecutionState( HNSDP_EXEC_STATE_RUNNING_STEP_WAIT );
}

void
HNSDPipeline::completedStep()
{
    setExecutionState( HNSDP_EXEC_STATE_RUNNING_STEP_COMPLETE );
}

HNSDP_ACTION_T
HNSDPipeline::checkForStepAction()
{
    if( m_execState == HNSDP_EXEC_STATE_READY )
    {
        switch( initiateNextStep() )
        {
            case HNSDP_RESULT_SUCCESS:
                setExecutionState( HNSDP_EXEC_STATE_RUNNING_STEP_START );
            break;

            case HNSDP_RESULT_PIPELINE_COMPLETE:
                return HNSDP_ACTION_COMPLETE;
            break;

            case HNSDP_RESULT_FAILURE:
                return HNSDP_ACTION_FAILURE;
            break;
        }
    }

    if( m_execState == HNSDP_EXEC_STATE_RUNNING_STEP_START )
    {
        switch( m_activeStep->getType() )
        {
            case HNSD_PSTEP_TYPE_SPLIT_STEP:
                return HNSDP_ACTION_SPLIT_STEP;
            break;

            case HNSD_PSTEP_TYPE_HW_SPLIT_STEP:
                return HNSDP_ACTION_HW_SPLIT_STEP;
            break;

            case HNSD_PSTEP_TYPE_INLINE:
                return HNSDP_ACTION_INLINE;
            break;
        }
    }

    if( m_execState == HNSDP_EXEC_STATE_RUNNING_STEP_COMPLETE )
    {
        m_activeStep = NULL;
        m_activeStepIndex += 1;
        setExecutionState( HNSDP_EXEC_STATE_INITIATE_NEXT_STEP );
    }

    if( m_execState == HNSDP_EXEC_STATE_INITIATE_NEXT_STEP )
    {
        switch( initiateNextStep() )
        {
            case HNSDP_RESULT_SUCCESS:
                setExecutionState( HNSDP_EXEC_STATE_RUNNING_STEP_START );
            break;

            case HNSDP_RESULT_PIPELINE_COMPLETE:
                return HNSDP_ACTION_COMPLETE;
            break;

            case HNSDP_RESULT_FAILURE:
                return HNSDP_ACTION_FAILURE;
            break;
        }
    }

    return HNSDP_ACTION_WAIT;
}

HNSDP_RESULT_T
HNSDPipeline::executeInlineStep()
{
    std::cout << "HNSDPipeline::executeInlineStep - start" << std::endl;

    if( m_activeStep == NULL )
        return HNSDP_RESULT_FAILURE;

    return m_activeStep->executeInline( &m_paramMap, m_storageManager );
}

HNSDP_RESULT_T
HNSDPipeline::createHardwareOperation( HNSDHardwareOperation **rtnPtr )
{
    if( m_activeStep == NULL )
        return HNSDP_RESULT_FAILURE;

    return m_activeStep->createHardwareOperation( &m_paramMap, m_storageManager, rtnPtr );
}

HNSDP_RESULT_T
HNSDPipeline::completedHardwareOperation( HNSDHardwareOperation **opPtr )
{
    std::cout << "HNSDPipeline::completedHardwareOperation - start" << std::endl;

    if( m_activeStep == NULL )
        return HNSDP_RESULT_FAILURE;

    return m_activeStep->completedHardwareOperation( &m_paramMap, m_storageManager, opPtr );
}
