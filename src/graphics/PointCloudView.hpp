#pragma once

#include <QQuickVTKItem.h>
#include <QVTKRenderWindowAdapter.h>

#include <copick3d/copick3d_api.hpp>
#include "vtk/QQuickVTKItemEx.hpp"


namespace copick3d::qtgui::graphics
{

class PointCloudView : public QQuickVTKItemEx
{
    enum PointCloudRenderType
    {
        Vertex = 0,
        Square = 1,
        // Arrow = 2,
    };

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

    Q_ENUM(PointCloudRenderType)
    Q_ENUM(PointCloudColorMode)
    Q_ENUM(PointCloudColorMap)

    Q_OBJECT
    Q_PROPERTY(QSharedPointer<copick3d::Frame> frame
        READ frame
        WRITE setFrame
        NOTIFY frameChanged)
    Q_PROPERTY(bool parallelProjection
        READ parallelProjection
        WRITE setParallelProjection
        NOTIFY parallelProjectionChanged)
    Q_PROPERTY(bool fixedPointSize
        READ fixedPointSize
        WRITE setFixedPointSize
        NOTIFY fixedPointSizeChanged)
    Q_PROPERTY(PointCloudColorMode colorMode 
        READ colorMode
        WRITE setColorMode
        NOTIFY colorModeChanged)
    Q_PROPERTY(float pointSize
        READ pointSize
        WRITE setPointSize
        NOTIFY pointSizeChanged)
    Q_PROPERTY(QColor backgroundColor
        READ backgroundColor
        WRITE setBackgroundColor
        NOTIFY backgroundColorChanged)
    Q_PROPERTY(bool axisGridVisible
        READ axisGridVisible
        WRITE setAxisGridVisible
        NOTIFY axisGridVisibleChanged)
    
    // Q_PROPERTY(PointCloudColorMap colorMap 
    //     READ colorMap
    //     WRITE setColorMap
    //     NOTIFY colorMapChanged)
    
    Q_PROPERTY(QVector3D pickedPoint
        READ pickedPoint
        NOTIFY pickedPointChanged)
    
public:
    PointCloudView(QQuickItem *parent = nullptr);

    vtkUserData initializeVTK(vtkRenderWindow *renderWindow) override;

    QSharedPointer<copick3d::Frame> frame() const { return m_frame; }
    void setFrame(QSharedPointer<copick3d::Frame> frame);

    bool parallelProjection() const { return m_parallelProjection; }
    void setParallelProjection(bool enable);

    bool fixedPointSize() const { return m_fixedPointSize; }
    void setFixedPointSize(bool enable);

    PointCloudColorMode colorMode() const { return m_colorMode; }
    void setColorMode(PointCloudColorMode mode);

    // PointCloudColorMap colorMap() const;
    // void setColorMap(PointCloudColorMap map);

    float pointSize() const { return m_pointSize; }
    void setPointSize(float size);

    QColor backgroundColor() const { return m_backgroundColor; }
    void setBackgroundColor(const QColor color);

    bool axisGridVisible() const { return m_axisGridVisible; }
    void setAxisGridVisible(bool visible);

    QVector3D pickedPoint() const { return m_pickedPoint; }

signals:
    void frameChanged(QSharedPointer<copick3d::Frame> frame);
    void parallelProjectionChanged(bool enable);
    void fixedPointSizeChanged(bool enable);
    void colorModeChanged(PointCloudColorMode mode);
    // void colorMapChanged(PointCloudColorMap map);
    void pointSizeChanged(float size);
    void backgroundColorChanged(const QColor color);
    void axisGridVisibleChanged(bool visible);
    
    void pickedPointChanged(QVector3D pickedPoint);

private:
    void updateColorModeImpl(vtkRenderWindow* renderWindow, vtkUserData userData);

private:
    QSharedPointer<copick3d::Frame> m_frame = nullptr;
    bool m_parallelProjection = false;
    bool m_fixedPointSize = false;
    PointCloudColorMode m_colorMode = PointCloudColorMode::Texture;
    float m_pointSize = 1.0f;
    QColor m_backgroundColor = QColor(25, 25, 25);
    bool m_axisGridVisible = true;

    QVector3D m_pickedPoint = QVector3D(0.0f, 0.0f, 0.0f);
};


} // namespace copick3d::qtgui::graphics
