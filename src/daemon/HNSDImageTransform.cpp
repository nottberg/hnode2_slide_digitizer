#include <sys/mman.h>
#include <sys/time.h>

#include <iostream>
#include <sstream>

#include <Poco/JSON/Object.h>
#include <Poco/JSON/Parser.h>
#include <Poco/StreamCopier.h>

#include <opencv2/opencv.hpp>

#include "HNSDImageTransform.h"

namespace pjs = Poco::JSON;

HNSDPSOrthogonalRotate::HNSDPSOrthogonalRotate( std::string instance )
: HNSDPipelineStepBase( instance )
{

}

HNSDPSOrthogonalRotate::~HNSDPSOrthogonalRotate()
{

}

HNSD_PSTEP_TYPE_T
HNSDPSOrthogonalRotate::getType()
{
    return HNSD_PSTEP_TYPE_INLINE;
}

void 
HNSDPSOrthogonalRotate::initSupportedParameters( HNSDPipelineManagerInterface *capture )
{
    // Get access to the parameters
    HNSDPipelineParameterMap *params = capture->getParamPtr();

    // Add the parameters that apply to this transform
    params->addParameter( getInstance(), "enable", "1", "desc" );
    params->addParameter( getInstance(), "bulk_rotate_degrees", "270", "desc" );
    params->addParameter( getInstance(), "result_image_index", "", "desc" );
}

bool
HNSDPSOrthogonalRotate::doesStepApply( HNSDPipelineManagerInterface *capture )
{
    return true;
}

HNSDP_RESULT_T
HNSDPSOrthogonalRotate::applyStep( HNSDPipelineManagerInterface *capture )
{
    std::cout << "HNSDPSOrthogonalRotate::applyStep - start" << std::endl;

    // Read image as grayscale
    cv::Mat srcImage;
    cv::Mat rotImage;

    std::string inFile = capture->getLastOutputPathAndFile();

    srcImage = cv::imread( inFile, cv::IMREAD_COLOR );

    if ( !srcImage.data )
    {
        printf("No image data \n");
        return HNSDP_RESULT_FAILURE;
    }

    // Print width and height
    uint ih = srcImage.rows;
    uint iw = srcImage.cols;
    printf( "HNSDPSOrthogonalRotate - width: %u  height: %u\n", iw, ih );

    cv::rotate( srcImage, rotImage, cv::ROTATE_90_COUNTERCLOCKWISE );

    std::string outFile = capture->registerNextFilename( "bulkRotate" );

    if( cv::imwrite( outFile, rotImage ) == false )
    {
        printf("Failed to write output file\n");
        return HNSDP_RESULT_FAILURE;
    }

    return HNSDP_RESULT_SUCCESS;
}

HNSDP_RESULT_T
HNSDPSOrthogonalRotate::completeStep( HNSDPipelineManagerInterface *capture )
{
    std::cout << "HNSDPSOrthogonalRotate::completeStep - start" << std::endl;

    return HNSDP_RESULT_SUCCESS;
}

HNSDPSCrop::HNSDPSCrop( std::string instance )
: HNSDPipelineStepBase( instance )
{

}

HNSDPSCrop::~HNSDPSCrop()
{

}

HNSD_PSTEP_TYPE_T
HNSDPSCrop::getType()
{
    return HNSD_PSTEP_TYPE_INLINE;
}

void 
HNSDPSCrop::initSupportedParameters( HNSDPipelineManagerInterface *capture )
{
    // Get access to the parameters
    HNSDPipelineParameterMap *params = capture->getParamPtr();

    // Add the parameters that apply to this transform
    params->addParameter( getInstance(), "enable", "1", "desc" );
    params->addParameter( getInstance(), "crop_factors", "0.13, 0.22, 0.0, 0.0", "desc" );
    params->addParameter( getInstance(), "result_image_index", "", "desc" );
}

bool
HNSDPSCrop::doesStepApply( HNSDPipelineManagerInterface *capture )
{
    return true;
}

HNSDP_RESULT_T
HNSDPSCrop::applyStep( HNSDPipelineManagerInterface *capture )
{
    std::cout << "HNSDPSCrop::applyStep - start" << std::endl;

    // Read image as grayscale
    cv::Mat srcImage;
    cv::Mat rotImage;

    std::string inFile = capture->getLastOutputPathAndFile();

    srcImage = cv::imread( inFile, cv::IMREAD_COLOR );

    if ( !srcImage.data )
    {
        printf("No image data \n");
        return HNSDP_RESULT_FAILURE;
    }

    // Print width and height
    uint ih = srcImage.rows;
    uint iw = srcImage.cols;
    printf( "HNSDPSCrop - width: %u  height: %u\n", iw, ih );

    // Crop to the region of interest
    double cropfactor[4];
    cropfactor[0] = 0.13;
    cropfactor[1] = 0.22;
    cropfactor[2] = 0.0;
    cropfactor[3] = 0.0;

    uint cx1 = (uint)( (double)ih * cropfactor[0] );
    uint cx2 = ih - (uint)( (double)ih * cropfactor[1] );

    uint cy1 = (uint)( (double)iw * cropfactor[2] );
    uint cy2 = iw - (uint)( (double)ih * cropfactor[3] );

    printf( "Crop Offsets - cx1: %u, cx2: %u, cy1: %u, cy2: %u\n", cx1, cx2, cy1, cy2 );

    cv::Mat cropImage = srcImage( cv::Range( cx1, cx2 ), cv::Range( cy1, cy2 ) );

    std::string outFile = capture->registerNextFilename( "crop" );

    if( cv::imwrite( outFile, cropImage ) == false )
    {
        printf("Failed to write output file\n");
        return HNSDP_RESULT_FAILURE;
    }

    return HNSDP_RESULT_SUCCESS;
}

HNSDP_RESULT_T
HNSDPSCrop::completeStep( HNSDPipelineManagerInterface *capture )
{
    std::cout << "HNSDPSCrop::completeStep - start" << std::endl;

    return HNSDP_RESULT_SUCCESS;
}
