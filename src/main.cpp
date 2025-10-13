#include <vtkSMPTools.h>
#include <vtkLogger.h>

#include <QtQml/QQmlApplicationEngine>

#include <QtQuick/QQuickWindow>
#include <QtQuick/QSGRenderNode>

#include <QtGui/QGuiApplication>
#include <QtGui/QSurfaceFormat>

#include <QQuickVTKItem.h>

#include "graphics/PointCloudView.hpp"
#include "graphics/PointCloudLoader.hpp"

int main(int argc, char *argv[])
{
    qputenv("QSG_RENDER_LOOP", "basic");
    qputenv("QSG_NO_VSYNC", "1");

    QSurfaceFormat fmt = QVTKRenderWindowAdapter::defaultFormat(false);
    // By default QtQuick sets the alpha buffer size to 0. We follow the same thing here to prevent a
    // transparent background.
    fmt.setAlphaBufferSize(0);
    QSurfaceFormat::setDefaultFormat(fmt);
    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGLRhi);

    vtkSMPTools::SetBackend("STDthread");
    vtkSMPTools::Initialize();

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif

    QGuiApplication app(argc, argv);

    qmlRegisterType<copick3d::qtgui::graphics::PointCloudLoader>("copick3d.qtgui.graphics", 1, 0, "PointCloudLoader");
    qmlRegisterType<copick3d::qtgui::graphics::PointCloudView>("copick3d.qtgui.graphics", 1, 0, "PointCloudView");

    QQmlApplicationEngine engine;
    engine.load(QUrl(QStringLiteral("qrc:/main.qml")));
    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
