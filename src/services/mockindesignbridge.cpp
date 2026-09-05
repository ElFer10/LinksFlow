#include "mockindesignbridge.h"

#include <QTimer>

MockInDesignBridge::MockInDesignBridge(QObject *parent)
    : InDesignBridge(parent) {}

void MockInDesignBridge::analyzeActiveDocument() {
  emit analysisStarted();

  QTimer::singleShot(350, this, [this]() {
    QList<LinkInfo> links;

    LinkInfo link1;
    link1.indesignLinkId = 101;
    link1.indesignPageItemId = 1001;
    link1.page = QStringLiteral("1");
    link1.fileName = QStringLiteral("portada.psd");
    link1.filePath = QStringLiteral("/Proyecto/Links/portada.psd");
    link1.fileType = QStringLiteral("PSD");
    link1.fileSizeBytes = 25800000;
    link1.colorMode = QStringLiteral("CMYK");
    link1.iccProfile = QStringLiteral("Coated FOGRA39");
    link1.actualResolution = {300.0, 300.0};
    link1.effectiveResolution = {1130.0, 962.0};
    link1.scale = {26.55, 31.19};

    links.append(link1);

    LinkInfo link2;
    link2.indesignLinkId = 102;
    link2.indesignPageItemId = 1002;
    link2.page = QStringLiteral("2");
    link2.fileName = QStringLiteral("producto.tif");
    link2.filePath = QStringLiteral("/Proyecto/Links/producto.tif");
    link2.fileType = QStringLiteral("TIFF");
    link2.fileSizeBytes = 87500000;
    link2.colorMode = QStringLiteral("CMYK");
    link2.iccProfile = QStringLiteral("PSO Coated v3");
    link2.actualResolution = {300.0, 300.0};
    link2.effectiveResolution = {600.0, 600.0};
    link2.scale = {50.0, 50.0};

    links.append(link2);

    LinkInfo link3;
    link3.indesignLinkId = 103;
    link3.indesignPageItemId = 1003;
    link3.page = QStringLiteral("4");
    link3.fileName = QStringLiteral("fondo.jpg");
    link3.filePath = QStringLiteral("/Proyecto/Links/fondo.jpg");
    link3.fileType = QStringLiteral("JPEG");
    link3.fileSizeBytes = 6900000;
    link3.colorMode = QStringLiteral("RGB");
    link3.iccProfile = QStringLiteral("Adobe RGB (1998)");
    link3.actualResolution = {300.0, 300.0};
    link3.effectiveResolution = {240.0, 240.0};
    link3.scale = {125.0, 125.0};
    link3.rotation = 12.5;

    links.append(link3);

    LinkInfo link4;
    link4.process = false;
    link4.indesignLinkId = 104;
    link4.indesignPageItemId = 1004;
    link4.page = QStringLiteral("6");
    link4.fileName = QStringLiteral("missing.psd");
    link4.filePath = QStringLiteral("/Proyecto/Links/missing.psd");
    link4.fileType = QStringLiteral("PSD");
    link4.state = LinkProcessState::Missing;
    link4.statusMessage = QStringLiteral("Archivo no encontrado");

    links.append(link4);

    emit analysisCompleted(links);
  });
}
