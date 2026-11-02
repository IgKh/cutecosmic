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
#include <Union/PlatformPlugin.h>

class CuteCosmicUnionPlatform : public Union::PlatformPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.kde.union.PlatformPlugin" FILE "platform.json")

public:
    CuteCosmicUnionPlatform(QObject* parent = nullptr);

    QString defaultStyleName() override;
    QIcon platformIcon(const QString& name, const QColor& color) override;
};
