#include <chrono>
#include <poll.h>
#include <unistd.h>
#include <signal.h>
#include <sys/signalfd.h>
#include <sys/stat.h>
#include <iostream>
#include <sys/mman.h>
#include <map>
#include <queue>

//#include "core/frame_info.hpp"
//#include "core/libcamera_app.hpp"
//#include "core/still_options.hpp"

//#include "output/output.hpp"

//#include "image/image.hpp"

#include "JPEGSerializer.h"
#include "CameraThread.h"

CameraThread::CameraThread()
{
    m_captureState = CTC_STATE_IDLE;
}

CameraThread::~CameraThread()
{

}

void
CameraThread::test()
{
    // Open Camera
  	std::cout << "Opening camera..." << std::endl;

	m_camMgr = std::make_unique<libcamera::CameraManager>();

    std::cout << "libcamera version: " << m_camMgr->version() << std::endl;

	int ret = m_camMgr->start();
	if (ret)
    {
		std::cerr << "camera manager failed to start, code " + std::to_string(-ret) << std::endl;
        return;
    }

	std::vector<std::shared_ptr<libcamera::Camera>> cameras = m_camMgr->cameras();
	// Do not show USB webcams as these are not supported in libcamera-apps!
	auto rem = std::remove_if(cameras.begin(), cameras.end(), [](auto &cam) { return cam->id().find("/usb") != std::string::npos; });
	cameras.erase(rem, cameras.end());
	std::sort(cameras.begin(), cameras.end(), [](auto l, auto r) { return l->id() > r->id(); });
	
	if (cameras.size() == 0)
    {
		std::cerr << "no cameras available" << std::endl;
        return;
    }

	std::string const &cam_id = cameras[0]->id();

    std::cout << "Cam 0 ID: " << cam_id << std::endl;

	std::shared_ptr<libcamera::Camera> camera = m_camMgr->get(cam_id);
	if( !camera )
    {
		std::cerr << "failed to find camera " + cam_id << std::endl;
        return;
    }

	if( camera->acquire() )
    {
		std::cerr << "failed to acquire camera " + cam_id << std::endl;
        return;
    }

	bool camera_acquired = true;

	std::cout << "Acquired camera " << cam_id << std::endl;

	std::cout << "Configuring still capture..." << std::endl;

	// Setup stream role
	std::vector<libcamera::StreamRole> stream_roles = { libcamera::StreamRole::StillCapture };
	std::unique_ptr< libcamera::CameraConfiguration > configuration = camera->generateConfiguration( stream_roles );
	if( !configuration )
    {
		std::cerr << "failed to generate still capture configuration" << std::endl;
        return;
    }

	// Now we get to override any of the default settings from the options_->
	configuration->at(0).pixelFormat = libcamera::formats::YUV420;

	//if (options->width)
		configuration->at(0).size.width = 4624; //options->width;
	//if (options->height)
		configuration->at(0).size.height = 3472; //options->height;

	configuration->at(0).colorSpace = libcamera::ColorSpace::Sycc;
	//configuration->transform = options->transform;
	
	// First finish setting up the configuration.
#if 0    
	libcamera::CameraConfiguration::Status validation = configuration->validate();
	if (validation == CameraConfiguration::Invalid)
		throw std::runtime_error("failed to valid stream configurations");
	else if (validation == CameraConfiguration::Adjusted)
		LOG(1, "Stream configuration adjusted");
#endif

	if( camera->configure( configuration.get() ) < 0 )
    {
        std::cerr << "failed to configure streams" << std::endl;
       	return;
    }

	std::cout << "Camera streams configured" << std::endl;

	std::cout << "=== Available controls ===" << std::endl;
	for( auto const &[id, info] : camera->controls() )
    {
		std::cout << id->name() << ":  " << info.toString() << std::endl;
    }

	// Next allocate all the buffers we need, mmap them and store them on a free list.

//    std::map< libcamera::FrameBuffer *, std::vector< libcamera::Span<uint8_t> > > mapped_buffers;
//    std::map< libcamera::Stream *, std::queue< libcamera::FrameBuffer * > > frame_buffers;

	std::unique_ptr< libcamera::Request > l_request;

	size_t buffer_size = 0;
	uint8_t *bufPtr = nullptr;

	libcamera::FrameBufferAllocator *allocator = new libcamera::FrameBufferAllocator( camera );
	for( libcamera::StreamConfiguration &config : *configuration )
	{
		libcamera::Stream *stream = config.stream();

        std::cout << "buffer alloc stream: " << stream->configuration().toString() << std::endl;

		if( allocator->allocate( stream ) < 0 )
        {
			std::cerr << "failed to allocate capture buffers" << std::endl;
            return;
        }

		if( allocator->buffers( stream ).size() != 1 )
		{
			std::cerr << "More than one allocated buffer" << std::endl;
            return;
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
			buffer_size += buffer->planes()[i].length;
			if( buffer->planes()[i].fd.get() != ogfd )
			{
				std::cerr << "Buffer plane fds do not match" << std::endl;
				return;
			}
		}

		// Memory map the whole buffer for the camera to capture into
		bufPtr = (uint8_t *) mmap( NULL, buffer_size, PROT_READ | PROT_WRITE, MAP_SHARED, plane.fd.get(), 0 );

        std::cout << "Buffer Map - ptr: " << bufPtr << "  length: " << buffer_size << std::endl;
                    
		//mapped_buffers[ buffer.get() ].push_back( libcamera::Span<uint8_t>( static_cast<uint8_t *>(bufPtr), buffer_size ) );
		//buffer_size = 0;
		//		}
		//	}
		//	frame_buffers[ stream ].push( buffer.get() );

		l_request = camera->createRequest();
		if( !l_request )
        {
            std::cerr << "failed to make request" << std::endl;
            return;
        }

		if( l_request->addBuffer( stream, buffer.get() ) < 0 )
        {
		    std::cerr << "failed to add buffer to request" << std::endl;
            return;
        }
	}
	
	std::cout << "Still capture setup complete" << std::endl;

	// Setup the camera controls
    libcamera::ControlList ctrlList( libcamera::controls::controls );

	// Set the region of interest
	libcamera::Rectangle sensor_area = *(camera->properties().get( libcamera::properties::ScalerCropMaximum ));
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

    std::cout << "Pre Camera Start" << std::endl;

	if( camera->start( &ctrlList ) )
    {
		std::cerr << "failed to start camera" << std::endl;
        return;
    }

	ctrlList.clear();

	std::cout << "Camera started!" << std::endl;

	camera->requestCompleted.connect( this, &CameraThread::requestComplete );

	// Send requests to wait for auto focus to lock up, etc.
	bool scanning = true;
	bool delay = false;
	do
	{
		if( m_captureStateMutex.try_lock() == false )
		{
			std::cout << "Polling capture state - retry" << std::endl;
			sleep( 1 );
			continue;
		}

		switch( m_captureState )
		{
			// Check if we are just waiting for the completion to occur
			case CTC_STATE_WAIT_COMPLETE:
			{
				std::cout << "Waiting for request complete" << std::endl;
				delay = true;
			}
			break;

			// Haven't submited the first request yet,
			// So build and submit that.
			case CTC_STATE_IDLE:
			{
				// Trigger the autofocus
				libcamera::ControlList cl;
				cl.set( libcamera::controls::AfMode, libcamera::controls::AfModeAuto );
				cl.set( libcamera::controls::AfTrigger, libcamera::controls::AfTriggerStart );

				l_request->controls() = std::move( cl );

				std::cout << "Queueing initial request" << std::endl;

				if( camera->queueRequest( l_request.get() ) < 0 )
    			{
					std::cerr << "Failed to queue request" << std::endl;
					m_captureStateMutex.unlock();
        			return;
    			}

				m_captureState = CTC_STATE_WAIT_COMPLETE;
			}
			break;

			// The camera is still scanning the autofocus
			// so resubmit the request to continue monitoring
			// for finished autofocus.
			case CTC_STATE_FOCUS:
			{
			    std::cout << "Queueing polling request" << std::endl;

				l_request->reuse( libcamera::Request::ReuseBuffers );
				
				libcamera::ControlList cl;
				l_request->controls() = std::move( cl );

				if( camera->queueRequest( l_request.get() ) < 0 )
    			{
					std::cerr << "Failed to queue request" << std::endl;
					m_captureStateMutex.unlock();
        			return;
    			}

				m_captureState = CTC_STATE_WAIT_COMPLETE;
			}
			break;

			// Autofocus has completed, so exit the 
			// loop and store this image as the one.
			case CTC_STATE_CAPTURED:
			{
			    std::cout << "Capture Request completed" << std::endl;
				scanning = false;
			}
			break;
		}

		// Release the mutex, since we are done modifying capture state
		m_captureStateMutex.unlock();

		// If we are waiting then delay a bit.
		if( delay == true )
		{
			sleep(1);
			delay = false;
		}

	}while( scanning == true );

	// Got our capture, shutdown the camera
	if( camera->stop() )
    {
		std::cout << "failed to stop camera" << std::endl;
    }

	if( camera )
		camera->requestCompleted.disconnect( this, &CameraThread::requestComplete );

	ctrlList.clear(); // no need for mutex here

	std::cout << "Camera stopped!" << std::endl;

	// Turn the capture into a jpeg file.
    JPEGSerializer jpgSer;

	libcamera::StreamConfiguration const &cfg = configuration->at(0);

    JPS_RIF_T pFormat = JPS_RIF_NOTSET;
	if( cfg.pixelFormat == libcamera::formats::YUYV )
        pFormat = JPS_RIF_YUYV;
	else if( cfg.pixelFormat == libcamera::formats::YUV420 )
        pFormat = JPS_RIF_YUV420;

    jpgSer.setRawSource( pFormat, cfg.size.width, cfg.size.height, cfg.stride, bufPtr, buffer_size );

    jpgSer.serialize();

	// Release the camera
	camera->release();

	// Reset the camera object
	camera.reset();

	// Reset the camera manager object
	m_camMgr.reset();

	std::cout << "Capture complete" << std::endl;
}

void
CameraThread::requestComplete( libcamera::Request *request )
{
    std::cout << "requestComplete - start" << std::endl;

	// Aquire the lock.
	m_captureStateMutex.lock();

    std::cout << "requestComplete - status: " << request->status() << std::endl;

	if( request->status() == libcamera::Request::RequestCancelled )
	{
		// If the request is cancelled while the camera is still running, it indicates
		// a hardware timeout. Let the application handle this error.
        std::cerr << "RequestCancelled, hardware timeout" << std::endl;
		m_captureStateMutex.unlock();
		return;
	}

	// Check if autofocus is still scanning
	int af_state = *request->metadata().get( libcamera::controls::AfState );
	std::cout << "requestComplete - afState: " << af_state << std::endl;

	if( af_state == libcamera::controls::AfStateScanning )
		m_captureState = CTC_STATE_FOCUS;
	else
		m_captureState = CTC_STATE_CAPTURED;

	std::cout << "requestComplete - capState: " << m_captureState << std::endl;

	m_captureStateMutex.unlock();

}
