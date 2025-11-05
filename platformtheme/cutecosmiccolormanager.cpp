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

#include <QCoreApplication>
#include <QDir>
#include <QLoggingCategory>
#include <QPalette>
#include <QSaveFile>
#include <QTemporaryFile>
#include <QTextStream>

using namespace Qt::StringLiterals;

Q_DECLARE_LOGGING_CATEGORY(lcCuteCosmic)

CuteCosmicColorManager::CuteCosmicColorManager(QObject* parent)
    : QObject(parent)
{
    QString templateName = "/cutecosmic_%1_XXXXXX.colors"_L1.arg(QCoreApplication::applicationName());

    d_kdeColorsFile = new QTemporaryFile(this);
    d_kdeColorsFile->setFileTemplate(QDir::tempPath() + templateName);
    if (!d_kdeColorsFile->open()) {
        qCWarning(lcCuteCosmic(),
            "Can't open temporary file for writing, KDE colorscheme won't be exported");
    }
    else {
        qCDebug(lcCuteCosmic(),
            "KDE color scheme will be written to %s",
            qPrintable(d_kdeColorsFile->fileName()));
    }
}

void CuteCosmicColorManager::reloadThemeColors()
{
    rebuildPalettes();
    rebuildKdeColors();
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

struct ColorConfigEntry
{
    const char* key;
    QColor color;
};

static QTextStream& operator<<(QTextStream& stream, const ColorConfigEntry& entry)
{
    stream << entry.key << "=" << entry.color.red() << "," << entry.color.green() << "," << entry.color.blue() << "\n";
    return stream;
}

void CuteCosmicColorManager::rebuildKdeColors()
{
    if (!d_kdeColorsFile->isOpen()) {
        return;
    }

    if (!libcosmic_theme_should_apply_colors()) {
        QCoreApplication* app = QCoreApplication::instance();
        if (app->property("KDE_COLOR_SCHEME_PATH").toString() == d_kdeColorsFile->fileName()) {
            app->setProperty("KDE_COLOR_SCHEME_PATH", QVariant());
        }
        return;
    }

    Q_ASSERT(d_systemPalette.get() != nullptr);

    CosmicExtendedPalette ep;
    libcosmic_theme_get_extended_palette(&ep);

    QColor window = d_systemPalette->color(QPalette::Active, QPalette::Window);
    QColor windowText = d_systemPalette->color(QPalette::Active, QPalette::WindowText);
    QColor base = d_systemPalette->color(QPalette::Active, QPalette::Base);
    QColor alternateBase = d_systemPalette->color(QPalette::Active, QPalette::AlternateBase);
    QColor text = d_systemPalette->color(QPalette::Active, QPalette::Text);
    QColor button = d_buttonPalette->color(QPalette::Active, QPalette::Button);
    QColor buttonText = d_buttonPalette->color(QPalette::Active, QPalette::ButtonText);
    QColor tooltip = d_systemPalette->color(QPalette::Active, QPalette::ToolTipBase);
    QColor tooltipText = d_systemPalette->color(QPalette::Active, QPalette::ToolTipText);
    QColor highlight = d_systemPalette->color(QPalette::Active, QPalette::Highlight);
    QColor highlightText = d_systemPalette->color(QPalette::Active, QPalette::HighlightedText);
    QColor placeholderText = d_systemPalette->color(QPalette::Active, QPalette::PlaceholderText);
    QColor link = d_systemPalette->color(QPalette::Active, QPalette::Link);
    QColor linkVisited = d_systemPalette->color(QPalette::Active, QPalette::LinkVisited);
    QColor negative = convertColor(ep.destructive);
    QColor neutral = convertColor(ep.warning);
    QColor positive = convertColor(ep.success);

    QSaveFile saveFile { d_kdeColorsFile->fileName() };
    if (!saveFile.open(QIODeviceBase::WriteOnly)) {
        return;
    }

    QTextStream stream { &saveFile };

    stream << "[Colors:Window]\n";
    stream << ColorConfigEntry { "BackgroundNormal", window };
    stream << ColorConfigEntry { "BackgroundAlternate", alternateBase };
    stream << ColorConfigEntry { "BackgroundActive", window };
    stream << ColorConfigEntry { "BackgroundLink", window };
    stream << ColorConfigEntry { "BackgroundVisited", window };
    stream << ColorConfigEntry { "BackgroundNegative", negative };
    stream << ColorConfigEntry { "BackgroundNeutral", neutral };
    stream << ColorConfigEntry { "BackgroundPositive", positive };
    stream << ColorConfigEntry { "ForegroundNormal", windowText };
    stream << ColorConfigEntry { "ForegroundInactive", placeholderText };
    stream << ColorConfigEntry { "ForegroundActive", highlight };
    stream << ColorConfigEntry { "ForegroundLink", link };
    stream << ColorConfigEntry { "ForegroundVisited", linkVisited };
    stream << ColorConfigEntry { "ForegroundNegative", negative };
    stream << ColorConfigEntry { "ForegroundNeutral", neutral };
    stream << ColorConfigEntry { "ForegroundPositive", positive };
    stream << ColorConfigEntry { "DecorationFocus", highlight };
    stream << ColorConfigEntry { "DecorationHover", highlight };
    stream << "\n";

    stream << "[Colors:View]\n";
    stream << ColorConfigEntry { "BackgroundNormal", base };
    stream << ColorConfigEntry { "BackgroundAlternate", alternateBase };
    stream << ColorConfigEntry { "BackgroundActive", base };
    stream << ColorConfigEntry { "BackgroundLink", base };
    stream << ColorConfigEntry { "BackgroundVisited", base };
    stream << ColorConfigEntry { "BackgroundNegative", negative };
    stream << ColorConfigEntry { "BackgroundNeutral", neutral };
    stream << ColorConfigEntry { "BackgroundPositive", positive };
    stream << ColorConfigEntry { "ForegroundNormal", text };
    stream << ColorConfigEntry { "ForegroundInactive", placeholderText };
    stream << ColorConfigEntry { "ForegroundActive", highlight };
    stream << ColorConfigEntry { "ForegroundLink", link };
    stream << ColorConfigEntry { "ForegroundVisited", linkVisited };
    stream << ColorConfigEntry { "ForegroundNegative", negative };
    stream << ColorConfigEntry { "ForegroundNeutral", neutral };
    stream << ColorConfigEntry { "ForegroundPositive", positive };
    stream << ColorConfigEntry { "DecorationFocus", highlight };
    stream << ColorConfigEntry { "DecorationHover", highlight };
    stream << "\n";

    stream << "[Colors:Button]\n";
    stream << ColorConfigEntry { "BackgroundNormal", button };
    stream << ColorConfigEntry { "BackgroundAlternate", button };
    stream << ColorConfigEntry { "BackgroundActive", button };
    stream << ColorConfigEntry { "BackgroundLink", button };
    stream << ColorConfigEntry { "BackgroundVisited", button };
    stream << ColorConfigEntry { "BackgroundNegative", negative };
    stream << ColorConfigEntry { "BackgroundNeutral", neutral };
    stream << ColorConfigEntry { "BackgroundPositive", positive };
    stream << ColorConfigEntry { "ForegroundNormal", buttonText };
    stream << ColorConfigEntry { "ForegroundInactive", buttonText };
    stream << ColorConfigEntry { "ForegroundActive", buttonText };
    stream << ColorConfigEntry { "ForegroundLink", link };
    stream << ColorConfigEntry { "ForegroundVisited", linkVisited };
    stream << ColorConfigEntry { "ForegroundNegative", negative };
    stream << ColorConfigEntry { "ForegroundNeutral", neutral };
    stream << ColorConfigEntry { "ForegroundPositive", positive };
    stream << ColorConfigEntry { "DecorationFocus", highlight };
    stream << ColorConfigEntry { "DecorationHover", highlight };
    stream << "\n";

    stream << "[Colors:Selection]\n";
    stream << ColorConfigEntry { "BackgroundNormal", highlight };
    stream << ColorConfigEntry { "BackgroundAlternate", highlight };
    stream << ColorConfigEntry { "BackgroundActive", highlight };
    stream << ColorConfigEntry { "BackgroundLink", highlight };
    stream << ColorConfigEntry { "BackgroundVisited", highlight };
    stream << ColorConfigEntry { "BackgroundNegative", negative };
    stream << ColorConfigEntry { "BackgroundNeutral", neutral };
    stream << ColorConfigEntry { "BackgroundPositive", positive };
    stream << ColorConfigEntry { "ForegroundNormal", highlightText };
    stream << ColorConfigEntry { "ForegroundInactive", highlightText };
    stream << ColorConfigEntry { "ForegroundActive", highlightText };
    stream << ColorConfigEntry { "ForegroundLink", highlightText };
    stream << ColorConfigEntry { "ForegroundVisited", highlightText };
    stream << ColorConfigEntry { "ForegroundNegative", negative };
    stream << ColorConfigEntry { "ForegroundNeutral", highlightText };
    stream << ColorConfigEntry { "ForegroundPositive", highlightText };
    stream << ColorConfigEntry { "DecorationFocus", highlight };
    stream << ColorConfigEntry { "DecorationHover", highlight };
    stream << "\n";

    stream << "[Colors:Tooltip]\n";
    stream << ColorConfigEntry { "BackgroundNormal", tooltip };
    stream << ColorConfigEntry { "BackgroundAlternate", tooltip };
    stream << ColorConfigEntry { "BackgroundActive", tooltip };
    stream << ColorConfigEntry { "BackgroundLink", tooltip };
    stream << ColorConfigEntry { "BackgroundVisited", tooltip };
    stream << ColorConfigEntry { "BackgroundNegative", negative };
    stream << ColorConfigEntry { "BackgroundNeutral", neutral };
    stream << ColorConfigEntry { "BackgroundPositive", positive };
    stream << ColorConfigEntry { "ForegroundNormal", tooltipText };
    stream << ColorConfigEntry { "ForegroundInactive", tooltipText };
    stream << ColorConfigEntry { "ForegroundActive", highlight };
    stream << ColorConfigEntry { "ForegroundLink", link };
    stream << ColorConfigEntry { "ForegroundVisited", linkVisited };
    stream << ColorConfigEntry { "ForegroundNegative", negative };
    stream << ColorConfigEntry { "ForegroundNeutral", neutral };
    stream << ColorConfigEntry { "ForegroundPositive", positive };
    stream << ColorConfigEntry { "DecorationFocus", highlight };
    stream << ColorConfigEntry { "DecorationHover", highlight };
    stream << "\n";

    stream << "[Colors:Header]\n";
    stream << ColorConfigEntry { "BackgroundNormal", window };
    stream << ColorConfigEntry { "BackgroundAlternate", alternateBase };
    stream << ColorConfigEntry { "BackgroundActive", window };
    stream << ColorConfigEntry { "BackgroundLink", window };
    stream << ColorConfigEntry { "BackgroundVisited", window };
    stream << ColorConfigEntry { "BackgroundNegative", negative };
    stream << ColorConfigEntry { "BackgroundNeutral", neutral };
    stream << ColorConfigEntry { "BackgroundPositive", positive };
    stream << ColorConfigEntry { "ForegroundNormal", windowText };
    stream << ColorConfigEntry { "ForegroundInactive", placeholderText };
    stream << ColorConfigEntry { "ForegroundActive", highlight };
    stream << ColorConfigEntry { "ForegroundLink", link };
    stream << ColorConfigEntry { "ForegroundVisited", linkVisited };
    stream << ColorConfigEntry { "ForegroundNegative", negative };
    stream << ColorConfigEntry { "ForegroundNeutral", neutral };
    stream << ColorConfigEntry { "ForegroundPositive", positive };
    stream << ColorConfigEntry { "DecorationFocus", highlight };
    stream << ColorConfigEntry { "DecorationHover", highlight };
    stream << "\n";

    if (saveFile.commit()) {
        QCoreApplication::instance()->setProperty("KDE_COLOR_SCHEME_PATH", d_kdeColorsFile->fileName());
    }
}

#include "moc_cutecosmiccolormanager.cpp"
