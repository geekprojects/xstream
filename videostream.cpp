//
// Created by Ian Parker on 06/03/2025.
//

#include "videostream.h"
#include "displaymanager.h"
#include "xstreamplugin.h"

using namespace std;

bool VideoStream::start()
{
    if (m_streaming)
    {
        log(DEBUG, "start: Already streaming!");
        return true;
    }

    log(DEBUG, "start: Starting...");
    m_streamMainThread = make_shared<thread>(&VideoStream::streamMain, this);
    log(DEBUG, "start: Done");

    return true;
}

bool VideoStream::stop()
{
    if (m_loop != nullptr)
    {
        log(DEBUG, "stop: Stopping...");
        m_streaming = false;
        g_main_loop_quit(m_loop);
    }
    return true;
}

void VideoStream::enoughDataCallback([[maybe_unused]] GstElement* appsrc, [[maybe_unused]] guint unused, DisplayContext* displayData)
{
    displayData->videoStream->enoughData(displayData->display);
}

void VideoStream::enoughData(const shared_ptr<Display> &display)
{
    log(DEBUG, "enoughData: display=%s", display->name.c_str());
}

void VideoStream::needDataCallback([[maybe_unused]] GstElement* appsrc, [[maybe_unused]] guint unused, const DisplayContext* displayData)
{
    displayData->videoStream->needData(displayData->display);
}

void VideoStream::needData(const shared_ptr<Display> &display)
{
#ifdef DEBUG
    log(DEBUG, "needData: display=%p", appsrc);
#endif
    guint size = display->width * display->height * 4;

    auto buffer = gst_buffer_new_allocate (nullptr, size, nullptr);
    gst_buffer_fill(buffer, 0, display->buffer, size);

    GstFlowReturn ret;
    g_signal_emit_by_name (display->appSrc, "push-buffer", buffer, &ret);
    if (ret == GST_FLOW_FLUSHING)
    {
        log(ERROR, "dataSource: GST_FLOW_FLUSHING");
    }
    else if (ret != GST_FLOW_OK)
    {
        log(ERROR, "dataSource: Error pushing data: %d", ret);
    }
    gst_buffer_unref (buffer);
}

void VideoStream::mediaConfigure(GstRTSPMedia* media, const shared_ptr<Display> &display)
{
    log(DEBUG, "mediaConfigure: media=%p, display=%s", media, display->name.c_str());

    /* get the element used for providing the streams of the media */
    auto element = gst_rtsp_media_get_element (media);

    /* get our appsrc, we named it 'mysrc' with the name property */
    display->appSrc = gst_bin_get_by_name_recurse_up (GST_BIN (element), "mysrc");

    /* this instructs appsrc that we will be dealing with timed buffer */
    gst_util_set_object_arg (G_OBJECT (display->appSrc), "format", "time");

    /* configure the caps of the video */
    g_object_set (G_OBJECT (display->appSrc), "caps",
        gst_caps_new_simple ("video/x-raw",
            "format", G_TYPE_STRING, "RGBA",
            "width", G_TYPE_INT, display->width,
            "height", G_TYPE_INT, display->height,
            "framerate", GST_TYPE_FRACTION, 0, 1, NULL), NULL);

    /* install the callback that will be called when a buffer is needed */

    auto displayContext = g_new0(DisplayContext, 1);
    displayContext->display = display;
    displayContext->videoStream = this;

    g_object_set_data_full (G_OBJECT (media), "display-context", displayContext, (GDestroyNotify) g_free);

    g_signal_connect (display->appSrc, "need-data", (GCallback)needDataCallback, displayContext);
    g_signal_connect (display->appSrc, "enough-data", (GCallback)enoughDataCallback, displayContext);

    //gst_object_unref (display->appSrc);
    gst_object_unref (element);

    log(DEBUG, "mediaConfigure: Done");
}

void VideoStream::mediaConfigureCallback(GstRTSPMediaFactory* factory, GstRTSPMedia* media, DisplayContext* displayData)
{
    displayData->videoStream->mediaConfigure(media, displayData->display);
}

void VideoStream::streamMain()
{
    log(DEBUG, "streamMain: calling gst_init");

    GError* error = nullptr;
    auto res = gst_init_check(nullptr, nullptr, &error);
    if (!res)
    {
        if (error != nullptr)
        {
            log(ERROR, "streamMain: gst_init_check failed: %s", error->message);
        }
        else
        {
            log(ERROR, "streamMain: gst_init_check failed: Unknown reason");
        }
        return;
    }

    log(DEBUG, "streamMain: Creating main loop...");
    m_loop = g_main_loop_new(nullptr, FALSE);

    log(DEBUG, "streamMain: Creating server...");
    m_server = gst_rtsp_server_new ();

    log(DEBUG, "streamMain: Creating mount points...");
    auto mounts = gst_rtsp_server_get_mount_points(m_server);

    for (const auto& display : m_xscreenPlugin->getDisplayManager()->getDisplays())
    {
        log(DEBUG, "streamMain: Creating factory for: /%s", display->name.c_str());
        auto factory = gst_rtsp_media_factory_new();

        string launch = "(";

        // Our "appsrc" where we provide the data
        launch += "appsrc name=mysrc block=true is-live=1 do-timestamp=1 min-latency=0 ! ";

        // Add a queue, this will discard old frames!
        launch += " queue max-size-time=500000000 ! ";

        // Convert it in to YUV
        launch += "videoconvert ! video/x-raw,format=I420 ! ";

        // Encode it
        switch (m_codec)
        {
            case CODEC_H264:
                // May be slow to encode, lower bandwidth
#ifdef __APPLE__
                // Use Apple Media, using hardware acceleration where available
                launch += "vtenc_h264 quality=0.25 realtime=true ! ";
#else
                // Standard H264 encoder
                launch += "x264enc ! ";
#endif

                // Make it streamable
                launch += "rtph264pay name=pay0 pt=96 ";
                break;

            case CODEC_MJPEG:
                // Faster encoding, higher bandwidth
                launch += "jpegenc !";
                launch += " rtpjpegpay name=pay0";
                break;
        }
        launch += " )";

        gst_rtsp_media_factory_set_launch(factory, launch.c_str());

        auto displayContext = g_new0(DisplayContext, 1);
        displayContext->display = display;
        displayContext->videoStream = this;

        g_signal_connect(factory, "media-configure", (GCallback)mediaConfigureCallback, displayContext);

        gst_rtsp_mount_points_add_factory (mounts, ("/" + display->name).c_str(), factory);
    }

    g_object_unref (mounts);
    log(DEBUG, "streamMain: Attaching server...");
    gst_rtsp_server_attach(m_server, nullptr);

    m_streaming = true;

    log(DEBUG, "streamMain: Starting loop...");
    g_main_loop_run(m_loop);
    log(DEBUG, "streamMain: Done!");

    m_loop = nullptr;
}
