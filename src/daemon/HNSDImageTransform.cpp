#include <iostream>

#include <opencv2/opencv.hpp>

#include "HNSDImageTransform.h"

HNSDPSOrthogonalRotate::HNSDPSOrthogonalRotate( std::string instance, std::string ownerID )
: HNSDPipelineStepBase( instance, ownerID )
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
HNSDPSOrthogonalRotate::initSupportedParameters( HNSDPipelineParameterMap *paramMap )
{
    // Add the parameters that apply to this transform
    paramMap->addParameter( getInstance(), "enable", "1", "desc" );
    paramMap->addParameter( getInstance(), "bulk_rotate_degrees", "270", "desc" );
    paramMap->addParameter( getInstance(), "result_image_index", "", "desc" );
}

bool
HNSDPSOrthogonalRotate::doesStepApply( HNSDPipelineParameterMap *paramMap )
{
    return true;
}

HNSDP_RESULT_T
HNSDPSOrthogonalRotate::executeInline( HNSDPipelineParameterMap *paramMap, HNSDStorageManager *storageMgr )
{
	std::string prevOwnerID;
    std::string prevInstanceID;
    HNSDStorageFile *filePtr;
    cv::Mat srcImage;
    cv::Mat rotImage;

    std::cout << "HNSDPSOrthogonalRotate::executeInline - start" << std::endl;

    // Find the output file from previous stage.
    if( paramMap->getPreviousOutputID( prevOwnerID, prevInstanceID ) != HNSDP_RESULT_SUCCESS )
        return HNSDP_RESULT_FAILURE;

    if( storageMgr->findFile( prevOwnerID, prevInstanceID, "output", &filePtr ) != HNSDSM_RESULT_SUCCESS )
        return HNSDP_RESULT_FAILURE;

    // Read image
    srcImage = cv::imread( filePtr->getPathAndFile(), cv::IMREAD_COLOR );

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


	if( storageMgr->allocateNewFile( getOwnerID(), getInstance(), "output", &filePtr ) != HNSDSM_RESULT_SUCCESS )
		return HNSDP_RESULT_FAILURE;

    if( cv::imwrite( filePtr->getPathAndFile(), rotImage ) == false )
    {
        printf("Failed to write output file\n");
        return HNSDP_RESULT_FAILURE;
    }

    paramMap->updatePreviousOutputID( getOwnerID(), getInstance() );

    return HNSDP_RESULT_SUCCESS;
}

HNSDP_RESULT_T
HNSDPSOrthogonalRotate::completeStep( HNSDPipelineParameterMap *paramMap, HNSDStorageManager *storageMgr )
{
    std::cout << "HNSDPSOrthogonalRotate::completeStep - start" << std::endl;

    return HNSDP_RESULT_SUCCESS;
}

HNSDPSCrop::HNSDPSCrop( std::string instance, std::string ownerID )
: HNSDPipelineStepBase( instance, ownerID )
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
HNSDPSCrop::initSupportedParameters( HNSDPipelineParameterMap *paramMap )
{
    // Add the parameters that apply to this transform
    paramMap->addParameter( getInstance(), "enable", "1", "desc" );
    paramMap->addParameter( getInstance(), "crop_factors", "0.13, 0.22, 0.0, 0.0", "desc" );
    paramMap->addParameter( getInstance(), "result_image_index", "", "desc" );
}

bool
HNSDPSCrop::doesStepApply( HNSDPipelineParameterMap *paramMap )
{
    return true;
}

HNSDP_RESULT_T
HNSDPSCrop::executeInline( HNSDPipelineParameterMap *paramMap, HNSDStorageManager *storageMgr )
{
	std::string prevOwnerID;
    std::string prevInstanceID;
    HNSDStorageFile *filePtr;
    cv::Mat srcImage;
    cv::Mat rotImage;

    std::cout << "HNSDPSCrop::executeInline - start" << std::endl;

    // Find the output file from previous stage.
    if( paramMap->getPreviousOutputID( prevOwnerID, prevInstanceID ) != HNSDP_RESULT_SUCCESS )
        return HNSDP_RESULT_FAILURE;

    if( storageMgr->findFile( prevOwnerID, prevInstanceID, "output", &filePtr ) != HNSDSM_RESULT_SUCCESS )
        return HNSDP_RESULT_FAILURE;

    // Read image
    srcImage = cv::imread( filePtr->getPathAndFile(), cv::IMREAD_COLOR );

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

	if( storageMgr->allocateNewFile( getOwnerID(), getInstance(), "output", &filePtr ) != HNSDSM_RESULT_SUCCESS )
		return HNSDP_RESULT_FAILURE;

    if( cv::imwrite( filePtr->getPathAndFile(), cropImage ) == false )
    {
        printf("Failed to write output file\n");
        return HNSDP_RESULT_FAILURE;
    }

    paramMap->updatePreviousOutputID( getOwnerID(), getInstance() );

    return HNSDP_RESULT_SUCCESS;
}

HNSDP_RESULT_T
HNSDPSCrop::completeStep( HNSDPipelineParameterMap *paramMap, HNSDStorageManager *storageMgr )
{
    std::cout << "HNSDPSCrop::completeStep - start" << std::endl;

    return HNSDP_RESULT_SUCCESS;
}
