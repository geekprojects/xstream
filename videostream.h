//
// Created by Ian Parker on 06/03/2025.
//

#ifndef VIDEOSTREAM_H
#define VIDEOSTREAM_H

#include <thread>

#include <gst/gst.h>
#include <gst/rtsp-server/rtsp-server.h>

#include <ufc/logger.h>

struct Display;
class XStreamPlugin;
class VideoStream;

struct DisplayContext
{
    Display* display;
    VideoStream* videoStream;
};

class VideoStream : private UFC::Logger
{
 private:
    XStreamPlugin* m_xscreenPlugin;

    std::shared_ptr<std::thread> m_streamMainThread;
    GMainLoop* m_loop = nullptr;
    GstRTSPServer* m_server = nullptr;
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
    explicit VideoStream(XStreamPlugin* plugin) : Logger("VideoStream"), m_xscreenPlugin(plugin) {}
    ~VideoStream() override = default;

    bool start();
    bool stop();
};



#endif //VIDEOSTREAM_H
