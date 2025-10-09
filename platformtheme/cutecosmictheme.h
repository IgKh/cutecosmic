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
#pragma once

#include <QObject>

#if QT_VERSION >= QT_VERSION_CHECK(6, 10, 0)
#include <QtGui/private/qgenericunixtheme_p.h>
#else
#include <QtGui/private/qgenericunixthemes_p.h>
#endif

#include <memory>

class CuteCosmicWatcher;

class CuteCosmicPlatformThemePrivate : public QObject
{
    Q_OBJECT

public:
    CuteCosmicPlatformThemePrivate();

    void reloadTheme();
    void setColorScheme(Qt::ColorScheme scheme);

private Q_SLOTS:
    void themeChanged();

private:
    CuteCosmicWatcher* d_watcher;

    Qt::ColorScheme d_requestedScheme;
};

class CuteCosmicPlatformTheme : public QGenericUnixTheme
{
public:
    CuteCosmicPlatformTheme();
    ~CuteCosmicPlatformTheme();

    Qt::ColorScheme colorScheme() const override;
    void requestColorScheme(Qt::ColorScheme scheme) override;

    const QPalette* palette(Palette type) const override;

#if QT_VERSION >= QT_VERSION_CHECK(6, 10, 0)
    Qt::ContrastPreference contrastPreference() const override;
#endif

private:
    std::unique_ptr<CuteCosmicPlatformThemePrivate> d_ptr;
};
