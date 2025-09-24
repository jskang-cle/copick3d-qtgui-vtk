#include <vtkSMPTools.h>

#include <QtQml/QQmlApplicationEngine>

#include <QtQuick/QQuickWindow>
#include <QtQuick/QSGRenderNode>

#include <QtGui/QGuiApplication>
#include <QtGui/QSurfaceFormat>

#include <QQuickVTKItem.h>

#include "graphics/PointCloudView.hpp"
#include "graphics/PointCloudLoader.hpp"

int main(int argc, char* argv[])
{
  QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);
  QQuickWindow::setSceneGraphBackend("opengl");
  // QQuickVTKItem::setGraphicsApi();
  
  vtkSMPTools::SetBackend("sequential");
  vtkSMPTools::Initialize();

  qDebug() << "Using VTK SMP backend:" << vtkSMPTools::GetBackend();

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
