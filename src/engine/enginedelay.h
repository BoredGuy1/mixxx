#pragma once

#include "engine/engineobject.h"

class ConfigKey;
class ControlPotmeter;
class ControlProxy;

class EngineDelay : public EngineObject {
    Q_OBJECT
  public:
    EngineDelay(const ConfigKey& delayControl, bool bPersist = true);
    virtual ~EngineDelay();

    void process(CSAMPLE* pInOut, const int iBufferSize);

    void setDelay(double newDelay);

  public slots:
    void slotDelayChanged();

  private:
    ControlPotmeter* m_pDelayPot;
    ControlProxy* m_pSampleRate;
    CSAMPLE* m_pDelayBuffer;
    int m_iDelayPos;
    int m_iDelay;
};
