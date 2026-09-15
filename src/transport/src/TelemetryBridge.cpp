// Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Proprietary
// Smart Ventilator ICU Application

#include <sv/transport/TelemetryBridge.h>

#include <sv/transport/ITelemetrySource.h>

#include "src/controllers/VentilatorController.h"

#include <sv/common/LogBuffer.h>

namespace sv::transport {

TelemetryBridge::TelemetryBridge(ITelemetrySource *source,
                                 VentilatorController *controller,
                                 QObject *parent)
    : QObject(parent)
    , m_source(source)
    , m_controller(controller)
{
    if (m_source == nullptr || m_controller == nullptr)
        return;

    connect(m_source, &ITelemetrySource::frameDecoded, this,
            [this](const QVariantMap &values, quint64) { onFrame(values); });
    connect(m_source, &ITelemetrySource::connectedChanged,
            this, &TelemetryBridge::onLinkChanged);
    connect(m_source, &ITelemetrySource::transportError,
            this, &TelemetryBridge::linkError);

    // Every accepted setting is republished as a whole frame. Sending only
    // the field that changed would let the device and the interface drift
    // apart after a dropped datagram.
    connect(m_controller, &VentilatorController::settingsChanged,
            this, &TelemetryBridge::publishSetpoints);

    // Only an operator action commands the device. adoptDeviceState() also
    // moves running, so the guard stops the interface echoing the device's
    // own state back at it as a command.
    connect(m_controller, &VentilatorController::runningChanged, this, [this]() {
        if (m_adopting)
            return;
        sendCommand(m_controller->running() ? StartVentilation : StopVentilation);
    });
}

bool TelemetryBridge::start()
{
    if (m_source == nullptr)
        return false;
    const bool opened = m_source->start();
    onLinkChanged();
    return opened;
}

void TelemetryBridge::stop()
{
    if (m_source != nullptr)
        m_source->stop();
    onLinkChanged();
}

bool TelemetryBridge::linkUp() const
{
    return m_source != nullptr && m_source->isConnected();
}

QString TelemetryBridge::descriptor() const
{
    return m_source != nullptr ? m_source->descriptor() : QString();
}

void TelemetryBridge::onFrame(const QVariantMap &values)
{
    m_adopting = true;
    m_controller->applyTelemetry(values);
    m_adopting = false;
}

void TelemetryBridge::onLinkChanged()
{
    const bool up = linkUp();
    if (up)
        m_everUp = true;

    m_controller->setHardwareBackend(up);

    // A link that has never carried a frame is not a link that dropped. The
    // interface is simply running on its own model, which is the normal
    // desktop case, and raising a disconnect for it would block the start on
    // a device that was never attached.
    if (m_everUp)
        m_controller->setBackendConnected(up);

    if (auto *log = sv::common::LogBuffer::instance()) {
        if (up) {
            log->note(sv::common::LogBuffer::Notice, QStringLiteral("sv.transport"),
                      tr("Device link up: %1").arg(descriptor()));
        } else if (m_everUp) {
            log->note(sv::common::LogBuffer::Warning, QStringLiteral("sv.transport"),
                      tr("Device link down"));
        }
    }

    // Setpoints are not pushed on reconnect. The device is the authority on
    // what it is delivering, and a freshly started interface would otherwise
    // overwrite a running therapy with its own defaults. Operator changes
    // still go out, through settingsChanged below.
    emit linkChanged();
}

void TelemetryBridge::publishSetpoints()
{
    if (m_source == nullptr || !m_source->isConnected())
        return;

    m_source->writeSetting(QString::fromLatin1(signals_::setFio2),
                           m_controller->fio2());
    m_source->writeSetting(QString::fromLatin1(signals_::setPeep),
                           m_controller->peep());
    m_source->writeSetting(QString::fromLatin1(signals_::setRate),
                           m_controller->respiratoryRate());
    m_source->writeSetting(QString::fromLatin1(signals_::setTidalVolume),
                           m_controller->tidalVolume());
    m_source->writeSetting(QString::fromLatin1(signals_::setPressureSupport),
                           m_controller->pressureSupport());
    m_source->writeSetting(QString::fromLatin1(signals_::setInspiratoryTime),
                           m_controller->inspiratoryTime());
    m_source->writeSetting(QString::fromLatin1(signals_::setTrigger),
                           m_controller->trigger());
}

void TelemetryBridge::sendCommand(Command code)
{
    if (m_source == nullptr || !m_source->isConnected())
        return;
    m_source->writeSetting(QString::fromLatin1(signals_::command), double(code));
}

} // namespace sv::transport
