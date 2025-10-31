#include "cutecosmiciconengine.h"

#include <QtGui/private/qguiapplication_p.h>

#include <QBuffer>
#include <QFile>
#include <QGuiApplication>
#include <QImageReader>
#include <QPainter>
#include <QPalette>
#include <QPixmapCache>
#include <QTextStream>
#include <QXmlStreamReader>

using namespace Qt::Literals;

CuteCosmicIconEngine::CuteCosmicIconEngine(const QString& iconName)
    : d_iconInfo(QIconLoader::instance()->loadIcon(iconName))
{
}

QIconEngine* CuteCosmicIconEngine::clone() const
{
    return new CuteCosmicIconEngine(d_iconInfo.iconName);
}

QString CuteCosmicIconEngine::key() const
{
    return "CuteCosmicIconEngine"_L1;
}

QString CuteCosmicIconEngine::iconName()
{
    return d_iconInfo.iconName;
}

bool CuteCosmicIconEngine::isNull()
{
    return d_iconInfo.entries.empty();
}

void CuteCosmicIconEngine::paint(QPainter* painter, const QRect& rect, QIcon::Mode mode, QIcon::State state)
{
    qreal scale = (painter->device()) ? painter->device()->devicePixelRatio() : qGuiApp->devicePixelRatio();

    QPixmap pixmap = scaledPixmap(rect.size(), mode, state, scale);
    if (!pixmap.isNull()) {
        painter->drawPixmap(rect, pixmap);
    }
}

QPixmap CuteCosmicIconEngine::pixmap(const QSize& size, QIcon::Mode mode, QIcon::State state)
{
    return scaledPixmap(size, mode, state, 1.0);
}

QPixmap CuteCosmicIconEngine::scaledPixmap(const QSize& size, QIcon::Mode mode, QIcon::State state, qreal scale)
{
    int iconSize = qMin(size.height(), size.width());

    QString fileName = bestIconFileForSize(iconSize, scale);
    if (fileName.isEmpty()) {
        return QPixmap();
    }

    QSize targetSize { iconSize, iconSize };

    if (!fileName.endsWith(".svg"_L1)) {
        QIcon icon { fileName };
        return icon.pixmap(targetSize, scale, mode, state);
    }

    QPixmap result = renderSvgIcon(fileName, targetSize * scale, mode, state);
    result.setDevicePixelRatio(scale);
    return result;
}

static bool isSymbolic(const QString& iconName)
{
    return iconName.endsWith("-symbolic"_L1)
        || iconName.endsWith("-symbolic-rtl"_L1);
}

static bool isKdeSymbolicIcon(const QByteArray& svg)
{
    // Breeze icons are mostly symbolic, but not always found as such because
    // of the internal symlinks in the theme. Furthermore, the way that KDE
    // symbolic icons are re-colored is by a stylesheet embedded into the icon
    // itself, which is replaced by KIconLoader prior to rendering. Try to
    // identify this stylesheet.
    QXmlStreamReader reader { svg };

    while (!reader.atEnd()) {
        reader.readNext();
        if (reader.error() != QXmlStreamReader::NoError) {
            return false;
        }

        bool isBreezeStylesheet = reader.isStartElement()
            && reader.name() == "style"_L1
            && reader.attributes().value("type"_L1) == "text/css"_L1
            && reader.attributes().value("id"_L1) == "current-color-scheme"_L1;

        if (isBreezeStylesheet) {
            return true;
        }
    }
    return false;
}

static void recolorIcon(QImage& image, const QColor& color)
{
    QRgb c = color.rgba();

    for (int y = 0; y < image.height(); y++) {
        QRgb* line = reinterpret_cast<QRgb*>(image.scanLine(y));
        for (int x = 0; x < image.width(); x++) {
            QRgb& rgb = line[x];
            rgb = qRgba(qRed(c), qGreen(c), qBlue(c), qAlpha(rgb));
        }
    }
}

QPixmap CuteCosmicIconEngine::renderSvgIcon(const QString& path, const QSize& size, QIcon::Mode mode, QIcon::State state)
{
    Q_UNUSED(state);

    QPalette appPalette = QGuiApplication::palette();

    QString cacheKey;
    QTextStream(&cacheKey) << "$cutecosmic|"_L1
                           << path << "|"_L1
                           << mode << "|"_L1
                           << size.width() << "|"_L1
                           << appPalette.cacheKey();

    QPixmap result;
    if (QPixmapCache::find(cacheKey, &result)) {
        return result;
    }

    QFile file { path };
    if (!file.open(QFile::ReadOnly)) {
        return QPixmap();
    }

    QByteArray data = file.readAll();

    QBuffer buffer { &data };
    QImageReader reader { &buffer, "svg" };
    reader.setScaledSize(size);

    QImage image = reader.read().convertToFormat(QImage::Format_ARGB32);
    if (isSymbolic(d_iconInfo.iconName) || isKdeSymbolicIcon(data)) {
        // FIXME: Active icon tint should be HighlightedText really, but
        // currently hovered menu items are painted with the highlight
        // color but hovered toolbar items (and other buttons) don't, and
        // that looks bad. Revisit with button palettes.
        QColor tint = (mode == QIcon::Active)
            ? appPalette.color(QPalette::Active, QPalette::Text)
            : appPalette.color(QPalette::Active, QPalette::Text);

        recolorIcon(image, tint);
    }

    result = QGuiApplicationPrivate::instance()->applyQIconStyleHelper(mode, QPixmap::fromImage(image));
    QPixmapCache::insert(cacheKey, result);
    return result;
}

static qreal directorySizeDistance(QIconDirInfo& dir, int size, qreal scale)
{
    // This is basically DirectorySizeDistance as listed in the XDG Icon Theme
    // specification

    const qreal scaledSize = size * scale;

    if (dir.type == QIconDirInfo::Fixed) {
        return qAbs(dir.size * dir.scale - scaledSize);
    }
    if (dir.type == QIconDirInfo::Scalable) {
        if (scaledSize < dir.minSize * dir.scale) {
            return dir.minSize * dir.scale - scaledSize;
        }
        else if (scaledSize > dir.maxSize * dir.scale) {
            return scaledSize - dir.maxSize * dir.scale;
        }
        return 0;
    }
    if (dir.type == QIconDirInfo::Threshold) {
        const qreal minScaledSize = (dir.size - dir.threshold) * dir.scale;
        const qreal maxScaledSize = (dir.size + dir.threshold) * dir.scale;

        if (scaledSize < minScaledSize) {
            return minScaledSize - scaledSize;
        }
        else if (scaledSize > maxScaledSize) {
            return scaledSize - maxScaledSize;
        }
        return 0;
    }
    return std::numeric_limits<qreal>::max();
}

QString CuteCosmicIconEngine::bestIconFileForSize(int size, qreal scale)
{
    qreal minDistance = std::numeric_limits<qreal>::max();
    QString minDistancePath;

    for (const auto& entry : std::as_const(d_iconInfo.entries)) {
        qreal distance = directorySizeDistance(entry->dir, size, scale);
        if (qFuzzyIsNull(distance)) {
            // XDG spec says we have to use the first exactly matching icon
            return entry->filename;
        }

        if (distance < minDistance) {
            minDistance = distance;
            minDistancePath = entry->filename;
        }
    }
    return minDistancePath;
}
