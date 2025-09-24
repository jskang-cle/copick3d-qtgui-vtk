#include "PointCloudLoader.hpp"

#include <QUrl>
#include <QDebug>

namespace copick3d::qtgui::graphics
{

PointCloudLoader::PointCloudLoader(QObject *parent)
    : QObject(parent)
{
}

void PointCloudLoader::loadPointCloud(const QString &filePath)
{
    qDebug() << "Loading point cloud from" << filePath;

    auto url = QUrl::fromUserInput(filePath);
    if (!url.isLocalFile())
    {
        qWarning() << "Only local file paths are supported.";
        return;
    }

    QString localFilePath = url.toLocalFile();

    QSharedPointer<copick3d::Frame> frame;
    try
    {
        frame = QSharedPointer<copick3d::Frame>::create(
            copick3d::Frame::LoadImageSet(localFilePath.toStdString()));
    }
    catch (const std::exception& e)
    {
        qDebug() << e.what();
        return;
    }
    
    m_frame = frame;
    emit frameChanged(m_frame);
}

QSharedPointer<copick3d::Frame> PointCloudLoader::frame() const
{
    return m_frame;
}

QString PointCloudLoader::filePath() const
{
    return m_filePath;
}

void PointCloudLoader::setFilePath(const QString &filePath)
{
    if (m_filePath == filePath)
        return;

    m_filePath = filePath;
    emit filePathChanged(m_filePath);

    loadPointCloud(m_filePath);
}

} // namespace copick3d::qtgui::graphics
