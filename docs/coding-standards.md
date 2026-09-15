# Smart Ventilator - Coding Standards

> Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
> Proprietary and confidential. Unauthorized distribution prohibited.

## C++ Conventions

- All project code lives under the `sv::` namespace (sub-namespaces like
  `sv::alarm`, `sv::hal` are fine).
- Member variables use the `m_` prefix: `m_tidalVolume`, `m_alarmLevel`.
- Every QObject subclass must include `Q_DISABLE_COPY_MOVE(ClassName)` right
  after the `Q_OBJECT` macro.
- Prefer smart pointers (`std::unique_ptr`, `std::shared_ptr`) for owning
  non-QObject resources. Let Qt's parent-child ownership handle QObject lifetimes.
- Use `enum class` over plain enums. Register them with `Q_ENUM` when QML needs
  access.
- Mark single-argument constructors `explicit`.
- Prefer `const` references for function parameters that are not primitives.
- Use `[[nodiscard]]` on functions where ignoring the return value is a bug.
- No raw `new` / `delete` outside of Qt's parent-child pattern.

## QML Conventions

- Use Theme tokens (`Theme.primaryColor`, `Theme.fontBody`) instead of literal
  colors or hardcoded font sizes. No magic hex values anywhere in QML.
- Controls (buttons, sliders, displays) must not import backend C++ modules
  directly. They receive data through properties bound from the parent screen.
- Screens are the only QML files that should reference facade objects.
- Prefer `Loader` for heavy sub-components that are not always visible.
- Keep QML files under 200 lines. Split into reusable components when they grow
  beyond that.

## Logging

- Use Qt's category-based logging: `Q_LOGGING_CATEGORY` and `qCInfo`, `qCWarning`,
  `qCCritical`.
- Never use bare `qDebug()` in committed code. It produces unfiltered output and
  cannot be routed or silenced in production.
- Each module defines its own logging category (e.g., `sv.alarm`, `sv.ventilation`).

## File Naming

- Headers: `PascalCase.h` (e.g., `VentilatorFacade.h`, `AlarmService.h`).
- Sources: matching `PascalCase.cpp`.
- QML files: `PascalCase.qml` (e.g., `HomeScreen.qml`, `WaveformChart.qml`).
- Test files: `tst_ClassName.cpp`.
- One class per file. Helper structs tightly coupled to a class may share its file.

## Comments

- Write code that explains itself. Use comments only when the *why* is not
  obvious from the code -- regulatory constraints, non-obvious performance
  choices, hardware quirks.
- Public API doc-comments use `///` (Doxygen-compatible) on facade and service
  classes.
- Do not leave commented-out code in the repository. Use version control instead.
