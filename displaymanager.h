//
// Created by Ian Parker on 06/03/2025.
//

#ifndef DISPLAYS_H
#define DISPLAYS_H

#include <string>
#include <vector>
#include <gst/gst.h>

#include <XPLMDisplay.h>

#include <ufc/logger.h>

class XScreenPlugin;
struct Texture;

struct Display
{
    //XScreenPlugin* plugin;
    GstElement* appSrc;
    Texture* texture;

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
    //XScreenPlugin* plugin;

    int textureNum;
    int textureWidth;
    int textureHeight;
    uint8_t* buffer;

    std::vector<Display*> displays;
};

class DisplayManager : UFC::Logger
{
    bool m_running = false;
    std::vector<Texture*> m_textures;
    std::vector<Display*> m_displays;
    float m_lastUpdate = 0.0f;

    static void copyDisplay(const Texture* texture, const Display* display);

    static int updateCallback(XPLMDrawingPhase inPhase, [[maybe_unused]] int inIsBefore, void *inRefcon);

    void dumpTexture(int i, const char * name);

    void update();

 public:
    DisplayManager() : Logger("DisplayManager") {}
    ~DisplayManager() override = default;

    bool start();
    bool stop();

    bool findDisplays();

    [[nodiscard]] std::vector<Display*> getDisplays() const { return m_displays; }

    void dumpTextures();
};

#endif //DISPLAYS_H
