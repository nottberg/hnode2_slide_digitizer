#ifndef __HNSD_PIPELINE_H__
#define __HNSD_PIPELINE_H__

#include <any>
#include <map>
#include <mutex>
#include <string>
#include <memory>

#include <hnode2/HNReqWaitQueue.h>

typedef enum HNSDPipelineResultEnum
{
    HNSDP_RESULT_SUCCESS,
    HNSDP_RESULT_FAILURE,
    HNSDP_RESULT_NOT_IMPLEMENTED,
    HNSDP_RESULT_PIPELINE_COMPLETE    
}HNSDP_RESULT_T;

typedef enum HNSDPipelineTypeEnum {
    HNSDP_TYPE_NOTSET,    
    HNSDP_TYPE_IMAGE_CAPTURE
}HNSDP_TYPE_T;

typedef enum HNSDPipelineStepTypeEnum
{
    HNSD_PSTEP_TYPE_NOTSET,
    HNSD_PSTEP_TYPE_SPLIT_STEP,
    HNSD_PSTEP_TYPE_HW_SPLIT_STEP,
    HNSD_PSTEP_TYPE_INLINE
}HNSD_PSTEP_TYPE_T;

typedef enum HNSDPipelineActionEnum
{
    HNSDP_ACTION_WAIT,
    HNSDP_ACTION_HW_SPLIT_STEP,
    HNSDP_ACTION_SPLIT_STEP,
    HNSDP_ACTION_INLINE,
    HNSDP_ACTION_COMPLETE
}HNSDP_ACTION_T;

typedef enum HNSDPipelineExecutionStateEnum
{
    HNSDP_EXEC_STATE_NOTSET,
    HNSDP_EXEC_STATE_PENDING,
    HNSDP_EXEC_STATE_READY,
    HNSDP_EXEC_STATE_RUNNING_STEP_START,
    HNSDP_EXEC_STATE_RUNNING_STEP_WAIT,
    HNSDP_EXEC_STATE_COMPLETED
}HNSDP_EXEC_STATE_T;

// Forward declarations
class HNSDHardwareOperation;
class HNSDPipeline;

class HNSDPipelineParameter
{
    public:
        HNSDPipelineParameter();
       ~HNSDPipelineParameter();

        void setName( std::string instance, std::string name );
        void setDesc( std::string description );
        void setDefaultValue( std::string defaultValue );
        void setActualValue( std::string actualValue );

        std::string getName();
        std::string getDesc();
        std::string getDefaultValue();
        std::string getActualValueAsStr();

    private:
        std::string m_name;

        std::string m_description;

        std::string m_defaultValue;

        std::string m_actualValue;
};

class HNSDPipelineParameterMap
{
    public:
        HNSDPipelineParameterMap();
       ~HNSDPipelineParameterMap();

        void addParameter( std::string instance, std::string name, std::string defaultValue, std::string description );

    private:

        std::map< std::string, HNSDPipelineParameter > m_nvPairs;

};

class HNSDPipelineClientInterface
{
    public:
        virtual HNSDPipeline* getPipelinePtr() = 0;

        virtual std::string registerNextFilename( std::string purpose ) = 0;

        virtual std::string getLastOutputPathAndFile() = 0;

        virtual void makePending() = 0;

        virtual void makeActive() = 0;

        virtual void makeComplete() = 0;
};

class HNSDPipelineStepBase
{
    public:
        HNSDPipelineStepBase( std::string instance );
       ~HNSDPipelineStepBase();

        std::string getInstance();

        virtual HNSD_PSTEP_TYPE_T getType() = 0;

        virtual void initSupportedParameters( HNSDPipelineClientInterface *capture ) = 0;

        virtual bool doesStepApply( HNSDPipelineClientInterface *capture ) = 0;

        virtual HNSDP_RESULT_T executeInline( HNSDPipelineClientInterface *capture );

        virtual HNSDP_RESULT_T createHardwareOperation( HNSDPipelineClientInterface *capture, HNSDHardwareOperation **rtnPtr );

        virtual HNSDP_RESULT_T completeStep( HNSDPipelineClientInterface *capture ) = 0;

    private:

        std::string m_instance;
};

class HNSDPipeline
{
    public:
        HNSDPipeline();
       ~HNSDPipeline();

        HNSDP_RESULT_T init( HNSDP_TYPE_T type, HNSDPipelineClientInterface *clientInf );

        void addStep( HNSDPipelineStepBase *newStep );

        void addParameter( std::string instance, std::string name, std::string defaultValue, std::string description );

        uint getStepCount();

        HNSDPipelineStepBase* getStepByIndex( uint index );

        void waitingForExecution();

        void prepareForExecution();

        void finishExecution();

        HNSDP_RESULT_T initiateNextStep();

        void startedStep();

        void completedStep();

        HNSDP_ACTION_T checkForStepAction();

        void executeInlineStep();

        HNSDP_RESULT_T createHardwareOperation( HNSDHardwareOperation **rtnPtr );
        
        void hardwareOperationCompleted( HNSDHardwareOperation *opPtr );

    private:

        HNSDPipelineClientInterface *m_clientInf;

        std::vector< HNSDPipelineStepBase* > m_pipeline;

        HNSDPipelineParameterMap m_paramMap;

        HNSDP_EXEC_STATE_T m_execState;

        uint m_activeStepIndex;
        HNSDPipelineStepBase *m_activeStep;
};

class HNSDPipelineManagerIntf
{
    public:
        virtual HNSDP_RESULT_T allocatePipeline( HNSDP_TYPE_T type, HNSDPipelineClientInterface *clientInf, HNSDPipeline **rtnPipeline ) = 0;

        virtual HNSDP_RESULT_T submitPipelineForExecution( HNSDPipeline *pipeline ) = 0;

        virtual void releasePipeline( HNSDPipeline **pipeline ) = 0;
};

#endif //__HNSD_PIPELINE_H__
