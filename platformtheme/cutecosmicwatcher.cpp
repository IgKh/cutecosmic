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

#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>
#include <QTimer>

using namespace Qt::StringLiterals;

static constexpr QLatin1StringView THEME_MODE_CONFIG_ID = "com.system76.CosmicTheme.Mode"_L1;
static constexpr qulonglong THEME_MODE_CONFIG_VERSION = 1;

static constexpr QLatin1StringView THEME_DARK_CONFIG_ID = "com.system76.CosmicTheme.Dark"_L1;
static constexpr QLatin1StringView THEME_LIGHT_CONFIG_ID = "com.system76.CosmicTheme.Light"_L1;
static constexpr qulonglong THEME_CONFIG_VERSION = 1;

CuteCosmicWatcher::CuteCosmicWatcher(QObject* parent)
    : QObject(parent)
{
    watchConfigurationChanges(THEME_MODE_CONFIG_ID, THEME_MODE_CONFIG_VERSION);

    // Most theme related settings are maintained separately for dark and light
    // modes, so we need to watch both.
    watchConfigurationChanges(THEME_DARK_CONFIG_ID, THEME_CONFIG_VERSION);
    watchConfigurationChanges(THEME_LIGHT_CONFIG_ID, THEME_CONFIG_VERSION);

    d_themeChangedEmitTimer = new QTimer(this);
    d_themeChangedEmitTimer->setSingleShot(true);
    d_themeChangedEmitTimer->setInterval(50);
    d_themeChangedEmitTimer->callOnTimeout(this, &CuteCosmicWatcher::themeChanged);
}

void CuteCosmicWatcher::watchConfigurationChanges(const QString& id, qulonglong version)
{
    QDBusInterface iface {
        "com.system76.CosmicSettingsDaemon"_L1,
        "/com/system76/CosmicSettingsDaemon"_L1,
        "com.system76.CosmicSettingsDaemon"_L1
    };

    QDBusReply<QDBusObjectPath> reply = iface.call("WatchConfig"_L1, id, version);
    if (!reply.isValid()) {
        qWarning() << "Failed watching COSMIC configuration entry" << id << ": " << reply.error();
    }
    else {
        QDBusObjectPath path = reply.value();
        QString service = path.path().sliced(1).replace('/', '.');

        QDBusConnection::sessionBus().connect(
            service,
            path.path(),
            "com.system76.CosmicSettingsDaemon.Config"_L1,
            "Changed"_L1,
            this,
            SLOT(configurationChanged(QString, QString)));
    }
}

void CuteCosmicWatcher::configurationChanged(QString id, QString key)
{
    Q_UNUSED(id);
    Q_UNUSED(key);

    if (!d_themeChangedEmitTimer->isActive()) {
        d_themeChangedEmitTimer->start();
    }
}

#include "moc_cutecosmicwatcher.cpp"
