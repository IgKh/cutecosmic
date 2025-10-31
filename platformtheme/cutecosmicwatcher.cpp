/*
 * This file is part of CuteCosmic.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#include "cutecosmicwatcher.h"

#include "bindings.h"

#include <QTimer>

CuteCosmicWatcher::CuteCosmicWatcher(QObject* parent)
    : QObject(parent)
    , d_watcherToken(nullptr)
{
    d_themeChangedEmitTimer = new QTimer(this);
    d_themeChangedEmitTimer->setSingleShot(true);
    d_themeChangedEmitTimer->setInterval(50);
    d_themeChangedEmitTimer->callOnTimeout(this, &CuteCosmicWatcher::themeChanged);

    QTimer::singleShot(0, this, &CuteCosmicWatcher::startWatching);
}

CuteCosmicWatcher::~CuteCosmicWatcher()
{
    if (d_watcherToken) {
        libcosmic_watcher_stop(d_watcherToken);
        d_watcherToken = nullptr;
    }
}

void CuteCosmicWatcher::startWatching()
{
    if (d_watcherToken != nullptr) {
        return;
    }

    auto callback = [](void* data) {
        CuteCosmicWatcher* self = reinterpret_cast<CuteCosmicWatcher*>(data);
        QMetaObject::invokeMethod(self, &CuteCosmicWatcher::configurationChanged, Qt::QueuedConnection);
    };

    d_watcherToken = libcosmic_watcher_start(callback, reinterpret_cast<void*>(this));
}

void CuteCosmicWatcher::configurationChanged()
{
    if (!d_themeChangedEmitTimer->isActive()) {
        d_themeChangedEmitTimer->start();
    }
}

#include "moc_cutecosmicwatcher.cpp"
