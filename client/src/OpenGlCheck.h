#pragma once

#include <SFML/OpenGL.hpp>
#include <SFML/Window.hpp>

#pragma comment(lib, "opengl32.lib")

struct GlCaps {
    bool valid = false;
    std::string vendor;
    std::string renderer;
    std::string version;
    GLint maxTexture = 0;
    bool shadersSupported = false;
};

GlCaps probeGL()
{
    GlCaps caps;

    sf::Context context;

    const GLubyte* vendorPtr = glGetString(GL_VENDOR);
    const GLubyte* rendererPtr = glGetString(GL_RENDERER);
    const GLubyte* versionPtr = glGetString(GL_VERSION);

    caps.vendor = vendorPtr ? (const char*)vendorPtr : "unknown";
    caps.renderer = rendererPtr ? (const char*)rendererPtr : "unknown";
    caps.version = versionPtr ? (const char*)versionPtr : "unknown";

    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &caps.maxTexture);
    caps.shadersSupported = sf::Shader::isAvailable();

    caps.valid = true;
    return caps;
}

void showGlErrorMessage(const GlCaps& caps)
{
    std::string msg = "Your system does not meet the game's graphics requirements.\r\n\r\n";

    msg += "Detected OpenGL Information:\r\n";
    msg += "Vendor:   " + caps.vendor + "\r\n";
    msg += "Renderer: " + caps.renderer + "\r\n";
    msg += "Version:  " + caps.version + "\r\n";
    msg += "Max Texture Size: " + std::to_string(caps.maxTexture) + "\r\n";
    msg += "Shaders Supported: ";
    msg += caps.shadersSupported ? "Yes\r\n" : "No\r\n";

    msg += "\r\nDetected Problems:\r\n";

    bool anyIssue = false;

    if (caps.renderer.find("GDI Generic") != std::string::npos) {
        msg += " - Your system is using Microsoft GDI Generic (software OpenGL).\r\n";
        msg += "   This means your graphics driver is not loaded.\r\n";
        anyIssue = true;
    }

    if (caps.maxTexture < 1024) {
        msg += " - Maximum texture size is extremely small (" + std::to_string(caps.maxTexture) + ").\r\n";
        msg += "   Your GPU is not being used, or OpenGL is in fallback mode.\r\n";
        anyIssue = true;
    }

    if (!caps.shadersSupported) {
        msg += " - Shaders are not supported (OpenGL < 2.0 context).\r\n";
        anyIssue = true;
    }

    if (!anyIssue) {
        msg += " - Unknown issue. Your OpenGL driver might be corrupted.\r\n";
    }

    msg += "\r\nHow to fix this:\r\n";
    msg += " - Update/reinstall your GPU drivers.\r\n";
    msg += " - Make sure you are not running the game through Remote Desktop.\r\n";
    msg += " - Restart your PC.\r\n";

    MessageBoxA(nullptr, msg.c_str(), "Graphics Error", MB_ICONERROR | MB_OK);
}