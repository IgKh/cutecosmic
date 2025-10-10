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

#include <qpa/qplatformdialoghelper.h>

class QEventLoop;

struct CuteCosmicFileDialogFilterPattern
{
    bool isMimeFilter;
    QString pattern;

    friend bool operator==(const CuteCosmicFileDialogFilterPattern& lhs, const CuteCosmicFileDialogFilterPattern& rhs) {
        return lhs.isMimeFilter == rhs.isMimeFilter && lhs.pattern == rhs.pattern;
    }
};
Q_DECLARE_METATYPE(CuteCosmicFileDialogFilterPattern)

struct CuteCosmicFileDialogFilter
{
    QString label;
    QList<CuteCosmicFileDialogFilterPattern> patterns;
    QString origFilter;

    bool isValid() const { return !patterns.isEmpty(); }

    friend bool operator==(const CuteCosmicFileDialogFilter& lhs, const CuteCosmicFileDialogFilter& rhs) {
        return lhs.label == rhs.label && lhs.patterns == rhs.patterns;
    }

    static CuteCosmicFileDialogFilter parseNameFilter(const QString& filter);
    static CuteCosmicFileDialogFilter parseMimeFilter(const QString& filter);
};
Q_DECLARE_METATYPE(CuteCosmicFileDialogFilter);

class CuteCosmicFileDialog : public QPlatformFileDialogHelper
{
    Q_OBJECT

public:
    CuteCosmicFileDialog();

    bool defaultNameFilterDisables() const override;
    QUrl directory() const override;
    void setDirectory(const QUrl& directory) override;
    QList<QUrl> selectedFiles() const override;
    void selectFile(const QUrl& filename) override;
    void setFilter() override;
    QString selectedNameFilter() const override;
    void selectNameFilter(const QString& filter) override;
    QString selectedMimeTypeFilter() const override;
    void selectMimeTypeFilter(const QString& filter) override;

    bool show(Qt::WindowFlags windowFlags, Qt::WindowModality windowModality, QWindow* parent) override;
    void exec() override;
    void hide() override;

private Q_SLOTS:
    void gotResponse(uint response, const QVariantMap& results);

private:
    QVariantMap buildPortalOptions(Qt::WindowModality windowModality);

    static bool isDirectoryChooserSupported();

    QEventLoop* d_loop;

    QUrl d_directory;
    QList<QUrl> d_selectedFiles;
    QList<CuteCosmicFileDialogFilter> d_filters;
    QString d_selectedNameFilter;
    QString d_selectedMimeTypeFilter;
};
