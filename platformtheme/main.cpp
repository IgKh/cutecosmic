#include "cutecosmictheme.h"

#include <qpa/qplatformthemeplugin.h>

class CuteCosmicPlatformThemePlugin : public QPlatformThemePlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QPlatformThemeFactoryInterface_iid FILE "cosmic.json")

public:
    QPlatformTheme* create(const QString& key, const QStringList& params) override;
};

QPlatformTheme* CuteCosmicPlatformThemePlugin::create(const QString& key, const QStringList& params)
{
    Q_UNUSED(params);

    if (key.compare(QLatin1String("cosmic"), Qt::CaseInsensitive) == 0) {
        return new CuteCosmicPlatformTheme();
    }
    return nullptr;
}

#include "main.moc"
