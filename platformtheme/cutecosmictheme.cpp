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
#include "cutecosmicfiledialog.h"
#include "cutecosmicwatcher.h"

#include "bindings.h"

#include <qpa/qwindowsysteminterface.h>

#include <QFont>

static QString consumeRustString(char* value)
{
    QString result = QString::fromUtf8(value, qstrlen(value));
    libcosmic_theme_free_string(value);
    return result;
}

static std::unique_ptr<QFont> loadFont(CosmicFontKind kind)
{
    CosmicFont fc;
    memset(&fc, 0, sizeof(CosmicFont));

    libcosmic_theme_get_font(kind, &fc);
    if (!fc.family) {
        return nullptr;
    }

    // Qt default font size is 9pt, while iced default font size is 14px (as
    // per cosmic::iced::Settings::default().default_text_size). The iced
    // default size is larger, and COSMIC has no setting for it. While using
    // that size makes text the same size it is in native COSMIC apps, without
    // the extra padding that is used even in Compact interface density - Qt
    // apps seem crowded, so best to keep with the smaller Qt default size.

    QString family = consumeRustString(fc.family);
    const int size = QGenericUnixTheme::defaultSystemFontSize;

    auto font = std::make_unique<QFont>(family, size);
    font->setWeight(static_cast<QFont::Weight>(fc.weight));
    font->setStretch(fc.stretch);

    switch (fc.style) {
    case CosmicFontStyle::Normal:
        font->setStyle(QFont::StyleNormal);
        break;
    case CosmicFontStyle::Italic:
        font->setStyle(QFont::StyleItalic);
        break;
    case CosmicFontStyle::Oblique:
        font->setStyle(QFont::StyleOblique);
        break;
    }

    if (kind == CosmicFontKind::Monospace) {
        font->setStyleHint(QFont::TypeWriter);
    }

    return font;
}

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

    d_interfaceFont = loadFont(CosmicFontKind::Interface);
    d_monospaceFont = loadFont(CosmicFontKind::Monospace);
}

void CuteCosmicPlatformThemePrivate::setColorScheme(Qt::ColorScheme scheme)
{
    if (d_requestedScheme == scheme) {
        return;
    }
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

bool CuteCosmicPlatformTheme::usePlatformNativeDialog(DialogType type) const
{
    if (type == DialogType::FileDialog) {
        return true;
    }
    return QGenericUnixTheme::usePlatformNativeDialog(type);
}

QPlatformDialogHelper* CuteCosmicPlatformTheme::createPlatformDialogHelper(DialogType type) const
{
    if (type == DialogType::FileDialog) {
        return new CuteCosmicFileDialog();
    }
    return QGenericUnixTheme::createPlatformDialogHelper(type);
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

const QFont* CuteCosmicPlatformTheme::font(Font type) const
{
    if (type == QPlatformTheme::SystemFont) {
        return d_ptr->d_interfaceFont.get();
    }
    if (type == QPlatformTheme::FixedFont) {
        return d_ptr->d_monospaceFont.get();
    }
    return nullptr;
}

QVariant CuteCosmicPlatformTheme::themeHint(ThemeHint hint) const
{
    if (hint == ThemeHint::SystemIconThemeName) {
        return consumeRustString(libcosmic_theme_icon_theme());
    }

    return QGenericUnixTheme::themeHint(hint);
}

#if QT_VERSION >= QT_VERSION_CHECK(6, 10, 0)

Qt::ContrastPreference CuteCosmicPlatformTheme::contrastPreference() const
{
    bool highContrast = libcosmic_theme_is_high_contrast();
    return highContrast ? Qt::ContrastPreference::HighContrast : Qt::ContrastPreference::NoPreference;
}

#endif

#include "moc_cutecosmictheme.cpp"
