#pragma once

#include <vtkOpenGLActor.h>
#include <vtkNew.h>
#include <vtkSmartPointer.h>
#include <vtkSetGet.h>

#include <qsharedpointer.h>

#include "ColormapPreset.hpp"

// Forward declarations
class vtkFloatArray;
class vtkPoints;
class vtkPolyData;
class vtkTrivialProducer;
class vtkPolyDataMapper;
class vtkOpenGLPolyDataMapper;
class vtkPlaneSource;
class vtkArrowSource;
class vtkGlyph3DMapper;
class vtkElevationFilter;
class vtkVertexGlyphFilter;
class vtkStatisticalOutlierRemoval;

class DepthFilter;

namespace copick3d 
{
    class Frame;
}

namespace copick3d::qtgui::graphics
{

class PointCloudActor : public vtkOpenGLActor
{
public:
    static PointCloudActor* New();
    vtkTypeMacro(PointCloudActor, vtkOpenGLActor);
    void PrintSelf(ostream& os, vtkIndent indent) override;

    void SetFrame(QSharedPointer<copick3d::Frame> frame);
    QSharedPointer<copick3d::Frame> GetFrame() const { return this->FramePtr; }

    vtkGetMacro(FixedPointSize, bool);
    vtkSetMacro(FixedPointSize, bool);

    enum PointColorMode
    {
        POINT_COLOR_MODE_RGB = 0,
        POINT_COLOR_MODE_NORMAL = 1,
        POINT_COLOR_MODE_DEPTH = 2,
        POINT_COLOR_MODE_SOLID = 3,
    };

    vtkGetMacro(ColorMode, int);
    vtkSetClampMacro(ColorMode, int, POINT_COLOR_MODE_RGB, POINT_COLOR_MODE_SOLID);

    enum PointColorMap
    { 
        POINT_COLOR_MAP_Gray = ColormapPreset::Gray,
        POINT_COLOR_MAP_Plasma = ColormapPreset::Plasma,
        POINT_COLOR_MAP_Viridis = ColormapPreset::Viridis,
        POINT_COLOR_MAP_Twilight = ColormapPreset::Twilight,
        POINT_COLOR_MAP_Turbo = ColormapPreset::Turbo,
    };

    vtkGetMacro(ColorMap, int);
    vtkSetClampMacro(ColorMap, int, POINT_COLOR_MAP_Gray, POINT_COLOR_MAP_Turbo);

    vtkGetMacro(PointSize, double);
    vtkSetClampMacro(PointSize, double, 0.1, 5.0);

    int RenderOpaqueGeometry(vtkViewport*) override;
    int HasOpaqueGeometry() override { return 1; }

private:
    PointCloudActor();
    ~PointCloudActor() override;

    PointCloudActor(const PointCloudActor&) = delete;
    void operator=(const PointCloudActor&) = delete;

    void InitializePipeline();
    void UpdatePipeline();

private:
    vtkNew<vtkFloatArray> Positions;
    vtkNew<vtkFloatArray> Normals;
    vtkNew<vtkFloatArray> Colors;

    vtkNew<vtkPoints> Points;
    vtkNew<vtkPolyData> PolyData;
    vtkNew<vtkTrivialProducer> PolyDataProducer;

    vtkNew<vtkStatisticalOutlierRemoval> SORFilter;
    vtkNew<vtkElevationFilter> ElevFilter;
    vtkNew<DepthFilter> DepthFilter;

    vtkNew<vtkVertexGlyphFilter> VertexGlyphFilter;
    vtkNew<vtkOpenGLPolyDataMapper> VertexGlyphMapper;

    vtkNew<vtkPlaneSource> RectSource;
    vtkNew<vtkGlyph3DMapper> Glyph3DMapper;

    vtkTimeStamp BuildTime;

    QSharedPointer<copick3d::Frame> FramePtr;

    double PointScale = 1.0;
    double PointSize = 1.0;

    bool FixedPointSize = false;
    int ColorMode = POINT_COLOR_MODE_RGB;
    int ColorMap = POINT_COLOR_MAP_Gray;
};

} // namespace copick3d::qtgui::graphics
