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
    HNSDP_RESULT_FAILURE
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

class HNSDPipelineManagerInterface
{
    public:
        virtual HNSDPipelineParameterMap* getParamPtr() = 0;

        virtual std::string registerNextFilename( std::string purpose ) = 0;

        virtual std::string getLastOutputPathAndFile() = 0;
};

class HNSDPipelineStepBase
{
    public:
        HNSDPipelineStepBase( std::string instance );
       ~HNSDPipelineStepBase();

        std::string getInstance();

        virtual HNSD_PSTEP_TYPE_T getType() = 0;

        virtual void initSupportedParameters( HNSDPipelineManagerInterface *capture ) = 0;

        virtual bool doesStepApply( HNSDPipelineManagerInterface *capture ) = 0;

        virtual HNSDP_RESULT_T applyStep( HNSDPipelineManagerInterface *capture ) = 0;

        virtual HNSDP_RESULT_T completeStep( HNSDPipelineManagerInterface *capture ) = 0;

    private:

        std::string m_instance;
};

class HNSDPipeline
{
    public:
        HNSDPipeline();
       ~HNSDPipeline();

        HNSDP_RESULT_T init( HNSDP_TYPE_T type );

        void addStep( HNSDPipelineStepBase *newStep );

        uint getStepCount();

        HNSDPipelineStepBase* getStepByIndex( uint index );

    private:

        std::vector< HNSDPipelineStepBase* > m_pipeline;

};

#endif //__HNSD_PIPELINE_H__
