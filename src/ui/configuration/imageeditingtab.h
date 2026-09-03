#pragma once

#include <QWidget>

#include "../../domain/imageeditingsettings.h"

class QCheckBox;
class QComboBox;
class QRadioButton;
class QWidget;

class ImageEditingTab : public QWidget {
  Q_OBJECT

public:
  explicit ImageEditingTab(QWidget *parent = nullptr);

  ImageEditingSettings settings() const;
  void setSettings(const ImageEditingSettings &settings);

signals:
  void settingsChanged();

private:
  void setupUi();
  void setupConnections();

  void updateColorModeContols();
  void updateColorProfileControls();

  QString colorModeText(ColorMode mode) const;

private:
  QCheckBox *m_changeColorModeCheckBox = nullptr;

  QComboBox *m_sourceColorModeCombo = nullptr;
  QComboBox *m_destinationColorModeCombo = nullptr;

  QCheckBox *m_changeColorProfileCheckBox = nullptr;
  QComboBox *m_iccProfileCombo = nullptr;

  QCheckBox *m_removeHiddenLayersCheckBox = nullptr;
  QCheckBox *m_mergeVisibleLayersCheckBox = nullptr;
  QCheckBox *m_flattenImageCheckBox = nullptr;

  QRadioButton *m_keepAlphaChannelsRadio = nullptr;
  QRadioButton *m_removeAlphaChannelsRadio = nullptr;

  QWidget *m_colorModeControlsContainer = nullptr;
  QWidget *m_colorProfileControlsContainer = nullptr;

  bool m_updatingUi = false;
};
