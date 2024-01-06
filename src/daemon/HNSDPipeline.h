#ifndef __HNSD_PIPELINE_H__
#define __HNSD_PIPELINE_H__

#include <vector>
#include <map>
#include <string>

#include "HNSDStorageManager.h"

typedef enum HNSDPipelineResultEnum
{
    HNSDP_RESULT_SUCCESS,
    HNSDP_RESULT_FAILURE,
    HNSDP_RESULT_NOT_IMPLEMENTED,
    HNSDP_RESULT_PIPELINE_COMPLETE,
    HNSDP_RESULT_NOT_FOUND
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
    HNSDP_ACTION_COMPLETE,
    HNSDP_ACTION_FAILURE
}HNSDP_ACTION_T;

typedef enum HNSDPipelineExecutionStateEnum
{
    HNSDP_EXEC_STATE_NOTSET,
    HNSDP_EXEC_STATE_PENDING,
    HNSDP_EXEC_STATE_READY,
    HNSDP_EXEC_STATE_RUNNING_STEP_START,
    HNSDP_EXEC_STATE_RUNNING_STEP_WAIT,
    HNSDP_EXEC_STATE_RUNNING_STEP_COMPLETE,
    HNSDP_EXEC_STATE_INITIATE_NEXT_STEP,    
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

        static std::string generateParameterID( std::string instance, std::string name );

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

        void clear();

        HNSDP_RESULT_T findParameter( std::string instance, std::string name, HNSDPipelineParameter **rtnParam );

        HNSDP_RESULT_T addParameter( std::string instance, std::string name, std::string defaultValue, std::string description );

        HNSDP_RESULT_T addParameter( std::string instance, std::string name, HNSDPipelineParameter **rtnParam );

        HNSDP_RESULT_T updatePreviousFileID( std::string purpose, std::string fileID );

        HNSDP_RESULT_T getPreviousFileID( std::string purpose, std::string &fileID );

    private:

        std::map< std::string, HNSDPipelineParameter* > m_nvPairs;

};

class HNSDPipelineClientInterface
{
    public:
        virtual HNSDPipeline* getPipelinePtr() = 0;

        //virtual std::string registerNextFilename( std::string purpose ) = 0;

        //virtual std::string getLastOutputPathAndFile() = 0;

        virtual void makePending() = 0;

        virtual void makeActive() = 0;

        virtual void makeComplete() = 0;
};

class HNSDPipelineStepBase
{
    public:
        HNSDPipelineStepBase( std::string instance, std::string ownerID );
       ~HNSDPipelineStepBase();

        std::string getInstance();
        std::string getOwnerID();

        virtual HNSD_PSTEP_TYPE_T getType() = 0;

        virtual void initSupportedParameters( HNSDPipelineParameterMap *params ) = 0;

        virtual bool doesStepApply( HNSDPipelineParameterMap *params ) = 0;

        virtual HNSDP_RESULT_T executeInline( HNSDPipelineParameterMap *params, HNSDStorageManager *fileMgr );

        virtual HNSDP_RESULT_T createHardwareOperation( HNSDPipelineParameterMap *params, HNSDStorageManager *fileMgr , HNSDHardwareOperation **rtnPtr );

        virtual HNSDP_RESULT_T completedHardwareOperation( HNSDPipelineParameterMap *params, HNSDStorageManager *fileMgr , HNSDHardwareOperation **rtnPtr );
 
        virtual HNSDP_RESULT_T completeStep( HNSDPipelineParameterMap *params, HNSDStorageManager *fileMgr ) = 0;

    private:

        std::string m_instance;
        std::string m_ownerID;
};

class HNSDPipeline
{
    public:
        HNSDPipeline();
       ~HNSDPipeline();

        HNSDP_RESULT_T init( HNSDP_TYPE_T type, std::string ownerID, HNSDStorageManager *storageMgr, HNSDPipelineClientInterface *clientInf );

        HNSDP_RESULT_T initializeParameters();

        void addStep( HNSDPipelineStepBase *newStep );

        void addParameter( std::string instance, std::string name, std::string defaultValue, std::string description );

        std::string getExecutionStateAsStr();

        uint getStepCount();

        HNSDPipelineStepBase* getStepByIndex( uint index );

        void waitingForExecution();

        void prepareForExecution();

        void finishExecution();

        HNSDP_RESULT_T initiateNextStep();

        void startedStep();

        void completedStep();

        HNSDP_ACTION_T checkForStepAction();

        HNSDP_RESULT_T executeInlineStep();

        HNSDP_RESULT_T createHardwareOperation( HNSDHardwareOperation **rtnPtr );
        
        HNSDP_RESULT_T completedHardwareOperation( HNSDHardwareOperation **opPtr );

    private:

        void setExecutionState( HNSDP_EXEC_STATE_T newState );

        HNSDStorageManager *m_storageManager;

        HNSDPipelineClientInterface *m_clientInf;

        std::string m_ownerID;

        std::vector< HNSDPipelineStepBase* > m_pipeline;

        HNSDPipelineParameterMap m_paramMap;

        HNSDP_EXEC_STATE_T m_execState;

        uint m_activeStepIndex;
        HNSDPipelineStepBase *m_activeStep;
};

class HNSDPipelineManagerIntf
{
    public:
        virtual HNSDP_RESULT_T allocatePipeline( HNSDP_TYPE_T type, std::string ownerID, HNSDPipelineClientInterface *clientInf, HNSDPipeline **rtnPipeline ) = 0;

        virtual HNSDP_RESULT_T submitPipelineForExecution( HNSDPipeline *pipeline ) = 0;

        virtual void releasePipeline( HNSDPipeline **pipeline ) = 0;
};

#endif //__HNSD_PIPELINE_H__
