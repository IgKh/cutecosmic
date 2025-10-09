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
#include "cutecosmictheme.h"
#include "cutecosmicwatcher.h"

#include "bindings.h"

#include <qpa/qwindowsysteminterface.h>

CuteCosmicPlatformThemePrivate::CuteCosmicPlatformThemePrivate()
    : d_requestedScheme(Qt::ColorScheme::Unknown)
{
    d_watcher = new CuteCosmicWatcher(this);
    connect(d_watcher, &CuteCosmicWatcher::themeChanged, this, &CuteCosmicPlatformThemePrivate::themeChanged);

    reloadTheme();
}

void CuteCosmicPlatformThemePrivate::reloadTheme()
{
    switch (d_requestedScheme) {
    case Qt::ColorScheme::Dark:
        libcosmic_theme_load(CosmicThemeKind::Dark);
        break;
    case Qt::ColorScheme::Light:
        libcosmic_theme_load(CosmicThemeKind::Light);
        break;
    case Qt::ColorScheme::Unknown:
        libcosmic_theme_load(CosmicThemeKind::SystemPreference);
        break;
    }
}

void CuteCosmicPlatformThemePrivate::setColorScheme(Qt::ColorScheme scheme)
{
    d_requestedScheme = scheme;
    themeChanged();
}

void CuteCosmicPlatformThemePrivate::themeChanged()
{
    reloadTheme();
    QWindowSystemInterface::handleThemeChange();
}

CuteCosmicPlatformTheme::CuteCosmicPlatformTheme()
    : d_ptr(new CuteCosmicPlatformThemePrivate())
{
}

CuteCosmicPlatformTheme::~CuteCosmicPlatformTheme()
{
}

Qt::ColorScheme CuteCosmicPlatformTheme::colorScheme() const
{
    bool dark = libcosmic_theme_is_dark();
    return dark ? Qt::ColorScheme::Dark : Qt::ColorScheme::Light;
}

void CuteCosmicPlatformTheme::requestColorScheme(Qt::ColorScheme scheme)
{
    d_ptr->setColorScheme(scheme);
}

const QPalette* CuteCosmicPlatformTheme::palette(Palette type) const
{
    Q_UNUSED(type);

    // Crude workaround for now - QPlatformTheme::palette() caches the fusion
    // palette, and there is seemingly no good way to invalidate that on theme
    // change. As per QApplicationPrivate::basePalette(), platform theme palette
    // takes precedence over the default style, so for now just return NULL here
    // to let the active QStyle dictate the palette.
    //
    // In the future - build our own palette based on libcosmic theme.
    return nullptr;
}

#if QT_VERSION >= QT_VERSION_CHECK(6, 10, 0)

Qt::ContrastPreference CuteCosmicPlatformTheme::contrastPreference() const
{
    bool highContrast = libcosmic_theme_is_high_contrast();
    return highContrast ? Qt::ContrastPreference::HighContrast : Qt::ContrastPreference::NoPreference;
}

#endif

#include "moc_cutecosmictheme.cpp"
