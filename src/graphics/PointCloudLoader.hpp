#pragma once

#include <QObject>
#include <QSharedPointer>

#include "copick3d/copick3d_api.hpp"

namespace copick3d::qtgui::graphics
{

class PointCloudLoader : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString filePath
        READ filePath WRITE setFilePath NOTIFY filePathChanged)
    Q_PROPERTY(QSharedPointer<copick3d::Frame> frame
        READ frame NOTIFY frameChanged)

public:
    explicit PointCloudLoader(QObject *parent = nullptr);
    void loadPointCloud(const QString &filePath);

    QSharedPointer<copick3d::Frame> frame() const;
    QString filePath() const;
    void setFilePath(const QString &filePath);

signals:
    void filePathChanged(const QString &filePath);
    void frameChanged(QSharedPointer<copick3d::Frame> frame);

private:
    QString m_filePath;
    QSharedPointer<copick3d::Frame> m_frame = nullptr;
};

} // namespace copick3d::qtgui::graphics
