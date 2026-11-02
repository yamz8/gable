// Gable — a fast native 3D viewer for IFC building models.

#include <QDir>
#include <QGuiApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QUrl>

#include "backend.h"
#include "cli.h"
#include "meshgeometry.h"
#include "theme.h"
#include "translator.h"

int main(int argc, char *argv[]) {
    // Commands draw no window, so they need no display: bare gable prints
    // help, as a command line tool should, and the launcher says gable open.
    // Anything that isn't a model counts as a command, so a typo gets an error
    // rather than a window.
    if (argc == 1 || isCliCommand(argv[1])) {
        QCoreApplication app(argc, argv);
        app.setApplicationName("gable");
        app.setApplicationVersion("0.1.0");
        return runCli(app.arguments());
    }

    QGuiApplication app(argc, argv);
    app.setApplicationName("gable");
    app.setApplicationVersion("0.1.0");

    // Matches the window to gable.desktop, so the compositor (Wayland
    // app_id) and the launcher pick up the installed icon.
    app.setDesktopFileName("gable");
    app.setWindowIcon(QIcon::fromTheme("gable"));

    QQuickStyle::setStyle("Material");

    // Installed before anything is translated, and every qsTr() binding is
    // re-evaluated when the Omarchy language changes.
    Translator translator;
    app.installTranslator(&translator);

    Theme theme;
    Backend backend;
    QObject::connect(&translator, &Translator::changed, &backend, &Backend::retranslate);

    qmlRegisterType<MeshGeometry>("Gable", 1, 0, "MeshGeometry");
    qmlRegisterUncreatableType<Backend>("Gable", 1, 0, "Backend", "Backend is provided as `backend`");

    QQmlApplicationEngine engine;
    QObject::connect(&translator, &Translator::changed, &engine, &QQmlEngine::retranslate);
    engine.rootContext()->setContextProperty("translator", &translator);
    engine.rootContext()->setContextProperty("theme", &theme);
    engine.rootContext()->setContextProperty("backend", &backend);
    engine.load(QUrl("qrc:/Main.qml"));
    if (engine.rootObjects().isEmpty())
        return 1;

    // `gable open model.ifc` (the launcher's form) and `gable model.ifc`
    // open it straight away.
    QStringList arguments = app.arguments().mid(1);
    if (arguments.value(0) == "open")
        arguments.removeFirst();
    for (const QString &argument : arguments) {
        if (argument.startsWith('-'))
            continue;
        const QUrl url = QUrl::fromUserInput(argument, QDir::currentPath(), QUrl::AssumeLocalFile);
        backend.openUrl(url);
        break;
    }

    return app.exec();
}
