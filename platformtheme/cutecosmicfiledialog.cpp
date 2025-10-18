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
#include "cutecosmicfiledialog.h"

#include <QtGui/private/qguiapplication_p.h>

#if QT_VERSION >= QT_VERSION_CHECK(6, 9, 0)
#include <QtGui/private/qdesktopunixservices_p.h>
#else
#include <QtGui/private/qgenericunixservices_p.h>
typedef QGenericUnixServices QDesktopUnixServices;
#endif

#include <qpa/qplatformintegration.h>

#include <QDBusInterface>
#include <QDBusMetaType>
#include <QDBusReply>
#include <QEventLoop>
#include <QFileInfo>
#include <QMimeDatabase>
#include <QRegularExpression>
#include <QUuid>

using namespace Qt::StringLiterals;

/*
 * This file dialog integration is implemented through the XDG Desktop Portal
 * interface. While the ideal way to do it would be through the relevant API
 * in libcosmic (as we want to behave as any native COSMIC application would
 * do), that API is missing important functionality at the moment and is very
 * convoluted. As it ends up going through the portal anyway, it makes no sense
 * to suffer through writing FFI bindings for it right now. Revisit this in the
 * future.
 *
 * This implementation is somewhat simpler than the one in the generic XDG
 * portal platform theme in the Qt tree, as we can assume that COSMIC is the
 * native environment and there is no other native dialog integration to fall
 * back on. This means that if anything is wrong, we can just ask Qt to use
 * a non-native dialog by returning false from the show() method.
 */

CuteCosmicFileDialogFilter CuteCosmicFileDialogFilter::parseNameFilter(const QString& filter)
{
    QRegularExpression regex(QLatin1String { QPlatformFileDialogHelper::filterRegExp });

    QRegularExpressionMatch match = regex.match(filter);
    if (!match.hasMatch()) {
        return CuteCosmicFileDialogFilter {};
    }

    QString label = match.captured(1);
    QStringList patterns = match.captured(2).split(u' ', Qt::SkipEmptyParts);
    if (patterns.isEmpty()) {
        return CuteCosmicFileDialogFilter {};
    }

    QList<CuteCosmicFileDialogFilterPattern> pats;
    for (const QString& pattern : patterns) {
        pats.append(CuteCosmicFileDialogFilterPattern { false, pattern });
    }

    return CuteCosmicFileDialogFilter {
        label,
        pats,
        filter
    };
}

CuteCosmicFileDialogFilter CuteCosmicFileDialogFilter::parseMimeFilter(const QString& filter)
{
    QMimeDatabase mimeDatabase;

    QMimeType mimeType = mimeDatabase.mimeTypeForName(filter);
    if (!mimeType.isValid()) {
        return CuteCosmicFileDialogFilter {};
    }

    return CuteCosmicFileDialogFilter {
        mimeType.comment(),
        { CuteCosmicFileDialogFilterPattern { true, filter } },
        filter
    };
}

QDBusArgument& operator<<(QDBusArgument& arg, const CuteCosmicFileDialogFilterPattern& pattern)
{
    uint kind = (pattern.isMimeFilter ? 1u : 0u);

    arg.beginStructure();
    arg << kind << pattern.pattern;
    arg.endStructure();
    return arg;
}

const QDBusArgument& operator>>(const QDBusArgument& arg, CuteCosmicFileDialogFilterPattern& pattern)
{
    uint kind;
    QString pat;

    arg.beginStructure();
    arg >> kind;
    arg >> pat;
    arg.endStructure();

    pattern.isMimeFilter = (kind == 1);
    pattern.pattern = pat;
    return arg;
}

QDBusArgument& operator<<(QDBusArgument& arg, const CuteCosmicFileDialogFilter& filter)
{
    arg.beginStructure();
    arg << filter.label << filter.patterns;
    arg.endStructure();
    return arg;
}

const QDBusArgument& operator>>(const QDBusArgument& arg, CuteCosmicFileDialogFilter& filter)
{
    arg.beginStructure();
    arg >> filter.label >> filter.patterns;
    arg.endStructure();
    return arg;
}

CuteCosmicFileDialog::CuteCosmicFileDialog()
{
    qDBusRegisterMetaType<CuteCosmicFileDialogFilterPattern>();
    qDBusRegisterMetaType<CuteCosmicFileDialogFilter>();
    qDBusRegisterMetaType<QList<CuteCosmicFileDialogFilter>>();

    d_loop = new QEventLoop(this);
    connect(this, &QPlatformFileDialogHelper::accept, d_loop, &QEventLoop::quit);
    connect(this, &QPlatformFileDialogHelper::reject, d_loop, &QEventLoop::quit);
}

bool CuteCosmicFileDialog::defaultNameFilterDisables() const
{
    return false;
}

QUrl CuteCosmicFileDialog::directory() const
{
    return d_directory;
}

void CuteCosmicFileDialog::setDirectory(const QUrl& directory)
{
    d_directory = directory;
}

QList<QUrl> CuteCosmicFileDialog::selectedFiles() const
{
    return d_selectedFiles;
}

void CuteCosmicFileDialog::selectFile(const QUrl& filename)
{
    d_selectedFiles.append(filename);
}

void CuteCosmicFileDialog::setFilter()
{
}

QString CuteCosmicFileDialog::selectedNameFilter() const
{
    return d_selectedNameFilter;
}

void CuteCosmicFileDialog::selectNameFilter(const QString&)
{
    // Not supported
}

QString CuteCosmicFileDialog::selectedMimeTypeFilter() const
{
    return d_selectedMimeTypeFilter;
}

void CuteCosmicFileDialog::selectMimeTypeFilter(const QString&)
{
    // Not supported
}

QVariantMap CuteCosmicFileDialog::buildPortalOptions(Qt::WindowModality windowModality)
{
    QVariantMap portalOptions;

    // handle_token (s)
    QString token = QUuid::createUuid().toString(QUuid::WithoutBraces).remove(QLatin1Char('-'));
    portalOptions["handle_token"_L1] = "cc"_L1 + token;

    // accept_label (s)
    if (options()->isLabelExplicitlySet(QFileDialogOptions::Accept)) {
        portalOptions["accept_label"_L1] = options()->labelText(QFileDialogOptions::Accept);
    }

    // modal (b)
    portalOptions["modal"_L1] = windowModality != Qt::NonModal;

    if (options()->acceptMode() == QFileDialogOptions::AcceptOpen) {
        // multiple (b)
        portalOptions["multiple"_L1] = options()->fileMode() == QFileDialogOptions::ExistingFiles;

        // directory (b)
        bool directoryMode = options()->fileMode() == QFileDialogOptions::Directory
            || options()->fileMode() == QFileDialogOptions::DirectoryOnly;

        if (directoryMode) {
            static bool directoryChooserSupported = isDirectoryChooserSupported();
            if (!directoryChooserSupported) {
                return QVariantMap();
            }
        }
        portalOptions["directory"_L1] = directoryMode;
    }

    // filters (a(sa(us))
    const QStringList nameFilters = options()->nameFilters();
    for (const QString& filter : nameFilters) {
        CuteCosmicFileDialogFilter f = CuteCosmicFileDialogFilter::parseNameFilter(filter);
        if (!f.isValid()) {
            continue;
        }

        d_filters.append(f);
    }

    const QStringList mimeFilters = options()->mimeTypeFilters();
    for (const QString& filter : mimeFilters) {
        CuteCosmicFileDialogFilter f = CuteCosmicFileDialogFilter::parseMimeFilter(filter);
        if (!f.isValid()) {
            continue;
        }

        d_filters.append(f);
    }

    portalOptions["filters"_L1] = QVariant::fromValue(d_filters);

    // current_filter ((sa(us)))
    if (!options()->initiallySelectedNameFilter().isEmpty()) {
        CuteCosmicFileDialogFilter f = CuteCosmicFileDialogFilter::parseNameFilter(options()->initiallySelectedNameFilter());
        if (f.isValid()) {
            portalOptions["current_filter"_L1] = QVariant::fromValue(f);
        }
    }
    else if (!options()->initiallySelectedMimeTypeFilter().isEmpty()) {
        CuteCosmicFileDialogFilter f = CuteCosmicFileDialogFilter::parseMimeFilter(options()->initiallySelectedMimeTypeFilter());
        if (f.isValid()) {
            portalOptions["current_filter"_L1] = QVariant::fromValue(f);
        }
    }

    // current_folder (ay)
    if (!d_directory.isEmpty()) {
        QByteArray dir = QFile::encodeName(d_directory.toLocalFile()).append('\0');
        portalOptions["current_folder"_L1] = dir;
    }

    if (options()->acceptMode() == QFileDialogOptions::AcceptSave && !d_selectedFiles.isEmpty()) {
        QUrl file = d_selectedFiles.first();

        // current_file (ay) - if it exists
        // current_name (s) - if it doesn't yet exist and is only a suggestion
        QFileInfo info { file.toLocalFile() };
        if (info.exists()) {
            QByteArray path = QFile::encodeName(file.toLocalFile()).append('\0');
            portalOptions["current_file"_L1] = path;
        }
        else {
            portalOptions["current_name"_L1] = info.fileName();
        }
    }

    return portalOptions;
}

bool CuteCosmicFileDialog::show(Qt::WindowFlags windowFlags, Qt::WindowModality windowModality, QWindow* parent)
{
    Q_UNUSED(windowFlags);

    if (d_directory.isEmpty()) {
        setDirectory(options()->initialDirectory());
    }

    QVariantMap portalOptions = buildPortalOptions(windowModality);
    if (portalOptions.isEmpty()) {
        return false;
    }

    QString parentRef;
    if (parent) {
        auto* services = dynamic_cast<QDesktopUnixServices*>(QGuiApplicationPrivate::platformIntegration()->services());
        if (services) {
            parentRef = services->portalWindowIdentifier(parent);
        }
    }

    QDBusInterface iface {
        "org.freedesktop.portal.Desktop"_L1,
        "/org/freedesktop/portal/desktop"_L1,
        "org.freedesktop.portal.FileChooser"_L1
    };

    QDBusMessage message;
    if (options()->acceptMode() == QFileDialogOptions::AcceptOpen) {
        message = iface.call("OpenFile"_L1, parentRef, options()->windowTitle(), portalOptions);
    }
    else {
        message = iface.call("SaveFile"_L1, parentRef, options()->windowTitle(), portalOptions);
    }

    QDBusReply<QDBusObjectPath> reply = message;
    if (!reply.isValid()) {
        qWarning() << "Failed requesting file chooser via portal:" << reply.error();
        return false;
    }

    QDBusConnection::sessionBus().connect(
        "org.freedesktop.portal.Desktop"_L1,
        reply.value().path(),
        "org.freedesktop.portal.Request"_L1,
        "Response"_L1,
        this,
        SLOT(gotResponse(uint, QVariantMap)));

    return true;
}

void CuteCosmicFileDialog::exec()
{
    // This method must block until dialog is excused
    d_loop->exec();
}

void CuteCosmicFileDialog::hide()
{
}

void CuteCosmicFileDialog::gotResponse(uint response, const QVariantMap& results)
{
    if (response != 0) {
        Q_EMIT reject();
        return;
    }

    if (results.contains("uris"_L1)) {
        const QStringList selected = results.value("uris"_L1).toStringList();

        d_selectedFiles.clear();
        for (const QString& uri : selected) {
            d_selectedFiles.append(QUrl(uri));
        }
    }

    if (results.contains("current_filter"_L1)) {
        QVariant v = results.value("current_filter"_L1);
        const CuteCosmicFileDialogFilter selected = qdbus_cast<CuteCosmicFileDialogFilter>(v);

        for (const CuteCosmicFileDialogFilter& filter : std::as_const(d_filters)) {
            if (filter == selected) {
                if (filter.patterns.first().isMimeFilter) {
                    d_selectedMimeTypeFilter = filter.origFilter;
                }
                else {
                    d_selectedNameFilter = filter.origFilter;
                }
                break;
            }
        }
    }

    Q_EMIT accept();
}

bool CuteCosmicFileDialog::isDirectoryChooserSupported()
{
    QDBusInterface iface {
        "org.freedesktop.portal.Desktop"_L1,
        "/org/freedesktop/portal/desktop"_L1,
        "org.freedesktop.DBus.Properties"_L1,
    };

    QDBusReply<QVariant> reply = iface.call("Get"_L1, "org.freedesktop.portal.FileChooser"_L1, "version"_L1);
    if (reply.isValid()) {
        uint version = reply.value().toUInt();
        return version >= 3;
    }
    return false;
}

#include "moc_cutecosmicfiledialog.cpp"
