// Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Proprietary
// Smart Ventilator ICU Application

#include "Application.h"

#include <sv/common/AppIdentity.h>

#include <QGuiApplication>
#include <QQuickStyle>
#include <QByteArray>

int main(int argc, char *argv[])
{
    qputenv("QT_QUICK_CONTROLS_STYLE", QByteArray("Basic"));
    QQuickStyle::setStyle(QStringLiteral("Basic"));

    QGuiApplication app(argc, argv);
    sv::common::applyApplicationIdentity();

    Application application;
    application.initialize();

    return app.exec();
}
