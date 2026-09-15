# Smart Ventilator - Software Architecture

> Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
> Proprietary and confidential. Unauthorized distribution prohibited.

## Overview

The Smart Ventilator application follows a strict layered architecture. Each layer
has a single responsibility, and dependencies only flow downward. The QML UI never
talks directly to hardware or data layers -- it goes through facade objects that
expose a clean, minimal API.

This design exists for two reasons: testability (every layer can be tested in
isolation with mocks) and regulatory traceability (IEC 62304 requires clear
software unit boundaries).

## Layer Diagram

```
+----------------------------------------------------------+
|                     QML / UI Layer                        |
|  (Screens, Controls, Theme, Animations)                  |
+----------------------------+-----------------------------+
                             |
                     Q_PROPERTY bindings
                             |
+----------------------------v-----------------------------+
|                    Facade Layer                           |
|  (VentilatorFacade, AlarmFacade, TrendFacade, etc.)      |
+----------------------------+-----------------------------+
                             |
                   signals / slots
                             |
+----------------------------v-----------------------------+
|                   Service Layer                           |
|  (VentilationService, AlarmService, WaveformService)     |
+----------------------------+-----------------------------+
                             |
                   abstract interfaces
                             |
+----------------------------v-----------------------------+
|                  Data / HAL Layer                         |
|  (SensorReader, ActuatorWriter, PatientRepository)       |
+----------------------------------------------------------+
```

## Facade Pattern

Each facade is a QObject that the QML engine consumes via context properties or
singletons. Facades hold no business logic. They translate between the service
layer (C++ signals/slots, domain types) and the UI layer (Q_PROPERTY, Q_INVOKABLE,
QML-friendly types).

```cpp
class VentilatorFacade : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(VentilatorFacade)

    Q_PROPERTY(double tidalVolume READ tidalVolume NOTIFY tidalVolumeChanged)
    Q_PROPERTY(int respiratoryRate READ respiratoryRate NOTIFY respiratoryRateChanged)

public:
    explicit VentilatorFacade(sv::VentilationService *service, QObject *parent = nullptr);

    double tidalVolume() const;
    int respiratoryRate() const;

    Q_INVOKABLE void setMode(const QString &mode);

signals:
    void tidalVolumeChanged();
    void respiratoryRateChanged();

private:
    sv::VentilationService *m_service = nullptr;
};
```

## Dependency Rule

Code in a given layer may only depend on the layer directly below it, never
sideways or upward. The service layer depends on abstract interfaces (pure virtual
classes) for hardware access, not concrete implementations. This lets us swap in
simulators during development and testing.

## Modules

| Module             | Purpose                                              |
|--------------------|------------------------------------------------------|
| `ventilation`      | Core breathing control: modes, parameters, waveforms |
| `alarm`            | Alarm detection, prioritization, escalation          |
| `monitoring`       | Real-time vitals display and waveform rendering      |
| `trends`           | Historical data storage, charting, export             |
| `patient`          | Patient demographics, session management             |
| `settings`         | Device configuration, calibration, preferences       |
| `hal`              | Hardware abstraction for sensors and actuators        |
| `common`           | Shared types, enums, utility functions               |

## Threading Model

The application uses three primary threads:

1. **Main / GUI thread** -- Runs the Qt event loop. All QML rendering, facade
   property updates, and user interaction handling happen here.

2. **Sensor thread** -- A dedicated QThread that polls hardware sensors at a
   fixed interval (typically 10 ms). Raw readings are packaged into value objects
   and delivered to the service layer via queued signal-slot connections.

3. **Alarm thread** -- Evaluates alarm conditions independently of the GUI
   refresh rate. This ensures alarm detection is never starved by heavy UI work.
   Alarm state changes are emitted as signals and picked up by the facade layer
   on the main thread.

All cross-thread communication uses Qt's queued connections. Shared state is
protected by QMutex or QReadWriteLock where direct access is unavoidable, but
the preference is always message-passing over shared memory.
