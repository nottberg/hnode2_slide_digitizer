#ifndef __JPEG_SERIALIZER_H__
#define __JPEG_SERIALIZER_H__

#include <stdint.h>

#include "CameraManager.h"

#if 0
typedef enum JPSRawImageFormatEnum
{
    JPS_RIF_NOTSET,
    JPS_RIF_YUYV,
    JPS_RIF_YUV420,  
}JPS_RIF_T;
#endif

class JPEGSerializer
{
    public:
        JPEGSerializer();
       ~JPEGSerializer();

        //void setRawSource( JPS_RIF_T format, uint width, uint height, uint stride, uint8_t *dataPtr, size_t dataLength );

        void serialize( CaptureRequest *request );

    private:
        
        void YUYV_to_JPEG(  CaptureRequest *request, uint outputWidth, uint outputHeight, uint8_t *&jpeg_buffer, size_t &jpeg_len );
        void YUV420_to_JPEG(  CaptureRequest *request, uint outputWidth, uint outputHeight, uint8_t *&jpeg_buffer, size_t &jpeg_len );

        void createThumbnail( CaptureRequest *request );
        void createExifData( CaptureRequest *request );
        void createImage( CaptureRequest *request );

        //std::string m_filename;

        //std::string m_platformName;
        //std::string m_cameraModel;

        //uint m_streamWidth;
        //uint m_streamHeight;
        //uint m_streamStride;

        uint m_outputWidth;
        uint m_outputHeight;

        uint m_thumbWidth;
        uint m_thumbHeight;

        //uint8_t  *m_rawDataPtr;
        //size_t    m_rawDataLength;
        //JPS_RIF_T m_rawFormat;

        uint m_restart;
        //uint m_quality;

        uint8_t *m_thumbBuffer;
        size_t   m_thumbLength;

        uint8_t *m_exifBuffer;
        size_t   m_exifLength;

        uint8_t *m_jpegBuffer;
        size_t   m_jpegLength;
};

#endif // __JPEG_SERIALIZER_H__