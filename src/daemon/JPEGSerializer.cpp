#include <cstdio>
#include <cstring>

#include <algorithm>
#include <iostream>
#include <map>
#include <stdexcept>
#include <vector>
#include <ctime>

#include <jpeglib.h>
#include <libexif/exif-data.h>

#include "JPEGSerializer.h"

static ExifEntry *exif_create_tag(ExifData *exif, ExifIfd ifd, ExifTag tag);
static void exif_set_string(ExifEntry *entry, char const *s);

static const ExifByteOrder exif_byte_order = EXIF_BYTE_ORDER_INTEL;
static const unsigned int exif_image_offset = 20; // offset of image in JPEG buffer
static const unsigned char exif_header[] = { 0xff, 0xd8, 0xff, 0xe1 };

ExifEntry *exif_create_tag( ExifData *exif, ExifIfd ifd, ExifTag tag )
{
	ExifEntry *entry = exif_content_get_entry( exif->ifd[ifd], tag );
	if( entry )
		return entry;
	entry = exif_entry_new();
	if( !entry )
    {
        std::cerr << "failed to allocate EXIF entry" << std::endl;
        return NULL;
    }

	entry->tag = tag;
	exif_content_add_entry( exif->ifd[ifd], entry );
	exif_entry_initialize( entry, entry->tag );
	exif_entry_unref( entry );
	return entry;
}

void exif_set_string( ExifEntry *entry, char const *s )
{
	if( entry->data )
		free( entry->data );
	entry->size = entry->components = strlen( s );
	entry->data = (unsigned char *)strdup( s );
	if( !entry->data )
    {
        std::cerr << "failed to copy exif string" << std::endl;
        return;
    }
	entry->format = EXIF_FORMAT_ASCII;
}

JPEGSerializer::JPEGSerializer()
{
    //m_filename = "/tmp/tmp.jpg";

    //m_platformName = "RaspberryPi";
    //m_cameraModel  = "";

    //m_streamWidth  = 0;
    //m_streamHeight = 0;
    //m_streamStride = 0;

    m_outputWidth  = 0;
    m_outputHeight = 0;

    m_thumbWidth  = 200;
    m_thumbHeight = 200;

    //m_rawDataPtr    = nullptr;
    //m_rawDataLength = 0;
    //m_rawFormat     = JPS_RIF_NOTSET;

    m_restart = 0;
    //m_quality = 93;

    m_thumbBuffer = nullptr;
    m_thumbLength = 0;

    m_exifBuffer = nullptr;
    m_exifLength = 0;

    m_jpegBuffer = nullptr;
    m_jpegLength = 0;
}

JPEGSerializer::~JPEGSerializer()
{

}

void
JPEGSerializer::YUYV_to_JPEG( CaptureRequest *request, uint outputWidth, uint outputHeight, uint8_t *&jpeg_buffer, size_t &jpeg_len )
{
	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;

	cinfo.err = jpeg_std_error( &jerr );
	jpeg_create_compress( &cinfo );

	cinfo.image_width      = outputWidth;
	cinfo.image_height     = outputHeight;
	cinfo.input_components = 3;
	cinfo.in_color_space   = JCS_YCbCr;
	cinfo.restart_interval = m_restart;

	jpeg_set_defaults( &cinfo );
	jpeg_set_quality( &cinfo, request->getOutputQuality(), TRUE );
	jpeg_buffer = NULL;
	jpeg_len = 0;
	jpeg_mem_dest( &cinfo, &jpeg_buffer, &jpeg_len );
	jpeg_start_compress( &cinfo, TRUE );

	const unsigned int output_width3 = 3 * outputWidth;
	std::vector<uint8_t> tmp_row( output_width3 );
	JSAMPROW jrow[1];
	jrow[0] = &tmp_row[0];

	// Pre-calculate the horizontal offsets to speed up the main loop.
	std::vector<unsigned int> h_offset( output_width3 );
	for( unsigned int i = 0, k = 0; i < outputWidth; i++ )
	{
		unsigned int off = (i * request->getStreamWidth()) / outputWidth * 2;
		unsigned int off_align = off & ~3;
		h_offset[k++] = off;
		h_offset[k++] = off_align + 1;
		h_offset[k++] = off_align + 3;
	}

	while (cinfo.next_scanline < outputHeight)
	{
		unsigned int offset = ((cinfo.next_scanline * request->getStreamHeight()) / outputHeight) * request->getStreamStride();
		for( unsigned int k = 0; k < output_width3; k += 3 )
		{
			tmp_row[k]     = request->getImageBufPtr()[offset + h_offset[k]];
			tmp_row[k + 1] = request->getImageBufPtr()[offset + h_offset[k + 1]];
			tmp_row[k + 2] = request->getImageBufPtr()[offset + h_offset[k + 2]];
		}
		jpeg_write_scanlines( &cinfo, jrow, 1 );
	}

	jpeg_finish_compress( &cinfo );
	jpeg_destroy_compress( &cinfo );
}

void
JPEGSerializer::YUV420_to_JPEG( CaptureRequest *request, uint outputWidth, uint outputHeight, uint8_t *&jpeg_buffer, size_t &jpeg_len )
{
	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;

	cinfo.err = jpeg_std_error( &jerr );
	jpeg_create_compress( &cinfo );

	cinfo.image_width      = outputWidth;
	cinfo.image_height     = outputHeight;
	cinfo.input_components = 3;
	cinfo.in_color_space   = JCS_YCbCr;
	cinfo.restart_interval = m_restart;

	jpeg_set_defaults( &cinfo );
	jpeg_set_quality( &cinfo, request->getOutputQuality(), TRUE );
	jpeg_buffer = NULL;
	jpeg_len = 0;
	jpeg_mem_dest( &cinfo, &jpeg_buffer, &jpeg_len );
	jpeg_start_compress( &cinfo, TRUE );

	const unsigned int output_width3 = 3 * outputWidth;
	std::vector<uint8_t> tmp_row( output_width3 );
	JSAMPROW jrow[1];
	jrow[0] = &tmp_row[0];

	const uint8_t *Y = request->getImageBufPtr();
	const uint8_t *U = Y + request->getStreamStride() * request->getStreamHeight();
	const uint8_t *V = U + (request->getStreamStride() / 2) * (request->getStreamHeight() / 2);

	// Pre-calculate the horizontal offsets to speed up the main loop.
	std::vector<unsigned int> h_offset( output_width3 );
	for( unsigned int i = 0, k = 0; i < outputWidth; i++ )
	{
		unsigned int off = (i * request->getStreamWidth()) / outputWidth;
		h_offset[k++] = off;
		h_offset[k++] = off / 2;
		h_offset[k++] = off / 2;
	}

	while( cinfo.next_scanline < outputHeight )
	{
		unsigned int offset = ( (cinfo.next_scanline *  request->getStreamHeight()) / outputHeight ) * request->getStreamStride();
		unsigned int offset_uv = (((cinfo.next_scanline / 2) * request->getStreamHeight()) / outputHeight) * ( request->getStreamStride() / 2);
		for( unsigned int k = 0; k < output_width3; k += 3 )
		{
			tmp_row[k]     = Y[offset + h_offset[k]];
			tmp_row[k + 1] = U[offset_uv + h_offset[k + 1]];
			tmp_row[k + 2] = V[offset_uv + h_offset[k + 2]];
		}
		jpeg_write_scanlines( &cinfo, jrow, 1 );
	}

	jpeg_finish_compress( &cinfo );
	jpeg_destroy_compress( &cinfo );
}

void 
JPEGSerializer::createThumbnail( CaptureRequest *request )
{
    switch( request->getRawFormat() )
    {
        case CS_STILLMODE_YUYV:
            YUYV_to_JPEG( request, request->getThumbnailWidth(), request->getThumbnailHeight(), m_thumbBuffer, m_thumbLength );
        break;

        case CS_STILLMODE_YUV420:
            YUV420_to_JPEG( request, request->getThumbnailWidth(), request->getThumbnailHeight(), m_thumbBuffer, m_thumbLength );
        break;

        default:
            std::cerr << "unsupported YUV format in JPEG encode" << std::endl;
        break;
    }

    // Make sure the thumbnail will fit with the exif data
    if( m_thumbLength > 60000)
    {
        free( m_thumbBuffer );
        m_thumbBuffer = NULL;
        m_thumbLength = 0;

        std::cerr << "Thumbnail too large for exif, reduce quality or size" << std::endl;
    }
}

void
JPEGSerializer::createImage( CaptureRequest *request )
{
    switch( request->getRawFormat() )
    {
        case CS_STILLMODE_YUYV:
            YUYV_to_JPEG( request, request->getOutputWidth(), request->getOutputHeight(), m_jpegBuffer, m_jpegLength );
        break;

        case CS_STILLMODE_YUV420:
            YUV420_to_JPEG( request, request->getOutputWidth(), request->getOutputHeight(), m_jpegBuffer, m_jpegLength );
        break;

        default:
            std::cerr << "unsupported YUV format in JPEG encode" << std::endl;
        break;
    }
}

void
JPEGSerializer::createExifData( CaptureRequest *request )
{
	m_exifBuffer = nullptr;
	ExifData *exif = nullptr;

	exif = exif_data_new();
	if( !exif )
    {
        std::cerr << "failed to allocate EXIF data" << std::endl;
        return;
    }

	exif_data_set_byte_order( exif, exif_byte_order );

	// First add some fixed EXIF tags.
    ExifEntry *entry = exif_create_tag( exif, EXIF_IFD_EXIF, EXIF_TAG_MAKE );
	exif_set_string( entry, request->getPlatformName().c_str() );

	entry = exif_create_tag( exif, EXIF_IFD_EXIF, EXIF_TAG_MODEL );
	exif_set_string( entry, request->getCameraModel().c_str() );
	
    entry = exif_create_tag( exif, EXIF_IFD_EXIF, EXIF_TAG_SOFTWARE );
	exif_set_string( entry, "hnode2-slide-digitizer" );
	
    entry = exif_create_tag( exif, EXIF_IFD_EXIF, EXIF_TAG_DATE_TIME );

	std::time_t raw_time;
	std::time( &raw_time );

	std::tm *time_info;
	char time_string[32];
	time_info = std::localtime( &raw_time );
	std::strftime( time_string, sizeof(time_string), "%Y:%m:%d %H:%M:%S", time_info );

	exif_set_string( entry, time_string );
	
    entry = exif_create_tag(exif, EXIF_IFD_EXIF, EXIF_TAG_DATE_TIME_ORIGINAL);
	exif_set_string(entry, time_string);
	
    entry = exif_create_tag(exif, EXIF_IFD_EXIF, EXIF_TAG_DATE_TIME_DIGITIZED);
	exif_set_string(entry, time_string);

	// Now add some tags filled in from the image metadata.
#if 0    
	auto exposure_time = metadata.get(libcamera::controls::ExposureTime);
	if (exposure_time)
	{
		entry = exif_create_tag(exif, EXIF_IFD_EXIF, EXIF_TAG_EXPOSURE_TIME);
		LOG(2, "Exposure time: " << *exposure_time);
		ExifRational exposure = { (ExifLong)*exposure_time, 1000000 };
			exif_set_rational(entry->data, exif_byte_order, exposure);
		}
		auto ag = metadata.get(libcamera::controls::AnalogueGain);
		if (ag)
		{
			entry = exif_create_tag(exif, EXIF_IFD_EXIF, EXIF_TAG_ISO_SPEED_RATINGS);
			auto dg = metadata.get(libcamera::controls::DigitalGain);
			float gain;
			gain = *ag * (dg ? *dg : 1.0);
			LOG(2, "Ag " << *ag << " Dg " << *dg << " Total " << gain);
			exif_set_short(entry->data, exif_byte_order, 100 * gain);
		}
		auto lp = metadata.get(libcamera::controls::LensPosition);
		if (lp)
		{
			entry = exif_create_tag(exif, EXIF_IFD_EXIF, EXIF_TAG_SUBJECT_DISTANCE);
			ExifRational dist = { 1000, (ExifLong)(1000.0 * *lp) };
			exif_set_rational(entry->data, exif_byte_order, dist);
		}

		// Command-line supplied tags.
		for (auto &exif_item : options->exif)
		{
			LOG(2, "Processing EXIF item: " << exif_item);
			exif_read_tag(exif, exif_item.c_str());
		}

		if (options->thumb_quality)
		{
			// Add some tags for the thumbnail. We put in dummy values for the thumbnail
			// offset/length to occupy the right amount of space, and fill them in later.
#endif
    if( m_thumbLength > 0 )
    {
	    std::cout << "Thumbnail dimensions are " << request->getThumbnailWidth() << " x " << request->getThumbnailHeight() << std::endl;

	    entry = exif_create_tag( exif, EXIF_IFD_1, EXIF_TAG_IMAGE_WIDTH );
	    exif_set_short( entry->data, exif_byte_order, request->getThumbnailWidth() );
	
        entry = exif_create_tag( exif, EXIF_IFD_1, EXIF_TAG_IMAGE_LENGTH );
	    exif_set_short( entry->data, exif_byte_order, request->getThumbnailHeight() );
	
    	entry = exif_create_tag( exif, EXIF_IFD_1, EXIF_TAG_COMPRESSION );
		exif_set_short( entry->data, exif_byte_order, 6 );
		
        ExifEntry *thumb_offset_entry = exif_create_tag( exif, EXIF_IFD_1, EXIF_TAG_JPEG_INTERCHANGE_FORMAT );
		exif_set_long( thumb_offset_entry->data, exif_byte_order, 0 );
		
        ExifEntry *thumb_length_entry = exif_create_tag( exif, EXIF_IFD_1, EXIF_TAG_JPEG_INTERCHANGE_FORMAT_LENGTH );
		exif_set_long( thumb_length_entry->data, exif_byte_order, m_thumbLength );

		// We actually have to write out an EXIF buffer to find out how long it is.
		m_exifLength = 0;
		exif_data_save_data( exif, &m_exifBuffer, (unsigned int *) &m_exifLength );
		free( m_exifBuffer );
		m_exifBuffer = nullptr;

		// Now fill in the correct offsets and length.
		unsigned int offset = m_exifLength - 6; // do not ask me why "- 6", I have no idea
		exif_set_long( thumb_offset_entry->data, exif_byte_order, offset );
	}

    // And create the EXIF data buffer *again*.
    exif_data_save_data( exif, &m_exifBuffer, (unsigned int *) &m_exifLength );
    exif_data_unref( exif );
    exif = nullptr;
}

#if 0
void
JPEGSerializer::setRawSource( JPS_RIF_T format, uint width, uint height, uint stride, uint8_t *dataPtr, size_t dataLength )
{
    m_rawFormat     = format;
    m_streamWidth   = width;
    m_streamHeight  = height;
    m_streamStride  = stride;
    m_rawDataPtr    = dataPtr;
    m_rawDataLength = dataLength;
}
#endif

void
JPEGSerializer::serialize( CaptureRequest *request )
{
	FILE *fp = nullptr;

    if( ( request->getStreamWidth() & 1) || ( request->getStreamHeight() & 1 ) )
    {
	    std::cerr << "both width and height must be even" << std::endl;
        return;
    }

    // If output width and height have not been set, then set
    // them to match the input stream width and height.
    if( m_outputHeight == 0 )
        m_outputHeight = request->getStreamHeight(); //m_streamHeight;

    if( m_outputWidth == 0 )
        m_outputWidth = request->getStreamWidth(); //m_streamWidth;

	// Make all the EXIF data, which includes the thumbnail.

	// Create thumbnail
    createThumbnail( request );

	// Create Exif data
    createExifData( request );

	// Create image body
    createImage( request );
	
    std::cout << "JPEG size is " << m_jpegLength << std::endl;

	// Write everything out.
	fp = fopen( request->getRawFilename().c_str(), "w" );
	if( !fp )
    {
        std::cerr << "failed to open file " << request->getRawFilename() << std::endl;
        goto cleanup;
    }

	std::cout << "EXIF data len " << m_exifLength << std::endl;
	std::cout << "Thumb len " << m_thumbLength << std::endl;
	std::cout << "JPEG len " << m_jpegLength << std::endl;

    // Write out the exif header
	if( fwrite( exif_header, sizeof(exif_header), 1, fp ) != 1 )
    {
		std::cerr << "failed to write file - output probably corrupt" << std::endl;
        goto fpcleanup;
    }
    
    // Write the exif data length upper byte field
    if( fputc( (m_exifLength + m_thumbLength + 2) >> 8, fp ) == EOF )
    {
		std::cerr << "failed to write file - output probably corrupt" << std::endl;
        goto fpcleanup;
    }
    
    // Write the exif data length lower byte field
    if( fputc( (m_exifLength + m_thumbLength + 2) & 0xff, fp ) == EOF ) 
    {
		std::cerr << "failed to write file - output probably corrupt" << std::endl;
        goto fpcleanup;
    }
    
    // Write the exif data
    if( fwrite( m_exifBuffer, m_exifLength, 1, fp ) != 1 ) 
    {
		std::cerr << "failed to write file - output probably corrupt" << std::endl;
        goto fpcleanup;
    }

    // Write the thumbnail buffer
	if( ( m_thumbLength != 0 ) && fwrite( m_thumbBuffer, m_thumbLength, 1, fp ) != 1 )
    {
		std::cerr << "failed to write file - output probably corrupt" << std::endl;
        goto fpcleanup;
    }
    
    // Write the image data
    if( fwrite( m_jpegBuffer + exif_image_offset, m_jpegLength - exif_image_offset, 1, fp ) != 1 )
    {
		std::cerr << "failed to write file - output probably corrupt" << std::endl;
        goto fpcleanup;
    }

fpcleanup:
    // Close the file descriptor
	fclose(fp);
	fp = nullptr;

cleanup:
	
    return;
}