#pragma once

#include <QString>

enum class ColorMode
{
    RGB,
    CMYK,
    Grayscale
};

enum class AlphaChannelHandling
{
    Keep,
    Remove
};

struct ImageEditingSettings
{
    bool changeColorMode = false;

    ColorMode sourceColorMode = ColorMode::RGB;
    ColorMode destinationColorMode = ColorMode::CMYK;

    bool changeColorProfile = false;
    QString iccProfile;

    bool removeHiddenLayers = false;
    bool mergeVisibleLayers = false;
    bool flattenImage = false;

    AlphaChannelHandling alphaChannels = AlphaChannelHandling::Keep;
};
