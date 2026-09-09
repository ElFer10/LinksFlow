#pragma once

#include "conversionsettings.h"
#include "imageeditingsettings.h"
#include "imageusage.h"
#include "resolutionsettings.h"

#include <QList>
#include <QString>
#include <QVariantMap>

enum class ProcessingJobState
{
    Pending,
    Processing,
    Completed,
    Skipped,
    Failed
};

struct ProcessingJob
{
    QString id;

    // Archivo
    QString sourcePath;
    QString outputPath;

    ImageFormat sourceFormat = ImageFormat::PSD;
    ImageFormat targetFormat = ImageFormat::PSD;

    // Todas las colocaciones de este archivo dentro del documento de InDesign.
    QList<ImageUsage> usages;

    double minimumEffectiveResolutionX = 0.0;
    double minimumEffectiveResolutionY = 0.0;

    // Resolución
    bool resizeRequired = false;
    double targetResolution = 0.0;

    ResolutionUnit resolutionUnit = ResolutionUnit::Ppi;

    OptimizationMethod optimizationMethod = OptimizationMethod::ScaleAndResample;

    bool cropToInDesignFrame = false;

    double safetyArea = 0.0;

    SafetyAreaUnit safetyAreaUnit = SafetyAreaUnit::Millimeters;

    // Color
    bool colorModeConversionRequired = false;

    ColorMode sourceColorMode = ColorMode::RGB;

    ColorMode destinationColorMode = ColorMode::RGB;

    bool colorProfileConversionRequired = false;
    QString targetIccProfile;

    // Capas
    bool removeHiddenLayers = false;
    bool mergeVisibleLayers = false;
    bool flattenImage = false;

    // Canales alfa
    AlphaChannelHandling alphaChannels = AlphaChannelHandling::Keep;

    // Formato
    bool formatConversionRequired = false;
    QVariantMap formatOptions;

    // Estado
    ProcessingJobState state = ProcessingJobState::Pending;

    QString statusMessage;
};
