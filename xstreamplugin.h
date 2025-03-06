//
// Created by Ian Parker on 19/11/2024.
//

#ifndef UFCPLUGIN_H
#define UFCPLUGIN_H

#define XPLM200 1
#define XPLM210 1
#define XPLM400 1

#include <XPLMMenus.h>
#include <XPLMUtilities.h>

#include <gst/gst.h>
#include <gst/rtsp-server/rtsp-server.h>

#include <ufc/logger.h>

#include <vector>
#include <thread>

class XPPluginDataSource;

class XPLogPrinter : public UFC::LogPrinter
{
 public:
    virtual ~XPLogPrinter() = default;

    void printf(const char* message, ...) override
    {
        va_list va;
        va_start(va, message);

        char buf[4096];
        vsnprintf(buf, 4094, message, va);
        XPLMDebugString(buf);

        va_end(va);
    }
};

class XScreenPlugin;

struct Display
{
    XScreenPlugin* plugin;
    GstElement* appSrc;

    int x;
    int y;
    int width;
    int height;
    std::string name;

    uint8_t* buffer;
    int col = 0;
};

struct Texture
{
    XScreenPlugin* plugin;

    int textureNum;
    int textureWidth;
    int textureHeight;
    uint8_t* buffer;

    std::vector<Display*> displays;
};

class XScreenPlugin : public UFC::Logger
{
 private:
    XPLogPrinter m_logPrinter;

    int m_menuContainer;
    XPLMMenuID m_menuId;

    // GStreamer stuff
    std::shared_ptr<std::thread> m_streamMainThread;
    GMainLoop* m_loop;
    GstRTSPServer* m_server;
    //GstClockTime m_timestamp = 0;
    bool m_streaming = false;
    bool m_pushData = false;
    float m_lastSent = 0.0f;

    //guint m_sourceId;

    std::vector<Texture*> m_textures;

    static float initCallback(float elapsedMe, float elapsedSim, int counter, void * refcon);
    float init(float elapsedMe, float elapsedSim, int counter);

    static void menuCallback(void* menuRef, void* itemRef);

    void copyDisplay(Texture* texture, Display* display);

    void dumpTextures();
    void findDisplay();

    void menu(void* itemRef);

    static void dataSourceCallback(Display* ctx);
    void dataSource(Display* display);

    static void needDataCallback(GstElement* appsrc, guint unused, Display* ctx);
    void needData(GstElement* appsrc, Display* display);
    static void enoughDataCallback(GstElement* appsrc, guint unused, Display* ctx);
    void enoughData(GstElement* appsrc, Display* display);

    static void mediaConfigureCallback(GstRTSPMediaFactory* factory, GstRTSPMedia* media, Display* display);
    void mediaConfigure(GstRTSPMediaFactory* factory, GstRTSPMedia* media, Display* display);

    void streamMain();

public:
    XScreenPlugin() : Logger("XScreenPlugin") {}

    int start(char* outName, char* outSig, char* outDesc);
    void stop();
    int enable();
    void disable();

    void startStream();

    void receiveMessage(XPLMPluginID inFrom, int inMsg, void * inParam);

    void dumpTexture(int i, const char * name);
    void updateDisplay();
};

#endif //UFCPLUGIN_H
