#include "util/battery/battery.h"

#include "moc_battery.cpp"

// Do not include platform-specific battery implementation unless we are built
// with battery support (__BATTERY__).
#ifdef __BATTERY__
#if defined(_WIN32)
#include "util/battery/batterywindows.h"
#elif defined(__APPLE__)
#include "util/battery/batterymac.h"
#else
#include "util/battery/batterylinux.h"
#endif
#endif
#include "util/math.h"

// interval (in ms) of the timer which calls update()
constexpr int kiUpdateInterval = 5000;

Battery::Battery(QObject* parent)
        : QObject(parent),
          m_chargingState(UNKNOWN),
          m_dPercentage(0.0),
          m_iMinutesLeft(0),
          m_timer(this) {
    connect(&m_timer, &GuiTickTimer::timeout, this, &Battery::update);
    m_timer.start(mixxx::Duration::fromMillis(kiUpdateInterval));
}

Battery* Battery::getBattery(QObject* parent) {
#ifdef __BATTERY__
#if defined(Q_OS_WIN)
    return new BatteryWindows(parent);
#elif defined(Q_OS_MAC)
    return new BatteryMac(parent);
#else
    return new BatteryLinux(parent);
#endif
#else
    Q_UNUSED(parent);
    return nullptr;
#endif
}

void Battery::update() {
    constexpr double kPercentageEpsilon = 0.1;
    double lastPercentage = m_dPercentage;
    int lastMinutesLeft = m_iMinutesLeft;
    ChargingState lastChargingState = m_chargingState;
    read();
    if (fabs(lastPercentage - m_dPercentage) > kPercentageEpsilon ||
        lastChargingState != m_chargingState ||
        lastMinutesLeft != m_iMinutesLeft) {
        emit stateChanged();
    }
}
