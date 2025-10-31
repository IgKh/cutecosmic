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

#include <QIconEngine>

#include <QtGui/private/qiconloader_p.h>

class CuteCosmicIconEngine : public QIconEngine
{
public:
    CuteCosmicIconEngine(const QString& iconName);

    QIconEngine* clone() const override;

    QString key() const override;
    QString iconName() override;
    bool isNull() override;

    void paint(QPainter* painter, const QRect& rect, QIcon::Mode mode, QIcon::State state) override;
    QPixmap pixmap(const QSize& size, QIcon::Mode mode, QIcon::State state) override;
    QPixmap scaledPixmap(const QSize& size, QIcon::Mode mode, QIcon::State state, qreal scale) override;

private:
    QPixmap renderSvgIcon(const QString& path, const QSize& size, QIcon::Mode mode, QIcon::State state);

    QString bestIconFileForSize(int size, qreal scale);

    QThemeIconInfo d_iconInfo;
};
