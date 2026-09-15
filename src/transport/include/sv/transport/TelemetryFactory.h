// Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Proprietary
// Smart Ventilator ICU Application

#pragma once

#ifndef SV_HAS_CANBUS
#define SV_HAS_CANBUS 1
#endif

#include <sv/transport/CanTelemetrySource.h>
#include <sv/transport/ITelemetrySource.h>
#include <sv/transport/UdpTelemetrySource.h>

#include <QString>

#include <memory>

namespace sv::transport {

/**
 * @brief Chooses the telemetry source for this run.
 *
 * Resolution order:
 *   1. SV_TELEMETRY=simulator forces the internal lung model and opens no
 *      link at all.
 *   2. SV_TELEMETRY=can forces CAN and fails loudly if it cannot open.
 *   3. Anything else, including nothing at all, opens the loopback frame
 *      link. It carries the same frames as CAN, so running
 *      tools/sim/ventilator_sim.py in another terminal is the whole setup.
 *      With no simulator answering, the link simply never comes up and the
 *      internal model runs, so the default costs a bound socket and nothing
 *      else.
 *
 * SV_CAN_PLUGIN and SV_CAN_INTERFACE override the plugin and interface,
 * which is how socketcan/can0 on the target becomes virtualcan/can0 on a
 * desktop without a rebuild. SV_UDP_LISTEN_PORT and SV_UDP_SEND_PORT do the
 * same for the loopback link.
 */
// Declared at namespace scope: a nested struct with default member
// initializers cannot be used as a default argument inside the class that
// encloses it, because the initializers are not complete until the class is.
struct TelemetryOptions
{
    bool allowSimulatorFallback = true;
    QString dbcPath = QStringLiteral(":/data/ventilator.dbc");
};

class TelemetryFactory
{
public:
    using Options = TelemetryOptions;

    static std::unique_ptr<ITelemetrySource> create(const Options &options = {},
                                                    QObject *parent = nullptr);

#if SV_HAS_CANBUS
    static CanTelemetrySource::Configuration canConfigurationFromEnvironment(
        const QString &dbcPath);
#endif

    /** @brief True when this build has the CAN link compiled in. */
    static constexpr bool canSupported() { return SV_HAS_CANBUS != 0; }

    /** @brief True when the environment or the build asks for the simulator. */
    static bool simulatorRequested();

    /** @brief True when the environment asks for the loopback frame link. */
    static bool udpRequested();

    /**
     * @brief True when an external device link was asked for.
     *
     * The in-process lung model is not one. It runs inside the application,
     * so bridging it to the controller would replace one internal model with
     * another and arm a heartbeat watchdog over a heartbeat the application
     * generates itself.
     */
    static bool deviceLinkRequested();

    static UdpTelemetrySource::Endpoint udpEndpointFromEnvironment();
};

} // namespace sv::transport
