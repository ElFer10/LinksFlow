#pragma once

#include <QMetaType>


enum class ResolutionUnit
{
    Ppi,
    PixelsPerCm
};

enum class OptimizationMethod
{
    ScaleAndResample,
    ResampleOnly
};

struct ResolutionSettings
{
    ResolutionUnit unit = ResolutionUnit::Ppi;

    double colorResolution = 300.0;
    double monochromeResolution = 1200.0;

    OptimizationMethod optimizationMethod =
        OptimizationMethod::ScaleAndResample;

    bool cropToInDesignFrame = true;

    double safetyArea = 3.0;

    enum class SafetyAreaUnit
    {
        Millimeters,
        Centimeters,
        Inches
    };

    SafetyAreaUnit safetyAreaUnit = SafetyAreaUnit::Millimeters; };

Q_DECLARE_METATYPE(ResolutionUnit)
Q_DECLARE_METATYPE(OptimizationMethod)