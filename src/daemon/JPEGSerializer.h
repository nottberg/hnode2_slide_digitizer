#ifndef __JPEG_SERIALIZER_H__
#define __JPEG_SERIALIZER_H__

#if 0
class EXIFSerializer
{
    public:

    private:

        // typedef int (*ExifReadFunction)(char const *, unsigned char *);
        //static int exif_read_short(char const *str, unsigned char *mem);
        //static int exif_read_sshort(char const *str, unsigned char *mem);
        //static int exif_read_long(char const *str, unsigned char *mem);
        //static int exif_read_slong(char const *str, unsigned char *mem);
        //static int exif_read_rational(char const *str, unsigned char *mem);
        //static int exif_read_srational(char const *str, unsigned char *mem);

};
#endif

#if 0
    switch( info.pixel_format )
    {
        case libcamera::formats::YUYV:
            YUYV_to_JPEG(input, info, output_width, output_height, quality, restart, jpeg_buffer, jpeg_len);
        break;

        case libcamera::formats::YUV420:
            YUV420_to_JPEG(input, info, output_width, output_height, quality, restart, jpeg_buffer, jpeg_len);
        break;
#endif

typedef enum JPSRawImageFormatEnum
{
    JPS_RIF_NOTSET,
    JPS_RIF_YUYV,
    JPS_RIF_YUV420,  
}JPS_RIF_T;

class JPEGSerializer
{
    public:
        JPEGSerializer();
       ~JPEGSerializer();

        void setRawSource( JPS_RIF_T format, uint width, uint height, uint stride, uint8_t *dataPtr, size_t dataLength );

        void serialize();

    private:
        
        void YUYV_to_JPEG(  uint outputWidth, uint outputHeight, uint quality, uint8_t *&jpeg_buffer, size_t &jpeg_len );
        void YUV420_to_JPEG(  uint outputWidth, uint outputHeight, uint quality, uint8_t *&jpeg_buffer, size_t &jpeg_len );

        void createThumbnail();
        void createExifData();
        void createImage();

		//if ((info.width & 1) || (info.height & 1))
        //info, info.width, info.height
		//unsigned int off = (i * info.width) / output_width * 2;
		//unsigned int offset = ((cinfo.next_scanline * info.height) / output_height) * info.stride;

        std::string m_filename;

        std::string m_platformName;
        std::string m_cameraModel;

        uint m_streamWidth;
        uint m_streamHeight;
        uint m_streamStride;

        uint m_outputWidth;
        uint m_outputHeight;

        uint m_thumbWidth;
        uint m_thumbHeight;

        uint8_t  *m_rawDataPtr;
        size_t    m_rawDataLength;
        JPS_RIF_T m_rawFormat;

        uint m_restart;
        uint m_quality;

        uint8_t *m_thumbBuffer;
        size_t   m_thumbLength;

        uint8_t *m_exifBuffer;
        size_t   m_exifLength;

        uint8_t *m_jpegBuffer;
        size_t   m_jpegLength;
};

#endif // __JPEG_SERIALIZER_H__