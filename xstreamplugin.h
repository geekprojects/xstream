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

#include <ufc/logger.h>

#include <thread>

class VideoStream;
class DisplayManager;

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


class XScreenPlugin : public UFC::Logger
{
 private:
    XPLogPrinter m_logPrinter;

    int m_menuContainer = -1;
    XPLMMenuID m_menuId = nullptr;

    std::shared_ptr<VideoStream> m_videoStream;
    std::shared_ptr<DisplayManager> m_displayManager;

    static void menuCallback(void* menuRef, void* itemRef);

    void menu(void* itemRef);

public:
    XScreenPlugin() : Logger("XScreenPlugin") {}

    int start(char* outName, char* outSig, char* outDesc);
    void stop();
    int enable();
    void disable();

    void startStream();

    void receiveMessage(XPLMPluginID inFrom, int inMsg, void * inParam);

    std::shared_ptr<VideoStream> getVideoStream() { return m_videoStream; }
    std::shared_ptr<DisplayManager> getDisplayManager() { return m_displayManager; }
};

#endif //UFCPLUGIN_H
