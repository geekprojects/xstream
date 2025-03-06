

#include "xstreamplugin.h"

#include <XPLMProcessing.h>
#include <XPLMMenus.h>
#include <XPLMPlugin.h>
#include <XPLMDisplay.h>

#ifdef __APPLE__
#define GL_DO_NOT_WARN_IF_MULTI_GL_VERSION_HEADERS_INCLUDED 1
#include <OpenGL/OpenGLAvailability.h>
#include <OpenGL/gl.h>
#include <OpenGL/gl3.h>
#else

#define GL_GLEXT_PROTOTYPES 1
#define GL3_PROTOTYPES 1

#include <GL/gl.h>
#endif

using namespace std;
using namespace UFC;

XScreenPlugin g_ufcPlugin;

int XScreenPlugin::start(char* outName, char* outSig, char* outDesc)
{
    setLogPrinter(&m_logPrinter);

    fprintf(stderr, "UFC: XPluginStart: Here!\n");
    strcpy(outName, "X Screen Streamer");
    strcpy(outSig, "com.geekprojects.xscreen.plugin");
    strcpy(outDesc, "X Screen Streamer");

    m_menuContainer = XPLMAppendMenuItem(XPLMFindPluginsMenu(), "XScreen", 0, 0);
    m_menuId = XPLMCreateMenu("XScreen", XPLMFindPluginsMenu(), m_menuContainer, menuCallback, this);

    XPLMAppendMenuItem(m_menuId, "Start Streaming", (void*)3, 1);
    XPLMAppendMenuItem(m_menuId, "Dump FBOs...", (void*)2, 1);

    return 1;
}

void XScreenPlugin::stop()
{
}

int updateCallback([[maybe_unused]] XPLMDrawingPhase inPhase, [[maybe_unused]] int inIsBefore, void *inRefcon)
{
    auto plugin = static_cast<XScreenPlugin*>(inRefcon);
    plugin->updateDisplay();
    return 0;
}

int XScreenPlugin::enable()
{
    return 1;
}

void XScreenPlugin::disable()
{
}

void XScreenPlugin::receiveMessage(XPLMPluginID inFrom, int inMsg, void* inParam)
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

float XScreenPlugin::initCallback(float elapsedMe, float elapsedSim, int counter, void * refcon)
{
    return ((XScreenPlugin*)refcon)->init(elapsedMe, elapsedSim, counter);
}

float XScreenPlugin::init(float elapsedMe, float elapsedSim, int counter)
{
    return 0;
}

void XScreenPlugin::menuCallback(void* menuRef, void* itemRef)
{
    if (menuRef != nullptr)
    {
        ((XScreenPlugin*)menuRef)->menu(itemRef);
    }
}

void XScreenPlugin::copyDisplay(Texture* texture, Display* display)
{
    uintptr_t srcPos = 0;
    uintptr_t dstStride = display->width * 4;
    uintptr_t srcStride = texture->textureWidth * 4;
    uintptr_t dstPos = dstStride * (display->height - 1);

    srcPos += display->x * 4;
    srcPos += display->y * srcStride;

    // Copy backwards!
    for (int y = 0; y < display->height; y++)
    {
        memcpy(display->buffer + dstPos, texture->buffer + srcPos, dstStride);
        srcPos += srcStride;
        dstPos -= dstStride;
    }
}

void XScreenPlugin::updateDisplay()
{
    if (!m_streaming || !m_pushData)
    {
        return;
    }

    float now = XPLMGetElapsedTime();
    if (now - m_lastSent < 0.5f)
    {
        return;
    }
    m_lastSent = now;

    for (Texture* texture : m_textures)
    {
        log(DEBUG, "updateDisplay: Texture: %d", texture->textureNum);
        glBindTexture(GL_TEXTURE_2D, texture->textureNum);
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture->buffer);

        // Slice the texture up in to the separate displays
        for (Display* display : texture->displays)
        {
            copyDisplay(texture, display);
        }
    }
    glBindTexture(GL_TEXTURE_2D, 0);

}

void XScreenPlugin::dumpTexture(int i, const char* name)
{
    GLint width;
    GLint height;
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &height);
    log(DEBUG, "dumpTexture: %s %d:  -> size=%d, %d", name, i, width, height);

    if (width >= 2048 && height >= 2048)
    {
        unique_ptr<unsigned short[]> data(new unsigned short[width * height * 4]);
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.get());
        char filename[1024];
        snprintf(filename, 1024, "dump/texture_%s_%d.dat", name, i);
        FILE* fp = fopen(filename, "wb");
        fwrite(data.get(), width * height * 4, 1, fp);
        fclose(fp);
    }
}

void XScreenPlugin::dumpTextures()
{
    for (int i = 0; i < 1000; i++)
    {
        if (glIsTexture(i))
        {
            log(DEBUG, "dumpFBOs: %d: Found texture!");

            glBindTexture(GL_TEXTURE_2D, i);
            dumpTexture(i, "dump");
            glBindTexture(GL_TEXTURE_2D, 0);
        }
    }
}

void XScreenPlugin::findDisplay()
{
    Texture* texture = nullptr;
    for (int i = 1; i < 1000; i++)
    {
        if (glIsTexture(i))
        {
            log(DEBUG, "findDisplay: %d: Found texture!");

            glBindTexture(GL_TEXTURE_2D, i);

            GLint width;
            GLint height;
            glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
            glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &height);
            log(DEBUG, "findDisplay: Texture %d:  -> size=%d, %d", i, width, height);

            if (width == 2048 && height == 2048)
            {
                unique_ptr<uint8_t[]> data(new uint8_t[width * height * 4]);
                glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.get());
                log(DEBUG, "findDisplay: Texture %d:  -> %02x %02x %02x %02x", i, data[0], data[1], data[2], data[3]);
                if (data[0] == 0 && data[1] == 0 && data[2] == 0 && data[3] == 0xff)
                {
                    log(DEBUG, "findDisplay: Texture %d:  -> Texture matches pattern!", i);
                    texture = new Texture();
                    texture->textureNum = i;
                    texture->textureWidth = width;
                    texture->textureHeight = height;
                    texture->buffer = new uint8_t[width * height * 4];
                    texture->plugin = this;
                    break;
                }
            }
        }
    }
    glBindTexture(GL_TEXTURE_2D, 0);

    if (texture != nullptr)
    {
        log(DEBUG, "findDisplay: Adding display for texture %d\n", texture->textureNum);

        // PFD
        {
            auto display = new Display();
            display->plugin = this;
            display->x = 0;
            display->y = 0;
            display->width = 512;
            display->height = 512;
            display->buffer = new uint8_t[display->width * display->height * 4];
            memset(display->buffer, 0, display->width * display->height * 4);
            display->name = "pfd";
            texture->displays.push_back(display);
        }

        // ND
        {
            auto display = new Display();
            display->plugin = this;
            display->x = 512;
            display->y = 0;
            display->width = 512;
            display->height = 512;
            display->buffer = new uint8_t[display->width * display->height * 4];
            memset(display->buffer, 0, display->width * display->height * 4);
            display->name = "nd";
            texture->displays.push_back(display);
        }

        m_textures.push_back(texture);
    }
}

void XScreenPlugin::menu(void* itemRef)
{
    log(DEBUG, "menu: itemRef=%p", itemRef);
    auto item = static_cast<int>(reinterpret_cast<intptr_t>(itemRef));
    if (item == 2)
    {
        dumpTextures();
    }
    else if (item == 3)
    {
        startStream();
    }
}

void XScreenPlugin::dataSourceCallback(Display* display)
{
    display->plugin->dataSource(display);
}

void XScreenPlugin::startStream()
{
    if (m_streaming)
    {
        log(DEBUG, "startStream: Already streaming!");
        return;
    }

    log(DEBUG, "startStream: Finding texture...");
    findDisplay();

    if (!m_textures.empty())
    {
        log(DEBUG, "startStream: Registering callback...");
        XPLMRegisterDrawCallback(updateCallback, xplm_Phase_Panel, 0, this);

        log(DEBUG, "startStream: Starting...");
        m_streamMainThread = make_shared<thread>(&XScreenPlugin::streamMain, this);
        log(DEBUG, "startStream: Done");
    }
}

void XScreenPlugin::dataSource(Display* display)
{

}

void XScreenPlugin::enoughDataCallback(GstElement* appsrc, guint unused, Display* display)
{
    display->plugin->enoughData(appsrc, display);
}

void XScreenPlugin::enoughData(GstElement* appsrc, Display* display)
{
    log(DEBUG, "enoughData: display=%p", appsrc);
    m_pushData = false;
    /*
    if (m_sourceId != 0) {
        GST_DEBUG ("stop feeding");
        g_source_remove (m_sourceId);
        m_sourceId = 0;
    }
    */
}

void XScreenPlugin::needDataCallback(GstElement * appsrc, [[maybe_unused]] guint unused, Display* display)
{
    display->plugin->needData(appsrc, display);
}

void XScreenPlugin::needData(GstElement* appsrc, Display* display)
{
    log(DEBUG, "needData: display=%p", appsrc);
    m_pushData = true;
    guint size = display->width * display->height * 4;

    auto buffer = gst_buffer_new_allocate (nullptr, size, nullptr);
#if 0
    for (guint i = 0; i < size; i+=4)
    {
        //display->buffer[i + 0] = display->col;
        //display->buffer[i + 1] = display->col;
        //display->buffer[i + 2] = display->col;
        display->buffer[i + 3] = 255;
    }
#endif
    gst_buffer_fill(buffer, 0, display->buffer, size);

    //gst_buffer_memset (buffer, 0, 0xff, size);

    /* increment the timestamp every 1/2 second */
#if 0
    //GST_BUFFER_PTS (buffer) = m_timestamp;
    GST_BUFFER_DURATION (buffer) = gst_util_uint64_scale_int (1, GST_SECOND, 2);
    GST_BUFFER_FLAGS(buffer) |= GST_BUFFER_FLAG_LIVE;
    m_timestamp += GST_BUFFER_DURATION(buffer);
#endif

    GstFlowReturn ret;
    g_signal_emit_by_name (display->appSrc, "push-buffer", buffer, &ret);
    if (ret == GST_FLOW_FLUSHING)
    {
        //m_pushData = false;
        log(ERROR, "dataSource: GST_FLOW_FLUSHING");
    }
    else if (ret != GST_FLOW_OK)
    {
        log(ERROR, "dataSource: Error pushing data: %d", ret);
    }
    gst_buffer_unref (buffer);
}

void XScreenPlugin::mediaConfigure(GstRTSPMediaFactory* factory, GstRTSPMedia* media, Display* display)
{
    log(DEBUG, "mediaConfigure: media=%p, display=%p", media, display);

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
    g_signal_connect (display->appSrc, "need-data", (GCallback)needDataCallback, display);
    g_signal_connect (display->appSrc, "enough-data", (GCallback)enoughDataCallback, display);
    //gst_object_unref (display->appSrc);
    gst_object_unref (element);

    log(DEBUG, "mediaConfigure: Done");
}

void XScreenPlugin::mediaConfigureCallback(GstRTSPMediaFactory* factory, GstRTSPMedia* media, Display* display)
{
    display->plugin->mediaConfigure(factory, media, display);
}

void XScreenPlugin::streamMain()
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

    for (auto texture : m_textures)
    {
        for (auto display : texture->displays)
        {
            log(DEBUG, "streamMain: Creating factory for: /%s", display->name.c_str());
            auto factory = gst_rtsp_media_factory_new();
            gst_rtsp_media_factory_set_launch(
                factory,
                //"( appsrc name=mysrc is-live=1 ! videoconvert ! video/x-raw,format=I420  ! x264enc ! rtph264pay name=pay0 pt=96 )");
                //"( appsrc name=mysrc is-live=1 ! videoconvert ! video/x-raw,format=I420  ! x264enc ! rtph264pay name=pay0 pt=96 )");
        //"( appsrc name=mysrc block=false is-live=1 do-timestamp=1 min-latency=0 ! videoconvert ! video/x-raw,format=I420 ! x264enc speed-preset=superfast tune=zerolatency ! rtph264pay name=pay0 pt=96 )");
        "( appsrc name=mysrc block=false is-live=1 do-timestamp=1 min-latency=0 ! videoconvert ! video/x-raw,format=I420 ! vtenc_h264_hw realtime=true  ! rtph264pay name=pay0 pt=96 )");
                //"(appsrc name=mysrc ! videoconvert ! video/x-raw,format=I420  ! x264enc ! rtph264pay)");

            g_signal_connect(factory, "media-configure", (GCallback)mediaConfigureCallback, display);

            gst_rtsp_mount_points_add_factory (mounts, ("/" + display->name).c_str(), factory);
        }
    }

    g_object_unref (mounts);
    log(DEBUG, "streamMain: Attaching server...");
    gst_rtsp_server_attach(m_server, NULL);

    m_streaming = true;

    log(DEBUG, "streamMain: Starting loop...");
    g_main_loop_run(m_loop);
    log(DEBUG, "streamMain: Done!");
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
