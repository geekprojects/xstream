

#include "xstreamplugin.h"

#include <XPLMProcessing.h>
#include <XPLMMenus.h>

#include "videostream.h"
#include "displaymanager.h"

using namespace std;
using namespace UFC;

XStreamPlugin g_ufcPlugin;

int XStreamPlugin::start(char* outName, char* outSig, char* outDesc)
{
    setLogPrinter(&m_logPrinter);

    fprintf(stderr, "UFC: XScreenPlugin: Here!\n");
    strcpy(outName, "XStream Display Streamer");
    strcpy(outSig, "com.geekprojects.xstream");
    strcpy(outDesc, "XStream Screen Streamer");

    m_menuContainer = XPLMAppendMenuItem(XPLMFindPluginsMenu(), "XScreen", nullptr, 0);
    m_menuId = XPLMCreateMenu("XScreen", XPLMFindPluginsMenu(), m_menuContainer, menuCallback, this);

    XPLMAppendMenuItem(m_menuId, "Start Streaming", (void*)3, 1);
    XPLMAppendMenuItem(m_menuId, "Dump Textures", (void*)2, 1);

    m_videoStream = make_shared<VideoStream>(this);
    m_displayManager = make_shared<DisplayManager>();

    return 1;
}

void XStreamPlugin::stop()
{
}


int XStreamPlugin::enable()
{
    return 1;
}

void XStreamPlugin::disable()
{
}

void XStreamPlugin::startStream()
{
    if (m_displayManager->findDisplays())
    {
        m_displayManager->start();
        m_videoStream->start();
    }
}

void XStreamPlugin::receiveMessage(XPLMPluginID inFrom, int inMsg, void* inParam)
{
#if 0
    switch (inMsg)
    {
        case XPLM_MSG_PLANE_LOADED:
        {
            int index = (int)(intptr_t)inParam;
            log(DEBUG, "receiveMessage: XPLM_MSG_PLANE_LOADED: index=%d", index);
            break;
        }
        default:
            log(DEBUG, "receiveMessage: Unhandled message: %d", inMsg);
    }
#endif
}

void XStreamPlugin::menuCallback(void* menuRef, void* itemRef)
{
    if (menuRef != nullptr)
    {
        ((XStreamPlugin*)menuRef)->menu(itemRef);
    }
}


void XStreamPlugin::menu(void* itemRef)
{
    log(DEBUG, "menu: itemRef=%p", itemRef);
    auto item = static_cast<int>(reinterpret_cast<intptr_t>(itemRef));
    if (item == 2)
    {
        m_displayManager->dumpTextures();
    }
    else if (item == 3)
    {
        startStream();
    }
}

PLUGIN_API int XPluginStart(char* outName, char* outSig, char* outDesc)
{
    return g_ufcPlugin.start(outName, outSig, outDesc);
}

PLUGIN_API void	XPluginStop()
{
    g_ufcPlugin.stop();
}

PLUGIN_API int XPluginEnable()
{
    return g_ufcPlugin.enable();
}

PLUGIN_API void XPluginDisable()
{
    g_ufcPlugin.disable();
}

PLUGIN_API void XPluginReceiveMessage(XPLMPluginID inFrom, int inMsg, void * inParam)
{
    g_ufcPlugin.receiveMessage(inFrom, inMsg, inParam);
}
