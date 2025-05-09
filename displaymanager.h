//
// Created by Ian Parker on 06/03/2025.
//

#ifndef DISPLAYS_H
#define DISPLAYS_H

#include <string>
#include <vector>
#include <gst/gst.h>

#include <XPLMDisplay.h>

#include "logger.h"
#include <yaml-cpp/node/node.h>

class XStreamPlugin;
struct Texture;

struct Display
{
    GstElement* appSrc = nullptr;

    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
    std::string name;
    std::shared_ptr<Texture> texture;
    uint8_t* buffer = nullptr;

    Display() = default;

    Display(int x, int y, int width, int height, const std::string &name, const std::shared_ptr<Texture> &texture) :
        x(x),
        y(y),
        width(width),
        height(height),
        name(name),
        texture(texture)
    {
        buffer = new uint8_t[width * height * 4];
    }

    ~Display()
    {
        delete[] buffer;
    }
};

struct Texture
{
    int textureNum = 0;
    int textureWidth = 0;
    int textureHeight = 0;
    uint8_t* buffer = nullptr;

    std::vector<std::shared_ptr<Display>> displays;

    ~Texture()
    {
        delete[] buffer;
    }
};

class DisplayManager : private Logger
{
    bool m_running = false;
    std::vector<std::shared_ptr<Texture>> m_textures;
    std::vector<std::shared_ptr<Display>> m_displays;
    float m_lastUpdate = 0.0f;

    static void copyDisplay(const std::shared_ptr<Texture> &texture, const std::shared_ptr<Display> &display);

    static int updateCallback(XPLMDrawingPhase inPhase, [[maybe_unused]] int inIsBefore, void *inRefcon);

    std::shared_ptr<Texture> checkTexture(YAML::Node &displayDef, int textureNum, YAML::Node &textureDef);
    void dumpTexture(int i, std::string icao);

    void update();


    bool findDefinition(YAML::Node &result);

 public:
    DisplayManager() : Logger("DisplayManager") {}
    ~DisplayManager() override = default;

    bool start();
    bool stop();

    bool findDisplays();
    [[nodiscard]] std::vector<std::shared_ptr<Display>> getDisplays() const { return m_displays; }

    void dumpTextures();
};

#endif //DISPLAYS_H
