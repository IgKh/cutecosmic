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
#include <QPalette>

#if QT_VERSION >= QT_VERSION_CHECK(6, 10, 0)
static constexpr int DEFAULT_FONT_SIZE = QGenericUnixTheme::defaultSystemFontSize;
#else
// https://github.com/qt/qtbase/blob/6.9/src/gui/platform/unix/qgenericunixthemes.cpp#L78
static constexpr int DEFAULT_FONT_SIZE = 9;
#endif

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

    auto font = std::make_unique<QFont>(family, DEFAULT_FONT_SIZE);
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
    : d_firstThemeChange(true)
    , d_requestedScheme(Qt::ColorScheme::Unknown)
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

    rebuildPalette();

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
    // A little bit of a hack - libcosmic configuration subscriptions emit the
    // current value as soon as they are set up, and we don't need this. So
    // ignore the first theme change notification.
    if (d_firstThemeChange) {
        d_firstThemeChange = false;
        return;
    }

    reloadTheme();
    QWindowSystemInterface::handleThemeChange();
}

static QColor convertColor(const CosmicColor& color)
{
    return qRgba(color.red, color.green, color.blue, color.alpha);
}

void CuteCosmicPlatformThemePrivate::rebuildPalette()
{
    if (!libcosmic_theme_should_apply_colors()) {
        // NULL palette means that the active style dictates the colors
        d_systemPalette.reset();
        return;
    }

    CosmicPalette p;
    libcosmic_theme_get_palette(&p);

    QColor window = convertColor(p.window);
    QColor windowText = convertColor(p.window_text);
    QColor background = convertColor(p.background);
    QColor text = convertColor(p.text);
    QColor disabledBackground = convertColor(p.background_disabled);
    QColor disabledText = convertColor(p.text_disabled);
    QColor button = convertColor(p.button);
    QColor buttonText = convertColor(p.button_text);
    QColor accent = convertColor(p.accent);
    QColor accentText = convertColor(p.accent_text);
    QColor disabledAccent = convertColor(p.accent_disabled);
    QColor tooltip = convertColor(p.tooltip);

    // Bevel colors - same definition as in Fusion palette
    QColor light = button.lighter(150);
    QColor mid = button.darker(130);
    QColor midLight = mid.lighter(110);
    QColor dark = button.darker(150);

    QColor placeholderText = text;
    placeholderText.setAlphaF(0.5);

    d_systemPalette = std::make_unique<QPalette>(
        windowText, button, light, dark, mid, text, light, background, window);

    d_systemPalette->setColor(QPalette::Midlight, midLight);
    d_systemPalette->setColor(QPalette::ToolTipBase, tooltip);
    d_systemPalette->setColor(QPalette::ToolTipText, windowText);
    d_systemPalette->setColor(QPalette::HighlightedText, accentText);
    d_systemPalette->setColor(QPalette::PlaceholderText, placeholderText);
    d_systemPalette->setColor(QPalette::Link, accent);

    d_systemPalette->setColor(QPalette::Disabled, QPalette::Base, disabledBackground);
    d_systemPalette->setColor(QPalette::Disabled, QPalette::Text, disabledText);
    d_systemPalette->setColor(QPalette::Disabled, QPalette::ButtonText, disabledText);

    d_systemPalette->setColor(QPalette::Active, QPalette::ButtonText, buttonText);
    d_systemPalette->setColor(QPalette::Inactive, QPalette::ButtonText, buttonText);

    d_systemPalette->setColor(QPalette::Active, QPalette::Highlight, accent);
    d_systemPalette->setColor(QPalette::Inactive, QPalette::Highlight, accent);
    d_systemPalette->setColor(QPalette::Disabled, QPalette::Highlight, disabledAccent);

    d_systemPalette->setColor(QPalette::Active, QPalette::Accent, accent);
    d_systemPalette->setColor(QPalette::Inactive, QPalette::Accent, accent);
    d_systemPalette->setColor(QPalette::Disabled, QPalette::Accent, disabledAccent);
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
    if (type == QPlatformTheme::SystemPalette) {
        return d_ptr->d_systemPalette.get();
    }
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
    if (hint == QPlatformTheme::SystemIconThemeName) {
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
