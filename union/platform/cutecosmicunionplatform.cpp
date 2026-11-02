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
#include "cutecosmicunionplatform.h"

#include "cutecosmiciconengine.h"

#include <QGuiApplication>
#include <QIcon>

using namespace Qt::StringLiterals;

CuteCosmicUnionPlatform::CuteCosmicUnionPlatform(QObject* parent)
    : Union::PlatformPlugin(parent)
{
}

QString CuteCosmicUnionPlatform::defaultStyleName()
{
    return qGuiApp->layoutDirection() == Qt::RightToLeft
        ? "breeze-rtl"_L1
        : "breeze"_L1;
}

QIcon CuteCosmicUnionPlatform::platformIcon(const QString& name, const QColor& color)
{
    if (name.isEmpty()) {
        return Union::PlatformPlugin::platformIcon(name, color);
    }

    CuteCosmicIconEngine* engine = new CuteCosmicIconEngine(name, color);
    return QIcon(engine);
}
