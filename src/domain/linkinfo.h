#pragma once

#include <QString>
#include <QtGlobal>

struct Resolution2D {
  double x = 0.0;
  double y = 0.0;
};

struct Scale2D {
  double horizontal = 100.0;
  double vertical = 100.0;
};

struct FlipState {
  bool horizontal = false;
  bool vertical = false;
};

enum class LinkProcessState { Ready, Missing, Unsupported, Error };

struct LinkInfo {
  // Selección para procesamiento
  bool process = true;

  // Referencias internas de InDesign.
  // Nos permitirán identificar posteriormente tanto el enlace
  // como el elemento gráfico que lo contiene.
  qint64 indesignLinkId = 0;
  qint64 indesignPageItemId = 0;

  // Documento
  QString page;

  // Archivo
  QString fileName;
  QString filePath;
  QString fileType;
  qint64 fileSizeBytes = 0;

  // Color
  QString colorMode;
  QString iccProfile;

  // Resolución
  Resolution2D actualResolution;
  Resolution2D effectiveResolution;

  // Transformaciones aplicadas en InDesign
  Scale2D scale;
  double rotation = 0.0;
  FlipState flip;

  // Estado del enlace
  LinkProcessState state = LinkProcessState::Ready;

  // Información adicional en caso de que el enlace
  // no pueda procesarse normalmente.
  QString statusMessage;
};
