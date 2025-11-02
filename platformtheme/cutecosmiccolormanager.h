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

class QPalette;

class CuteCosmicColorManager : QObject
{
    Q_OBJECT

public:
    CuteCosmicColorManager(QObject* parent = nullptr);

    void reloadThemeColors();

    const QPalette* systemPalette() const { return d_systemPalette.get(); }
    const QPalette* menuPalette() const { return d_menuPalette.get(); }
    const QPalette* buttonPalette() const { return d_buttonPalette.get(); }

private:
    void rebuildPalettes();

    std::unique_ptr<QPalette> d_systemPalette;
    std::unique_ptr<QPalette> d_menuPalette;
    std::unique_ptr<QPalette> d_buttonPalette;
};
