//
// Created by Ian Parker on 06/03/2025.
//

#ifndef VIDEOSTREAM_H
#define VIDEOSTREAM_H

#include <memory>
#include <thread>

#include <gst/gst.h>
#include <gst/rtsp-server/rtsp-server.h>

#include <ufc/logger.h>

struct Display;
class XScreenPlugin;
class VideoStream;

struct DisplayContext
{
    Display* display;
    VideoStream* videoStream;
};

class VideoStream : UFC::Logger
{
 private:
    XScreenPlugin* m_xscreenPlugin;

    std::shared_ptr<std::thread> m_streamMainThread;
    GMainLoop* m_loop = nullptr;
    GstRTSPServer* m_server = nullptr;
    //GstClockTime m_timestamp = 0;
    bool m_streaming = false;
    bool m_pushData = false;

    static void needDataCallback(GstElement* appsrc, guint unused, DisplayContext* displayData);
    void needData(GstElement* appsrc, Display* display);
    static void enoughDataCallback(GstElement* appsrc, guint unused, DisplayContext* displayData);
    void enoughData(GstElement* appsrc, Display* display);

    static void mediaConfigureCallback(GstRTSPMediaFactory* factory, GstRTSPMedia* media, DisplayContext* displayData);
    void mediaConfigure(GstRTSPMediaFactory* factory, GstRTSPMedia* media, Display* display);

    void streamMain();

 public:
    explicit VideoStream(XScreenPlugin* plugin) : Logger("VideoStream"), m_xscreenPlugin(plugin) {}
    ~VideoStream() = default;

    bool init();
    bool start();
    bool stop();
};



#endif //VIDEOSTREAM_H
