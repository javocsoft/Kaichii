// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QCoreApplication>

#include "qt_common/config/switch2_gpu_notice.h"

namespace QtCommon {

QString Switch2GpuNotice(const Settings::HostProfile& host) {
    const QString gpu = QString::fromStdString(host.gpu_name);
    QString text;
    switch (Settings::Switch2GpuConcernFor(host)) {
    case Settings::Switch2GpuConcern::None:
        return {};
    case Settings::Switch2GpuConcern::Software:
        text = QCoreApplication::translate(
                   "Switch2GpuNotice",
                   "%1 draws on the processor instead of a graphics card, which usually means the "
                   "graphics driver is missing or not working: every game runs very slowly, "
                   "Switch games too. Install or update the driver for your graphics card.")
                   .arg(gpu);
        break;
    case Settings::Switch2GpuConcern::Integrated:
        text = QCoreApplication::translate(
                   "Switch2GpuNotice",
                   "%1 is integrated graphics. Switch games run well on it, but Switch 2 games "
                   "ask much more of the graphics: they take long to load and usually run at a "
                   "few frames per second, even with the Performance preset. They need a "
                   "dedicated graphics card, such as an NVIDIA GeForce RTX or an AMD Radeon RX.")
                   .arg(gpu);
        break;
    case Settings::Switch2GpuConcern::Measured:
        text = QCoreApplication::translate(
                   "Switch2GpuNotice",
                   "%1 measured %2 % of the reference computer's graphics speed. Switch games run "
                   "well on it, but Switch 2 games need about half that computer's speed or "
                   "more: here they will likely run at a few frames per second, even with the "
                   "Performance preset.")
                   .arg(gpu)
                   .arg(host.measured_gpu_ratio * 100.0, 0, 'f', 0);
        break;
    }
    if (!host.other_dedicated_gpu_name.empty()) {
        text += QLatin1Char(' ') +
                QCoreApplication::translate("Switch2GpuNotice",
                                            "This computer also has %1: choose it under Configure "
                                            "> Graphics > Device to play Switch 2 games.")
                    .arg(QString::fromStdString(host.other_dedicated_gpu_name));
    }
    return text;
}

} // namespace QtCommon
