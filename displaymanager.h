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
    std::shared_ptr<uint8_t[]> buffer;

    Display() = default;

    Display(int x, int y, int width, int height, const std::string &name, const std::shared_ptr<Texture> &texture) :
        x(x),
        y(y),
        width(width),
        height(height),
        name(name),
        texture(texture)
    {
        this->buffer = std::shared_ptr<uint8_t[]>(new uint8_t[width * height * 4]);
    }
};

struct Texture
{
    int textureNum;
    int textureWidth;
    int textureHeight;
    std::shared_ptr<uint8_t[]> buffer;

    std::vector<std::shared_ptr<Display>> displays;
};

class DisplayManager : private UFC::Logger
{
    bool m_running = false;
    std::vector<std::shared_ptr<Texture>> m_textures;
    std::vector<std::shared_ptr<Display>> m_displays;
    float m_lastUpdate = 0.0f;

    static void copyDisplay(const std::shared_ptr<Texture> &texture, const std::shared_ptr<Display> &display);

    static int updateCallback(XPLMDrawingPhase inPhase, [[maybe_unused]] int inIsBefore, void *inRefcon);

    std::shared_ptr<Texture> checkTexture(int textureNum);
    void dumpTexture(int i, const char * name);

    void update();

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
