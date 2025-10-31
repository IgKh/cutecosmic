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

#include <memory>

#if QT_VERSION >= QT_VERSION_CHECK(6, 10, 0)
#include <QtGui/private/qgenericunixtheme_p.h>
#else
#include <QtGui/private/qgenericunixthemes_p.h>
#endif

class CuteCosmicColorManager;
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
    friend class CuteCosmicPlatformTheme;

    CuteCosmicWatcher* d_watcher;
    CuteCosmicColorManager* d_colorManager;

    bool d_firstThemeChange;
    Qt::ColorScheme d_requestedScheme;
    std::unique_ptr<QFont> d_interfaceFont;
    std::unique_ptr<QFont> d_monospaceFont;
};

class CuteCosmicPlatformTheme : public QGenericUnixTheme
{
public:
    CuteCosmicPlatformTheme();
    ~CuteCosmicPlatformTheme();

    bool usePlatformNativeDialog(DialogType type) const override;
    QPlatformDialogHelper* createPlatformDialogHelper(DialogType type) const override;

    const QPalette* palette(Palette type) const override;
    const QFont* font(Font type) const override;

    QVariant themeHint(ThemeHint hint) const override;

    QIconEngine* createIconEngine(const QString& iconName) const override;

    Qt::ColorScheme colorScheme() const override;
    void requestColorScheme(Qt::ColorScheme scheme) override;

#if QT_VERSION >= QT_VERSION_CHECK(6, 10, 0)
    Qt::ContrastPreference contrastPreference() const override;
#endif

private:
    std::unique_ptr<CuteCosmicPlatformThemePrivate> d_ptr;
};
