//
// Created by Ian Parker on 06/03/2025.
//

#include "displaymanager.h"

#include <XPLMProcessing.h>
#include <XPLMDisplay.h>
#include <XPLMDataAccess.h>
#include <filesystem>

#include <yaml-cpp/yaml.h>

#include <fnmatch.h>

#include <png.h>

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

int DisplayManager::updateCallback([[maybe_unused]] XPLMDrawingPhase inPhase, [[maybe_unused]] int inIsBefore, void *inRefcon)
{
    auto displayManager = static_cast<DisplayManager*>(inRefcon);
    displayManager->update();
    return 0;
}

bool DisplayManager::start()
{
    if (m_running)
    {
        return true;
    }

    if (m_textures.empty())
    {
        if (!findDisplays())
        {
            return false;
        }
    }

    log(DEBUG, "startStream: Registering callback...");
    XPLMRegisterDrawCallback(updateCallback, xplm_Phase_Panel, 0, this);

    m_running = true;

    return true;
}

bool DisplayManager::stop()
{
    if (m_running)
    {
        log(DEBUG, "stop: Stopping...");
        m_running = false;
        XPLMUnregisterDrawCallback(updateCallback, xplm_Phase_Panel, 0, this);
    }
    return true;
}

bool DisplayManager::findDisplays()
{
    YAML::Node displayDef;
    if (!findDefinition(displayDef))
    {
        return false;
    }

    shared_ptr<Texture> texture = nullptr;
    YAML::Node textureNode;
    for (int i = 1; i < 1000; i++)
    {
        if (glIsTexture(i))
        {
            log(DEBUG, "findDisplay: %d: Found texture!");

            texture = checkTexture(displayDef, i, textureNode);
            if (texture != nullptr)
            {
                break;
            }
        }
    }
    glBindTexture(GL_TEXTURE_2D, 0);

    if (texture != nullptr)
    {
        log(DEBUG, "findDisplay: Adding display for texture %d", texture->textureNum);

        YAML::Node displaysNode = textureNode["displays"];
        for (auto displayNode : displaysNode)
        {
             auto display = make_shared<Display>(
                displayNode["x"].as<int>(),
                displayNode["y"].as<int>(),
                displayNode["width"].as<int>(),
                displayNode["height"].as<int>(),
                displayNode["name"].as<string>(),
                texture);
            texture->displays.push_back(display);
            m_displays.push_back(display);
        }
        m_textures.push_back(texture);
    }

    return !m_textures.empty();
}

std::string readString(const std::string& dataRefName)
{
    XPLMDataRef dataRef = XPLMFindDataRef(dataRefName.c_str());
    if (dataRef == nullptr)
    {
        return "";
    }

    // Get the length
    int bytes = XPLMGetDatab(dataRef, nullptr, 0, 0);

    // Read the value
    char buffer[4096];
    XPLMGetDatab(dataRef, buffer, 0, bytes);
    buffer[bytes] = '\0';

    return {buffer};
}

bool DisplayManager::findDefinition(YAML::Node& result)
{
    string aircraftAuthor = readString("sim/aircraft/view/acf_studio");
    if (aircraftAuthor.empty())
    {
        aircraftAuthor = readString("sim/aircraft/view/acf_author");
    }
    string aircraftICAO = readString("sim/aircraft/view/acf_ICAO");

    log(DEBUG, "findDefinition: Author: %s, ICAO type: %s", aircraftAuthor.c_str(), aircraftICAO.c_str());

    for (const auto & entry : filesystem::directory_iterator("Resources/plugins/xstream/data"))
    {
        if (entry.path().extension() == ".yaml")
        {
            auto aircraftFile = YAML::LoadFile(entry.path());
            if (!aircraftFile["author"] || !aircraftFile["icao"])
            {
                log(ERROR, "%s: Not a valid aircraft definition", entry.path().c_str());
                continue;
            }
            auto fileAuthor = aircraftFile["author"].as<string>();
            vector<string> fileICAOs;
            auto icaoNode = aircraftFile["icao"];
            if (icaoNode.IsScalar())
            {
                auto fileICAO = aircraftFile["icao"].as<string>();
                fileICAOs.push_back(fileICAO);
            }
            else
            {
                for (auto fileICAO : icaoNode)
                {
                    fileICAOs.push_back(fileICAO.as<string>());
                }
            }

            int match = fnmatch(fileAuthor.c_str(), aircraftAuthor.c_str(), 0);
            if (match != 0)
            {
                continue;
            }

            bool foundICAO = false;
            for (const auto& fileICAO : fileICAOs)
            {
                match = fnmatch(aircraftICAO.c_str(), fileICAO.c_str(), 0);
                if (match == 0)
                {
                    foundICAO = true;
                    break;
                }
            }

            if (!foundICAO)
            {
                continue;
            }

            log(INFO, "%s: Aircraft definition found!", entry.path().c_str());
            result = aircraftFile;
            return true;
        }
    }
    return false;
}

shared_ptr<Texture> DisplayManager::checkTexture(YAML::Node& displayDef, int textureNum, YAML::Node& matchingDef)
{
    glBindTexture(GL_TEXTURE_2D, textureNum);

    GLint width;
    GLint height;
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &height);
    log(DEBUG, "findDisplay: Texture %d:  -> size=%d, %d", textureNum, width, height);

    YAML::Node textures = displayDef["textures"];
    for (const YAML::Node& textureNode : textures)
    {
        int requiredWidth = textureNode["width"].as<int>();
        int requiredHeight = textureNode["height"].as<int>();
        auto requiredBytes = textureNode["bytes"].as<vector<int>>();
        if (width == requiredWidth && height == requiredHeight)
        {
            const unique_ptr<uint8_t[]> data(new uint8_t[width * height * 4]);
            glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.get());
            log(DEBUG, "findDisplay: Texture %d: Correct size. Checking bytes (%02x %02x %02x %02x)", textureNum, data[0], data[1], data[2], data[3]);

            bool bytesMatch = true;
            for (int i = 0; i < requiredBytes.size(); i++)
            {
                if (requiredBytes[i] != data[i])
                {
                    bytesMatch = false;
                    break;
                }
            }

            if (bytesMatch)
            {
                log(DEBUG, "findDisplay: Texture %d:  -> Texture matches pattern!", textureNum);
                auto texture = make_shared<Texture>();
                texture->textureNum = textureNum;
                texture->textureWidth = width;
                texture->textureHeight = height;
                texture->buffer = new uint8_t[width * height * 4];
                matchingDef = textureNode;
                return texture;
            }
        }
    }

    // Not the texture we're looking for!
    return nullptr;
}

void DisplayManager::update()
{
    if (!m_running)
    {
        return;
    }

    float now = XPLMGetElapsedTime();
    if (now - m_lastUpdate < 0.5f)
    {
        return;
    }
    m_lastUpdate = now;

    for (const auto& texture : m_textures)
    {
#ifdef DEBUG
        log(DEBUG, "updateDisplay: Texture: %d", texture->textureNum);
#endif
        glBindTexture(GL_TEXTURE_2D, texture->textureNum);
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture->buffer);

        // Slice the texture up in to the separate displays
        for (const auto& display : texture->displays)
        {
            copyDisplay(texture, display);
        }
    }
    glBindTexture(GL_TEXTURE_2D, 0);
}

void DisplayManager::copyDisplay(const shared_ptr<Texture>& texture, const shared_ptr<Display>& display)
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

void DisplayManager::dumpTextures()
{
    string icao = readString("sim/aircraft/view/acf_ICAO");
    for (int i = 0; i < 1000; i++)
    {
        if (glIsTexture(i))
        {
            glBindTexture(GL_TEXTURE_2D, i);
            dumpTexture(i, icao);
        }
    }
    glBindTexture(GL_TEXTURE_2D, 0);
}

void DisplayManager::dumpTexture(int i, std::string icao)
{
    GLint width;
    GLint height;
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &height);

    if (width >= 2048 && height >= 2048)
    {
        log(DEBUG, "dumpTexture: Texture %d: Size: %d, %d", i, width, height);

        unique_ptr<uint8_t[]> data(new uint8_t[width * height * 4]);
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.get());

        char filename[1024];
        snprintf(filename, 1024, "dump/texture_%s_%d.png", icao.c_str(), i);

        FILE* fp = fopen(filename, "wb");
        if (fp == nullptr)
        {
            log(ERROR, "dumpTexture: Failed to open file %s", filename);
            return;
        }
        png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
        png_infop info_ptr = png_create_info_struct(png_ptr);
        png_init_io(png_ptr, fp);

        // Output is 8bit depth, RGBA format.
        png_set_IHDR(
          png_ptr,
          info_ptr,
          width, height,
          8,
          PNG_COLOR_TYPE_RGBA,
          PNG_INTERLACE_NONE,
          PNG_COMPRESSION_TYPE_DEFAULT,
          PNG_FILTER_TYPE_DEFAULT
        );
        png_write_info(png_ptr, info_ptr);

        png_bytep row = data.get();
        for (int y = 0; y < height; y++)
        {
            png_write_row(png_ptr, row);
            row += width * 4;
        }
        png_write_end(png_ptr, nullptr);
        fclose(fp);

        snprintf(filename, 1024, "dump/texture_%s_%d.dat", icao.c_str(), i);
        fp = fopen(filename, "wb");
        if (fp == nullptr)
        {
            log(ERROR, "dumpTexture: Failed to open file %s", filename);
            return;
        }
        fwrite(data.get(), width * height * 4, 1, fp);
        fclose(fp);
    }
}
