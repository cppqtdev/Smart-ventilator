#!/usr/bin/env python3
"""Checks QML references against the C++ objects they are bound to.

A QML binding onto a property that does not exist on the backing QObject
fails silently: the expression evaluates to undefined, the control renders
blank, and nothing is logged unless the engine happens to warn. That class of
mistake survives review and shows up as an empty tile in front of a user.

This maps each conventional QML property name (ventilatorData, alarmData, ...)
onto its C++ class, harvests the Q_PROPERTY, Q_INVOKABLE and public slot names
from that class's header, and reports references that match nothing.

It is name-based and therefore approximate - a local property that shadows a
data object is reported as a miss - so treat output as leads, not verdicts.
"""
from __future__ import annotations

import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# QML identifier -> header defining the object it is bound to.
BINDINGS = {
    'ventilatorData':     'src/controllers/VentilatorController.h',
    'ventilatorModel':    'src/controllers/VentilatorController.h',
    'ventilatorController': 'src/controllers/VentilatorController.h',
    'alarmData':          'src/controllers/AlarmController.h',
    'alarmModel':         'src/controllers/AlarmController.h',
    'alarmController':    'src/controllers/AlarmController.h',
    'patientData':        'src/controllers/PatientController.h',
    'patientModel':       'src/controllers/PatientController.h',
    'patientController':  'src/controllers/PatientController.h',
    'ventilationCatalog': 'src/controllers/VentilationCatalog.h',
    'monitoringPresenter': 'src/presentation/include/sv/presentation/MonitoringPresenter.h',
    'calibrationService': 'src/services/include/sv/services/CalibrationService.h',
    'calibrationModel':   'src/services/include/sv/services/CalibrationService.h',
    'presenter':          'src/presentation/include/sv/presentation/MonitoringPresenter.h',
    # Screens hold the catalogue in a local `catalog` property; including it
    # here is what actually exercises the mode/parameter bindings.
    'catalog':            'src/controllers/VentilationCatalog.h',
    'batteryData':        'src/backend/include/sv/backend/BatteryFacade.h',
    'clockData':          'src/controllers/ClockController.h',
    'clockController':    'src/controllers/ClockController.h',
    'eventData':          'src/controllers/EventController.h',
    'eventModel':         'src/controllers/EventController.h',
    'eventController':    'src/controllers/EventController.h',
    'userControllerData': 'src/controllers/UserController.h',
    'userController':     'src/controllers/UserController.h',
    'appSettingsData':    'src/core/AppSettings.h',
    'appSettings':        'src/core/AppSettings.h',
    'databaseData':       'src/core/DatabaseManager.h',
    'databaseManager':    'src/core/DatabaseManager.h',
}

# Names that exist on every QObject or QAbstractItemModel.
UNIVERSAL = {
    'objectName', 'destroyed', 'deleteLater', 'toString', 'count', 'index',
    'rowCount', 'columnCount', 'data', 'get', 'setProperty', 'property',
}


def api_of(header_path: str) -> set[str]:
    path = os.path.join(ROOT, header_path)
    if not os.path.isfile(path):
        return set()
    text = open(path, encoding='utf-8').read()
    names = set(re.findall(r'Q_PROPERTY\s*\(\s*[\w:<>,\s\*&]+?\s+(\w+)\s+READ', text))
    names |= set(re.findall(r'Q_INVOKABLE\s+(?:[\w:<>,\s\*&]+?\s+)?(\w+)\s*\(', text))
    # public slots are callable from QML too
    for block in re.findall(r'(?:public\s+slots|public\s+Q_SLOTS)\s*:(.*?)(?=\n\s*(?:signals|Q_SIGNALS|public|private|protected)\s*:|\Z)',
                            text, re.S):
        names |= set(re.findall(r'(?:[\w:<>,\s\*&]+?\s+)?(\w+)\s*\([^;]*\)\s*;', block))
    return names | UNIVERSAL


def main() -> int:
    apis = {name: api_of(header) for name, header in BINDINGS.items()}
    missing = {name for name, api in apis.items() if len(api) <= len(UNIVERSAL)}
    for name in sorted(missing):
        print('note: could not read the API for %s (%s)' % (name, BINDINGS[name]))

    ref = re.compile(r'\b(' + '|'.join(BINDINGS) + r')\.(\w+)')
    problems = []

    for base, dirs, files in os.walk(ROOT):
        dirs[:] = [d for d in dirs if d not in {'_legacy', 'build', '.git', 'node_modules'}]
        for f in files:
            if not f.endswith('.qml'):
                continue
            path = os.path.join(base, f)
            for n, raw in enumerate(open(path, encoding='utf-8'), 1):
                line = raw.split('//')[0]
                for obj, member in ref.findall(line):
                    api = apis.get(obj, set())
                    if not api or obj in missing:
                        continue
                    if member not in api:
                        problems.append('%s:%d  %s.%s is not a property, invokable '
                                        'or slot of %s'
                                        % (os.path.relpath(path, ROOT), n, obj, member,
                                           os.path.basename(BINDINGS[obj])[:-2]))

    for p in problems:
        print(p)
    print('\n%d unresolved reference(s)' % len(problems))
    return 1 if problems else 0


if __name__ == '__main__':
    raise SystemExit(main())
