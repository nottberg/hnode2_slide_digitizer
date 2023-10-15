#include <sys/mman.h>

#include <iostream>
#include <sstream>

#include <Poco/JSON/Object.h>
#include <Poco/JSON/Parser.h>
#include <Poco/StreamCopier.h>

#include "CameraManager.h"

namespace pjs = Poco::JSON;


CameraControl::CameraControl()
{

}

CameraControl::~CameraControl()
{

}

CameraFormat::CameraFormat()
{

}

CameraFormat::~CameraFormat()
{

}


CaptureRequest::CaptureRequest()
{
    m_mode   = CS_STILLMODE_DEFAULT;
    m_width  = 4624;
    m_height = 3472;

    m_stride = 0;
}

CaptureRequest::~CaptureRequest()
{

}

void
CaptureRequest::setID( std::string id )
{
    m_id = id;
}

void
CaptureRequest::setImageFormat( CS_STILLMODE_T mode, uint width, uint height )
{
    m_mode   = mode;
    m_width  = width;
    m_height = height;
}

CM_RESULT_T
CaptureRequest::initAfterConfigSet()
{
    switch( m_mode )
    {
        case CS_STILLMODE_YUV420:
	        m_cfgObj->at(0).pixelFormat = libcamera::formats::YUV420;
        break;

        case CS_STILLMODE_YUYV:
        case CS_STILLMODE_DEFAULT:
        case CS_STILLMODE_NOTSET:
        default:
            m_mode = CS_STILLMODE_YUYV;
	        m_cfgObj->at(0).pixelFormat = libcamera::formats::YUYV;
        break;
    }

	m_cfgObj->at(0).size.width = m_width;
	
	m_cfgObj->at(0).size.height = m_height;

    return CM_RESULT_SUCCESS;
}

CM_RESULT_T
CaptureRequest::initAfterRequestSet()
{
    return CM_RESULT_SUCCESS;
}

std::string
CaptureRequest::getID()
{
    return m_id;
}

std::string 
CaptureRequest::getPlatformName()
{
    return "RaspberryPi";
}

std::string
CaptureRequest::getCameraModel()
{
    return "camModel";
}

CS_STILLMODE_T
CaptureRequest::getRawFormat()
{
    return m_mode;
}

size_t
CaptureRequest::getStreamWidth()
{
    return m_width;
}

size_t
CaptureRequest::getStreamHeight()
{
    return m_height;
}

size_t
CaptureRequest::getStreamStride()
{
    return m_stride; 
}

uint8_t *
CaptureRequest::getImageBufPtr()
{
    return m_imgBufPtr;
}

size_t
CaptureRequest::getThumbnailWidth()
{
    return 200;
}

size_t
CaptureRequest::getThumbnailHeight()
{
    return 200;
}

size_t
CaptureRequest::getOutputWidth()
{
    return m_width;
}

size_t
CaptureRequest::getOutputHeight()
{
    return m_height;
}

uint
CaptureRequest::getOutputQuality()
{
    return 93;
}

std::string
CaptureRequest::getRawFilename()
{
    return "/tmp/tmp.jpg";
}


Camera::Camera( CameraManager *parent, std::string id )
{
    m_id = id;
    m_parent = parent;

    m_eventCB = NULL;
    m_capReq = NULL;
}

Camera::~Camera()
{

}

void
Camera::printInfo()
{
    std::cout << "=== Camera: " << m_modelName << "  (id: " << m_libID << ")" << std::endl;
}

CM_RESULT_T
Camera::initFromLibrary()
{
    // Save away the library pointer
    //m_camPtr = objPtr;

    if( m_camPtr == nullptr )
        return CM_RESULT_SUCCESS;

    // Get the unique id for this camera.
    m_libID = m_camPtr->id();

    // Get a few well known camera properties info
    const libcamera::ControlList &cl = m_camPtr->properties();

    auto model = cl.get( libcamera::properties::Model );
    m_modelName = model ? *model : "";

	std::optional< libcamera::Span< const libcamera::Rectangle > > sensorSize = cl.get( libcamera::properties::PixelArrayActiveAreas );
	if( sensorSize ) // && ( sensorSize.type() == libcamera::ControlType::ControlTypeRectangle ) )
    {
        m_arrayHeight = (*sensorSize)[0].height;
        m_arrayWidth = (*sensorSize)[0].width;

        std::cout << "SensorSize: " << m_arrayWidth << "x" << m_arrayHeight << std::endl;
    }

    return CM_RESULT_SUCCESS;
}

#if 0
CM_RESULT_T
Camera::setStillCaptureMode( CS_STILLMODE_T mode, uint width, uint height )
{
    m_captureMode   = mode;
    m_captureWidth  = width;
    m_captureHeight = height;

    return CM_RESULT_SUCCESS;
}
#endif

std::string
Camera::getID()
{
    return m_id;
}

CM_RESULT_T
Camera::acquire( CaptureRequest *request, CameraEventInf *callback )
{
    // Set the callback interface
    m_eventCB = callback;

    // Save away the current request object
    m_capReq = request;

    if( m_camPtr->acquire() )
    {
		std::cerr << "failed to acquire camera " + getID() << std::endl;
        return CM_RESULT_FAILURE;
    }

    return CM_RESULT_SUCCESS;
}

CM_RESULT_T
Camera::configureForCapture()
{
	// Setup stream role
	std::vector<libcamera::StreamRole> stream_roles = { libcamera::StreamRole::StillCapture };
	m_capReq->m_cfgObj = m_camPtr->generateConfiguration( stream_roles );
	if( !m_capReq->m_cfgObj )
    {
		std::cerr << "failed to generate still capture configuration" << std::endl;
        return CM_RESULT_FAILURE;
    }

    m_capReq->initAfterConfigSet();

    //config.setImageFormat( m_captureMode, m_captureWidth, m_captureHeight );

//	configuration->at(0).colorSpace = libcamera::ColorSpace::Sycc;
	//configuration->transform = options->transform;
	
	// Validate the configuration.
	libcamera::CameraConfiguration::Status validation = m_capReq->m_cfgObj->validate();
	if( validation == libcamera::CameraConfiguration::Invalid )
    {
		std::cerr << "ERROR: validation failed" << std::endl;
		return CM_RESULT_FAILURE;
    }
	else if( validation == libcamera::CameraConfiguration::Adjusted )
    {
		std::cout << "camera configuration adjusted" << std::endl;
    }

    // Apply the configuration
	if( m_camPtr->configure( m_capReq->m_cfgObj.get() ) < 0 )
    {
        std::cerr << "failed to configure streams" << std::endl;
       	return CM_RESULT_FAILURE;
    }

	std::cout << "Camera streams configured" << std::endl;

    m_capReq->m_imgBufLength = 0;

	libcamera::FrameBufferAllocator *allocator = new libcamera::FrameBufferAllocator( m_camPtr );
	for( libcamera::StreamConfiguration &cfg : *(m_capReq->m_cfgObj) )
	{
		libcamera::Stream *stream = cfg.stream();

        std::cout << "buffer alloc stream: " << stream->configuration().toString() << std::endl;

		if( allocator->allocate( stream ) < 0 )
        {
			std::cerr << "failed to allocate capture buffers" << std::endl;
            return CM_RESULT_FAILURE;
        }

		if( allocator->buffers( stream ).size() != 1 )
		{
			std::cerr << "More than one allocated buffer" << std::endl;
            return CM_RESULT_FAILURE;
		}

        const std::unique_ptr< libcamera::FrameBuffer > &buffer = allocator->buffers( stream ).front();
		const libcamera::FrameBuffer::Plane &plane = buffer->planes()[0];

		std::cout << "Allocated buffer plane count: " << buffer->planes().size() << std::endl;
		std::cout << "plane 0 - size: " << buffer->planes()[0].length << "  fd: " << buffer->planes()[0].fd.get() << std::endl;
		std::cout << "plane 1 - size: " << buffer->planes()[1].length << "  fd: " << buffer->planes()[1].fd.get() << std::endl;
		std::cout << "plane 2 - size: " << buffer->planes()[2].length << "  fd: " << buffer->planes()[2].fd.get() << std::endl;

		// Add up all of the planes sizes and ensure they are all the same fd.
		int ogfd = plane.fd.get();
		for( uint i = 0; i < buffer->planes().size(); i++ )
		{
			m_capReq->m_imgBufLength += buffer->planes()[i].length;
			if( buffer->planes()[i].fd.get() != ogfd )
			{
				std::cerr << "Buffer plane fds do not match" << std::endl;
				return CM_RESULT_FAILURE;
			}
		}

		// Memory map the whole buffer for the camera to capture into
		m_capReq->m_imgBufPtr = (uint8_t *) mmap( NULL, m_capReq->m_imgBufLength, PROT_READ | PROT_WRITE, MAP_SHARED, plane.fd.get(), 0 );

        std::cout << "Buffer Map - ptr: " << m_capReq->m_imgBufPtr << "  length: " << m_capReq->m_imgBufLength << std::endl;
                    
		m_capReq->m_reqObj = m_camPtr->createRequest();
		if( !m_capReq->m_reqObj )
        {
            std::cerr << "failed to make request" << std::endl;
            return CM_RESULT_FAILURE;
        }

        std::cout << "Camera Configuration Stride: " << cfg.stride << std::endl;
        m_capReq->m_stride = cfg.stride;

        m_capReq->initAfterRequestSet();

		if( m_capReq->m_reqObj->addBuffer( stream, buffer.get() ) < 0 )
        {
		    std::cerr << "failed to add buffer to request" << std::endl;
            return CM_RESULT_FAILURE;
        }
	}
	
	std::cout << "Still capture setup complete" << std::endl;

    return CM_RESULT_SUCCESS;
}

CM_RESULT_T
Camera::start()
{
	// Setup the camera controls
    libcamera::ControlList ctrlList( libcamera::controls::controls );

	// Set the region of interest
	libcamera::Rectangle sensor_area = *( m_camPtr->properties().get( libcamera::properties::ScalerCropMaximum ) );
	int x = 0.25 * sensor_area.width;
	int y = 0.25 * sensor_area.height;
	int w = 0.4 * sensor_area.width;
	int h = 0.5 * sensor_area.height;
	libcamera::Rectangle crop( x, y, w, h );
	crop.translateBy( sensor_area.topLeft() );
	std::cout << "Using crop " << crop.toString() << std::endl;
	ctrlList.set( libcamera::controls::ScalerCrop, crop );

#if 0
	// Build a list of initial controls that we must set in the camera before starting it.
	// We don't overwrite anything the application may have set before calling us.
	if (!ctrlList.get(controls::ScalerCrop) && options_->roi_width != 0 && options_->roi_height != 0)
	{
		Rectangle sensor_area = *camera_->properties().get(properties::ScalerCropMaximum);
		int x = options_->roi_x * sensor_area.width;
		int y = options_->roi_y * sensor_area.height;
		int w = options_->roi_width * sensor_area.width;
		int h = options_->roi_height * sensor_area.height;
		Rectangle crop(x, y, w, h);
		crop.translateBy(sensor_area.topLeft());
		LOG(2, "Using crop " << crop.toString());
		ctrlList.set(controls::ScalerCrop, crop);
	}

	if (!ctrlList.get(controls::AfWindows) && !ctrlList.get(controls::AfMetering) && options_->afWindow_width != 0 &&
		options_->afWindow_height != 0)
	{
		Rectangle sensor_area = *camera_->properties().get(properties::ScalerCropMaximum);
		int x = options_->afWindow_x * sensor_area.width;
		int y = options_->afWindow_y * sensor_area.height;
		int w = options_->afWindow_width * sensor_area.width;
		int h = options_->afWindow_height * sensor_area.height;
		Rectangle afwindows_rectangle[1];
		afwindows_rectangle[0] = Rectangle(x, y, w, h);
		afwindows_rectangle[0].translateBy(sensor_area.topLeft());
		LOG(2, "Using AfWindow " << afwindows_rectangle[0].toString());
		//activate the AfMeteringWindows
		ctrlList.set(controls::AfMetering, controls::AfMeteringWindows);
		//set window
		ctrlList.set(controls::AfWindows, afwindows_rectangle);
	}

	// Framerate is a bit weird. If it was set programmatically, we go with that, but
	// otherwise it applies only to preview/video modes. For stills capture we set it
	// as long as possible so that we get whatever the exposure profile wants.
	if (!ctrlList.get(controls::FrameDurationLimits))
	{
		if (StillStream())
			ctrlList.set(controls::FrameDurationLimits,
						  libcamera::Span<const int64_t, 2>({ INT64_C(100), INT64_C(1000000000) }));
		else if (!options_->framerate || options_->framerate.value() > 0)
		{
			int64_t frame_time = 1000000 / options_->framerate.value_or(DEFAULT_FRAMERATE); // in us
			ctrlList.set(controls::FrameDurationLimits,
						  libcamera::Span<const int64_t, 2>({ frame_time, frame_time }));
		}
	}

	if (!ctrlList.get(controls::ExposureTime) && options_->shutter)
		ctrlList.set(controls::ExposureTime, options_->shutter.get<std::chrono::microseconds>());
	if (!ctrlList.get(controls::AnalogueGain) && options_->gain)
		ctrlList.set(controls::AnalogueGain, options_->gain);
	if (!ctrlList.get(controls::AeMeteringMode))
		ctrlList.set(controls::AeMeteringMode, options_->metering_index);
	if (!ctrlList.get(controls::AeExposureMode))
		ctrlList.set(controls::AeExposureMode, options_->exposure_index);
	if (!ctrlList.get(controls::ExposureValue))
		ctrlList.set(controls::ExposureValue, options_->ev);
	if (!ctrlList.get(controls::AwbMode))
		ctrlList.set(controls::AwbMode, options_->awb_index);
	if (!ctrlList.get(controls::ColourGains) && options_->awb_gain_r && options_->awb_gain_b)
		ctrlList.set(controls::ColourGains,
					  libcamera::Span<const float, 2>({ options_->awb_gain_r, options_->awb_gain_b }));
	if (!ctrlList.get(controls::Brightness))
		ctrlList.set(controls::Brightness, options_->brightness);
	if (!ctrlList.get(controls::Contrast))
		ctrlList.set(controls::Contrast, options_->contrast);
	if (!ctrlList.get(controls::Saturation))
		ctrlList.set(controls::Saturation, options_->saturation);
	if (!ctrlList.get(controls::Sharpness))
		ctrlList.set(controls::Sharpness, options_->sharpness);

	// AF Controls, where supported and not already set
	if (!ctrlList.get(controls::AfMode) && camera_->controls().count(&controls::AfMode) > 0)
	{
		int afm = options_->afMode_index;
		if (afm == -1)
		{
			// Choose a default AF mode based on other options
			if (options_->lens_position || options_->set_default_lens_position || options_->af_on_capture)
				afm = controls::AfModeManual;
			else
				afm = camera_->controls().at(&controls::AfMode).max().get<int>();
		}
		ctrlList.set(controls::AfMode, afm);
	}
	if (!ctrlList.get(controls::AfRange) && camera_->controls().count(&controls::AfRange) > 0)
		ctrlList.set(controls::AfRange, options_->afRange_index);
	if (!ctrlList.get(controls::AfSpeed) && camera_->controls().count(&controls::AfSpeed) > 0)
		ctrlList.set(controls::AfSpeed, options_->afSpeed_index);

	if (ctrlList.get(controls::AfMode).value_or(controls::AfModeManual) == controls::AfModeAuto)
	{
		// When starting a viewfinder or video stream in AF "auto" mode,
		// trigger a scan now (but don't move the lens when capturing a still).
		// If an application requires more control over AF triggering, it may
		// override this behaviour with prior settings of AfMode or AfTrigger.
		if (!StillStream() && !ctrlList.get(controls::AfTrigger))
			ctrlList.set(controls::AfTrigger, controls::AfTriggerStart);
	}
	else if ((options_->lens_position || options_->set_default_lens_position) &&
			 camera_->controls().count(&controls::LensPosition) > 0 && !ctrlList.get(controls::LensPosition))
	{
		float f;
		if (options_->lens_position)
			f = options_->lens_position.value();
		else
			f = camera_->controls().at(&controls::LensPosition).def().get<float>();
		LOG(2, "Setting LensPosition: " << f);
		ctrlList.set(controls::LensPosition, f);
	}

	if (options_->flicker_period && !ctrlList.get(controls::AeFlickerMode) &&
		camera_->controls().find(&controls::AeFlickerMode) != camera_->controls().end() &&
		camera_->controls().find(&controls::AeFlickerPeriod) != camera_->controls().end())
	{
		ctrlList.set(controls::AeFlickerMode, controls::FlickerManual);
		ctrlList.set(controls::AeFlickerPeriod, options_->flicker_period.get<std::chrono::microseconds>());
	}
#endif

	if( m_camPtr->start( &ctrlList ) )
    {
		std::cerr << "failed to start camera" << std::endl;
        return CM_RESULT_FAILURE;
    }

	ctrlList.clear();

	std::cout << "Camera started!" << std::endl;

	m_camPtr->requestCompleted.connect( this, &Camera::requestComplete );

    return CM_RESULT_SUCCESS;
}

CM_RESULT_T
Camera::stop()
{
	// Got our capture, shutdown the camera
	if( m_camPtr->stop() )
    {
		std::cout << "failed to stop camera" << std::endl;
        return CM_RESULT_FAILURE;
    }

	if( m_camPtr )
		m_camPtr->requestCompleted.disconnect( this, &Camera::requestComplete );

    return CM_RESULT_SUCCESS;
}

CM_RESULT_T
Camera::queueAutofocusRequest()
{
    // Trigger the autofocus
    libcamera::ControlList cl;
    cl.set( libcamera::controls::AfMode, libcamera::controls::AfModeAuto );
    cl.set( libcamera::controls::AfTrigger, libcamera::controls::AfTriggerStart );

    m_capReq->m_reqObj->controls() = std::move( cl );

    std::cout << "Queueing initial request" << std::endl;

    if( m_camPtr->queueRequest( m_capReq->m_reqObj.get() ) < 0 )
    {
        std::cerr << "Failed to queue request" << std::endl;
        return CM_RESULT_FAILURE;
    }

    return CM_RESULT_SUCCESS;
}

CM_RESULT_T
Camera::queueRequest()
{
    m_capReq->m_reqObj->reuse( libcamera::Request::ReuseBuffers );
				
    libcamera::ControlList cl;
    m_capReq->m_reqObj->controls() = std::move( cl );

    if( m_camPtr->queueRequest( m_capReq->m_reqObj.get() ) < 0 )
    {
        std::cerr << "Failed to queue request" << std::endl;
        return CM_RESULT_FAILURE;
    }

    return CM_RESULT_SUCCESS;
}

void
Camera::requestComplete( libcamera::Request *request )
{
    std::cout << "requestComplete - status: " << request->status() << std::endl;

    if( m_eventCB == NULL )
        return;

	if( request->status() == libcamera::Request::RequestCancelled )
	{
		// If the request is cancelled while the camera is still running, it indicates
		// a hardware timeout. Let the application handle this error.
        std::cerr << "RequestCancelled, hardware timeout" << std::endl;
        m_eventCB->requestEvent( CR_EVTYPE_REQ_CANCELED );
		return;
	}

	// Check if autofocus is still scanning
	int af_state = *request->metadata().get( libcamera::controls::AfState );

	switch( af_state )
    {
        case libcamera::controls::AfStateScanning:
        	std::cout << "requestComplete - autofocus scanning" << std::endl;
            m_eventCB->requestEvent( CR_EVTYPE_REQ_FOCUSING );
        break;

        case libcamera::controls::AfStateFailed:
        	std::cout << "requestComplete - autofocus failed" << std::endl;
            m_eventCB->requestEvent( CR_EVTYPE_REQ_FAILURE );
        break;

        case libcamera::controls::AfStateFocused:
        default:
        	std::cout << "requestComplete - autofocus complete - afState: " << af_state << std::endl;
            m_eventCB->requestEvent( CR_EVTYPE_REQ_COMPLETE );
        break;
    }
}

CM_RESULT_T
Camera::release()
{
    m_camPtr->release();

    return CM_RESULT_SUCCESS;
}

std::string
Camera::getLibraryInfoJSONStr()
{
    // Create a json root object
    pjs::Object jsRoot;
    pjs::Object jsCamera;
    pjs::Array jsProperties;
    pjs::Array jsControls;
    pjs::Array jsStreams;

    if( m_camPtr == nullptr )
        return "";

    jsRoot.set( "library", "libcamera" );
    jsRoot.set( "library-version", m_parent->getCameraLibraryVersion() );

    jsCamera.set( "id", m_id );
    jsCamera.set( "libID", m_libID );
    jsCamera.set( "model", m_modelName );

	// Disable any libcamera logging for this bit.
	//logSetTarget(LoggingTargetNone);

    // Aquire the camera while we interrogate it
	m_camPtr->acquire();

    // Get camera properties info
    const libcamera::ControlList &cl = m_camPtr->properties();

    const libcamera::ControlIdMap *cidMap = cl.idMap();

    for( libcamera::ControlList::const_iterator it = cl.begin(); it != cl.end(); it++ )
    {
        pjs::Object jsProperty;
        jsProperty.set( "id", it->first );
        jsProperty.set( "name", cidMap->at(it->first)->name() );
        jsProperty.set( "descriptive-value", it->second.toString() );

        jsProperties.add( jsProperty );
    }
    jsCamera.set( "properties", jsProperties );

    // Get camera controls info
    const libcamera::ControlInfoMap &cim = m_camPtr->controls();

    for( libcamera::ControlInfoMap::const_iterator it = cim.begin(); it != cim.end(); it++ )
    {
        pjs::Object jsControl;
        jsControl.set( "id", it->first->id() );
        jsControl.set( "type", (uint) it->first->type() );
        jsControl.set( "name", it->first->name() );
        jsControl.set( "descriptive-value", it->second.toString() );

        jsControls.add( jsControl );
    }
    jsCamera.set( "controls", jsControls );

    // Get Raw stream info
    pjs::Object jsRawStream;
    pjs::Array jsRSFormats;

    jsRawStream.set( "type", (uint) libcamera::StreamRole::Raw );
    jsRawStream.set( "typeStr", "Raw" );

	std::unique_ptr< libcamera::CameraConfiguration > rconfig = m_camPtr->generateConfiguration( {libcamera::StreamRole::Raw} );
	
    if( rconfig )
    {
	    const libcamera::StreamFormats &formats = rconfig->at( 0 ).formats();

	    if( formats.pixelformats().size() )
        {
	        for( const auto &pix : formats.pixelformats() )
	        {
                pjs::Object jsFormat;

                jsFormat.set( "mode", pix.toString() );

                pjs::Array jsFrameSizes;

		        unsigned int num = formats.sizes( pix ).size();
		        for( const auto &size : formats.sizes( pix ) )
		        {
                    pjs::Object jsFrameSize;
                    jsFrameSize.set( "width", size.width );
                    jsFrameSize.set( "height", size.height );
                    jsFrameSizes.add( jsFrameSize );
                }
                jsFormat.set( "frameSizes", jsFrameSizes );

                jsRSFormats.add( jsFormat );
            }
        }

    }

    jsRawStream.set( "formats", jsRSFormats );

    jsStreams.add( jsRawStream );

    // Get Raw stream info
    pjs::Object jsStillStream;
    pjs::Array jsStillFormats;

    jsStillStream.set( "type", (uint) libcamera::StreamRole::StillCapture );
    jsStillStream.set( "typeStr", "StillCapture" );

	std::unique_ptr< libcamera::CameraConfiguration > sconfig = m_camPtr->generateConfiguration( {libcamera::StreamRole::StillCapture} );
	
    if( sconfig )
    {
	    const libcamera::StreamFormats &formats = sconfig->at( 0 ).formats();

	    if( formats.pixelformats().size() )
        {
	        for( const auto &pix : formats.pixelformats() )
	        {
                pjs::Object jsFormat;

                jsFormat.set( "mode", pix.toString() );

                pjs::Array jsFrameSizes;

		        unsigned int num = formats.sizes( pix ).size();
		        for( const auto &size : formats.sizes( pix ) )
		        {
                    pjs::Object jsFrameSize;
                    jsFrameSize.set( "width", size.width );
                    jsFrameSize.set( "height", size.height );
                    jsFrameSizes.add( jsFrameSize );
                }
                jsFormat.set( "frameSizes", jsFrameSizes );

                jsStillFormats.add( jsFormat );
            }
        }

    }

    jsStillStream.set( "formats", jsStillFormats );

    jsStreams.add( jsStillStream );

    jsCamera.set( "streams", jsStreams );

    // Add the camera object to the returned data
    jsRoot.set( "camera", jsCamera );

    // Done interrogating camera
	m_camPtr->release();

    // Render response content
    std::stringstream ostr;
    try{ 
        pjs::Stringifier::stringify( jsRoot, ostr, 1 ); 
    } catch( const std::exception &err ) {
        std::cerr << "ERROR: Exception while serializing library info: " << err.what() << std::endl;
        return "";
    } 

    return ostr.str();
}

CameraManager::CameraManager()
{

}

CameraManager::~CameraManager()
{

}

CM_RESULT_T
CameraManager::start()
{
    // Open Camera
  	std::cout << "Opening camera..." << std::endl;

	m_camMgr = std::make_unique< libcamera::CameraManager >();

    std::cout << "libcamera version: " << m_camMgr->version() << std::endl;

	int ret = m_camMgr->start();
	if( ret )
    {
		std::cerr << "camera manager failed to start, code " + std::to_string(-ret) << std::endl;
        return CM_RESULT_FAILURE;
    }

    return CM_RESULT_SUCCESS;
}

CM_RESULT_T
CameraManager::stop()
{
    // Reset the camera manager object
	m_camMgr.reset();

    return CM_RESULT_SUCCESS;
}

CM_RESULT_T
CameraManager::initCameraList()
{
    char tmpID[64];

    // Cleanup and previous camera list
    cleanupCameraList();

    // Call the library to get a current list of cameras
	std::vector< std::shared_ptr< libcamera::Camera> > cameras = m_camMgr->cameras();

	// Do not show USB webcams as these are not supported in libcamera-apps!
	auto rem = std::remove_if( cameras.begin(), cameras.end(), [](auto &cam) { return cam->id().find("/usb") != std::string::npos; } );
	cameras.erase( rem, cameras.end() );
	std::sort( cameras.begin(), cameras.end(), [](auto l, auto r) { return l->id() > r->id(); } );

    // Build the local list of available cameras
    uint camIndx = 0;
    for( std::vector< std::shared_ptr< libcamera::Camera > >::iterator it = cameras.begin(); it != cameras.end(); it++ )
    {
        sprintf( tmpID, "cam%u", camIndx );

        std::cout << "Adding camera - id: " << tmpID << std::endl;

        std::shared_ptr< Camera > camPtr = std::make_shared< Camera >( this, tmpID );
        
        // Save away the library pointer, and initialize the camera object.
        camPtr->m_camPtr = *it;
        camPtr->initFromLibrary();

        m_camMap.insert( std::pair< std::string, std::shared_ptr< Camera > >( tmpID, camPtr ) );

        camIndx += 1;
    }

    return CM_RESULT_SUCCESS;
}

CM_RESULT_T
CameraManager::cleanupCameraList()
{
    m_camMap.clear();

    return CM_RESULT_SUCCESS;
}

std::string
CameraManager::getCameraLibraryVersion()
{
    return ( m_camMgr ? m_camMgr->version() : "" );
}

void
CameraManager::getCameraIDList( std::vector< std::string > &idList )
{
    idList.clear();

    for( std::map< std::string, std::shared_ptr< Camera > >::iterator it = m_camMap.begin(); it != m_camMap.end(); it++ )
        idList.push_back( it->first );
}

std::shared_ptr< Camera >
CameraManager::lookupCameraByID( std::string id )
{
    // Try to lookup the bamera by id
    std::map< std::string, std::shared_ptr< Camera > >::iterator it = m_camMap.find( id );

    // If not found, return null
    if( it == m_camMap.end() )
        return nullptr;

    return it->second;
}
