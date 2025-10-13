#pragma once

#include <QQuickVTKItem.h>
#include <QVTKRenderWindowAdapter.h>

#include <copick3d/copick3d_api.hpp>
#include "vtk/QQuickVTKItemEx.hpp"


namespace copick3d::qtgui::graphics
{

class PointCloudView : public QQuickVTKItemEx
{
    enum PointCloudColorMode
    {
        Texture = 0,
        Normal = 1,
        Depth = 2,
        SolidColor = 3,
    };

    enum PointCloudColorMap
    {
        Gray = 0,
        Warm = 1,
        Cool = 2,
        Jet = 3,
        Rainbow = 4,
    };

    Q_ENUM(PointCloudColorMode)
    Q_ENUM(PointCloudColorMap)

    Q_OBJECT
    Q_PROPERTY(QSharedPointer<copick3d::Frame> frame
        WRITE setFrame
        NOTIFY frameChanged)
    Q_PROPERTY(bool parallelProjection
        READ parallelProjection
        WRITE setParallelProjection
        NOTIFY parallelProjectionChanged)
    Q_PROPERTY(PointCloudColorMode colorMode 
        READ colorMode
        WRITE setColorMode
        NOTIFY colorModeChanged)
    Q_PROPERTY(float pointSize
        READ pointSize
        WRITE setPointSize
        NOTIFY pointSizeChanged)
    // Q_PROPERTY(PointCloudColorMap colorMap 
    //     READ colorMap
    //     WRITE setColorMap
    //     NOTIFY colorMapChanged)
    
public:
    PointCloudView(QQuickItem *parent = nullptr);

    vtkUserData initializeVTK(vtkRenderWindow *renderWindow) override;

    void setFrame(QSharedPointer<copick3d::Frame> frame);

    bool parallelProjection() const { return m_parallelProjection; }
    void setParallelProjection(bool enable);

    PointCloudColorMode colorMode() const { return m_colorMode; }
    void setColorMode(PointCloudColorMode mode);

    // PointCloudColorMap colorMap() const;
    // void setColorMap(PointCloudColorMap map);

    float pointSize() const { return m_pointSize; }
    void setPointSize(float size);

signals:
    void frameChanged(QSharedPointer<copick3d::Frame> frame);
    void parallelProjectionChanged(bool enable);
    void colorModeChanged(PointCloudColorMode mode);
    // void colorMapChanged(PointCloudColorMap map);
    void pointSizeChanged(float size);

private:
    void updateColorModeImpl(vtkRenderWindow* renderWindow, vtkUserData userData);

private:
    QSharedPointer<copick3d::Frame> m_frame = nullptr;
    bool m_parallelProjection = false;
    PointCloudColorMode m_colorMode = PointCloudColorMode::Texture;
    float m_pointSize = 1.0f;
};


} // namespace copick3d::qtgui::graphics
