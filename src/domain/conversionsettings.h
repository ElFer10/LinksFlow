#pragma once

#include <QList>
#include <QVariantMap>

enum class ImageFormat
{
    PSD,
    TIFF,
    JPEG,
    PNG,
    WebP,
    BMP
};

struct ConversionRule
{
    ImageFormat sourceFormat = ImageFormat::PSD;

    bool enabled = false;

    ImageFormat destinationFormat =
        ImageFormat::TIFF;

    QVariantMap options;
};

struct ConversionSettings
{
    bool enabled = false;

    QList<ConversionRule> rules;
};
