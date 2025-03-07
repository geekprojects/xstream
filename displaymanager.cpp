//
// Created by Ian Parker on 06/03/2025.
//

#include "displaymanager.h"

#include <XPLMProcessing.h>
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
    shared_ptr<Texture> texture = nullptr;
    for (int i = 1; i < 1000; i++)
    {
        if (glIsTexture(i))
        {
            log(DEBUG, "findDisplay: %d: Found texture!");

            texture = checkTexture(i);
            if (texture != nullptr)
            {
                break;
            }
        }
    }
    glBindTexture(GL_TEXTURE_2D, 0);

    if (texture != nullptr)
    {
        log(DEBUG, "findDisplay: Adding display for texture %d\n", texture->textureNum);

        // PFD
        {
            auto display = make_shared<Display>(
                0,
                512,
                512,
                512,
                "pfd",
                texture);
            texture->displays.push_back(display);
            m_displays.push_back(display);
        }

        // ND
        {
            auto display = make_shared<Display>(
                512,
                512,
                512,
                512,
                "nd",
                texture);
            texture->displays.push_back(display);
            m_displays.push_back(display);
        }

        // ECAM
        {
            auto display = make_shared<Display>(
                1024,
                512,
                512,
                512,
                "ecam",
                texture
                );
            texture->displays.push_back(display);
            m_displays.push_back(display);
        }

        m_textures.push_back(texture);
    }

    return !m_textures.empty();
}

shared_ptr<Texture> DisplayManager::checkTexture(int textureNum)
{
    glBindTexture(GL_TEXTURE_2D, textureNum);

    GLint width;
    GLint height;
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &height);
    log(DEBUG, "findDisplay: Texture %d:  -> size=%d, %d", textureNum, width, height);

    if (width == 2048 && height == 2048)
    {
        const unique_ptr<uint8_t[]> data(new uint8_t[width * height * 4]);
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.get());
        log(DEBUG, "findDisplay: Texture %d:  -> %02x %02x %02x %02x", textureNum, data[0], data[1], data[2], data[3]);
        if (data[0] == 0 && data[1] == 0 && data[2] == 0 && data[3] == 0xff)
        {
            log(DEBUG, "findDisplay: Texture %d:  -> Texture matches pattern!", textureNum);
            auto texture = make_shared<Texture>();
            texture->textureNum = textureNum;
            texture->textureWidth = width;
            texture->textureHeight = height;
            texture->buffer = shared_ptr<uint8_t[]>(new uint8_t[width * height * 4]);
            return texture;
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
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture->buffer.get());

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
        memcpy(display->buffer.get() + dstPos, texture->buffer.get() + srcPos, dstStride);
        srcPos += srcStride;
        dstPos -= dstStride;
    }
}

void DisplayManager::dumpTextures()
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

void DisplayManager::dumpTexture(int i, const char* name)
{
    GLint width;
    GLint height;
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &height);
    log(DEBUG, "dumpTexture: %s %d:  -> size=%d, %d", name, i, width, height);

    if (width >= 2048 && height >= 2048)
    {
        unique_ptr<uint8_t[]> data(new uint8_t[width * height * 4]);
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.get());
        char filename[1024];
        snprintf(filename, 1024, "dump/texture_%s_%d.dat", name, i);
        FILE* fp = fopen(filename, "wb");
        if (fp == nullptr)
        {
            log(ERROR, "dumpTexture: Failed to open file %s", filename);
            return;
        }
        fwrite(data.get(), width * height * 4, 1, fp);
        fclose(fp);
    }
}
