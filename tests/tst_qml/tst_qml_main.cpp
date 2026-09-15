// Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Proprietary
// Smart Ventilator ICU Application

#include <QtQuickTest>

#include <QQmlEngine>

class Setup : public QObject
{
    Q_OBJECT

public slots:
    void qmlEngineAvailable(QQmlEngine *engine)
    {
        // The UI is addressed through qrc:, so the resource root has to be on
        // the import path for the singletons in ui/Theme to resolve.
        engine->addImportPath(QStringLiteral("qrc:/"));
    }
};

QUICK_TEST_MAIN_WITH_SETUP(smart_ventilator_qml, Setup)

#include "tst_qml_main.moc"
