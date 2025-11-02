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
#include "cutecosmiccolormanager.h"

#include "bindings.h"

#include <QPalette>

CuteCosmicColorManager::CuteCosmicColorManager(QObject* parent)
    : QObject(parent)
{
}

void CuteCosmicColorManager::reloadThemeColors()
{
    rebuildPalettes();
}

static QColor convertColor(const CosmicColor& color)
{
    return QColor::fromRgba(qRgba(color.red, color.green, color.blue, color.alpha));
}

static QColor withAlpha(QColor c, qreal factor)
{
    c.setAlphaF(c.alphaF() * factor);
    return c;
}

static QColor alphaBlend(const QColor& fg, const QColor& bg)
{
    qreal alpha = fg.alphaF();

    int r = qRound(fg.red() * alpha + bg.red() * (1.0 - alpha));
    int g = qRound(fg.green() * alpha + bg.green() * (1.0 - alpha));
    int b = qRound(fg.blue() * alpha + bg.blue() * (1.0 - alpha));

    return qRgb(r, g, b);
}

struct BevelColors
{
    QColor light;
    QColor mid;
    QColor midLight;
    QColor dark;
};

static BevelColors generateBevelColors(const QColor& base)
{
    // Same definition as in Fusion palette
    QColor mid = base.darker(130);
    return BevelColors {
        base.lighter(150),
        mid,
        mid.lighter(110),
        base.darker(150)
    };
}

void CuteCosmicColorManager::rebuildPalettes()
{
    if (!libcosmic_theme_should_apply_colors()) {
        // NULL palette means that the active style dictates the colors
        d_systemPalette.reset();
        d_menuPalette.reset();
        d_buttonPalette.reset();
        return;
    }

    CosmicPalette p;
    libcosmic_theme_get_palette(&p);

    QColor window = convertColor(p.window);
    QColor windowText = convertColor(p.window_text);
    QColor disabledWindowText = convertColor(p.window_text_disabled);
    QColor windowComponent = convertColor(p.window_component);
    QColor background = convertColor(p.background);
    QColor text = convertColor(p.text);
    QColor disabledText = convertColor(p.text_disabled);
    QColor component = convertColor(p.component);
    QColor componentText = convertColor(p.component_text);
    QColor disabledComponentText = convertColor(p.component_text_disabled);
    QColor button = convertColor(p.button);
    QColor buttonText = convertColor(p.button_text);
    QColor disabledButtonText = convertColor(p.button_text_disabled);
    QColor accent = convertColor(p.accent);
    QColor accentText = convertColor(p.accent_text);
    QColor disabledAccent = convertColor(p.accent_disabled);
    QColor tooltip = convertColor(p.tooltip);

    BevelColors componentBevel = generateBevelColors(component);
    QColor placeholderText = withAlpha(text, 0.5);

    d_systemPalette = std::make_unique<QPalette>(
        windowText,
        component,
        componentBevel.light,
        componentBevel.dark,
        componentBevel.mid,
        text,
        componentBevel.light,
        background,
        window);

    d_systemPalette->setColor(QPalette::Midlight, componentBevel.midLight);
    d_systemPalette->setColor(QPalette::ToolTipBase, tooltip);
    d_systemPalette->setColor(QPalette::ToolTipText, windowText);
    d_systemPalette->setColor(QPalette::HighlightedText, accentText);
    d_systemPalette->setColor(QPalette::PlaceholderText, placeholderText);
    d_systemPalette->setColor(QPalette::Link, accent);

    d_systemPalette->setColor(QPalette::Disabled, QPalette::WindowText, disabledWindowText);
    d_systemPalette->setColor(QPalette::Disabled, QPalette::Text, disabledText);
    d_systemPalette->setColor(QPalette::Disabled, QPalette::ButtonText, disabledComponentText);

    d_systemPalette->setColor(QPalette::Active, QPalette::ButtonText, componentText);
    d_systemPalette->setColor(QPalette::Inactive, QPalette::ButtonText, componentText);

    d_systemPalette->setColor(QPalette::Active, QPalette::Highlight, accent);
    d_systemPalette->setColor(QPalette::Inactive, QPalette::Highlight, accent);
    d_systemPalette->setColor(QPalette::Disabled, QPalette::Highlight, disabledAccent);

    d_systemPalette->setColor(QPalette::Active, QPalette::Accent, accent);
    d_systemPalette->setColor(QPalette::Inactive, QPalette::Accent, accent);
    d_systemPalette->setColor(QPalette::Disabled, QPalette::Accent, disabledAccent);

    // Disabled menu items (and buttons in general) in libcomsic use the window
    // component disabled text on top of 50% alpha window component background.
    // Emulate this by alpha-blending the two.
    QColor menuDisabledText = alphaBlend(withAlpha(disabledWindowText, 0.5), windowComponent);

    d_menuPalette = std::make_unique<QPalette>(*d_systemPalette);
    d_menuPalette->setColor(QPalette::Disabled, QPalette::Text, menuDisabledText);
    d_menuPalette->setColor(QPalette::Disabled, QPalette::ButtonText, menuDisabledText);

    // Push button palette - need to alpha-blend the button background color over
    // the window background to imitate how libcosmic renders
    QColor buttonBackground = alphaBlend(button, window);
    QColor buttonDisabledText = alphaBlend(withAlpha(disabledButtonText, 0.5), buttonBackground);
    BevelColors buttonBevel = generateBevelColors(buttonBackground);

    d_buttonPalette = std::make_unique<QPalette>(*d_systemPalette);
    d_buttonPalette->setColor(QPalette::Button, buttonBackground);
    d_buttonPalette->setColor(QPalette::Light, buttonBevel.light);
    d_buttonPalette->setColor(QPalette::Mid, buttonBevel.mid);
    d_buttonPalette->setColor(QPalette::Midlight, buttonBevel.midLight);
    d_buttonPalette->setColor(QPalette::Dark, buttonBevel.dark);

    d_buttonPalette->setColor(QPalette::Active, QPalette::ButtonText, buttonText);
    d_buttonPalette->setColor(QPalette::Inactive, QPalette::ButtonText, buttonText);
    d_buttonPalette->setColor(QPalette::Disabled, QPalette::ButtonText, buttonDisabledText);
}

#include "moc_cutecosmiccolormanager.cpp"
