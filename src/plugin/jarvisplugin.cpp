#include "jarvisplugin.h"
#include "jarvisbackend.h"

void JarvisPlugin::registerTypes(const char *uri)
{
    Q_ASSERT(QLatin1String(uri) == QLatin1String("org.kde.plasma.jarvis"));
    qmlRegisterSingletonType<JarvisBackend>(uri, 1, 0, "JarvisBackend",
        [](QQmlEngine *engine, QJSEngine *scriptEngine) -> QObject * {
            Q_UNUSED(scriptEngine)
            auto *backend = new JarvisBackend();
            engine->setObjectOwnership(backend, QQmlEngine::CppOwnership);
            return backend;
        });
}
