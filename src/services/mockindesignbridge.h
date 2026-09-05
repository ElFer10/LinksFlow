#pragma once

#include "indesignbridge.h"

class MockInDesignBridge : public InDesignBridge {
  Q_OBJECT

public:
  explicit MockInDesignBridge(QObject *parent = nullptr);

  void analyzeActiveDocument() override;
};
