#include "PointCloudView.hpp"

#include <vtkObject.h>
#include <vtkObjectFactory.h>
#include <vtkNew.h>

#include <vtkCamera.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkInteractorStyleTrackballCamera.h>

#include <vtkOrientationMarkerWidget.h>
#include <vtkCameraOrientationWidget.h>
#include <vtkCameraOrientationRepresentation.h>

#include <vtkPoints.h>
#include <vtkDataArray.h>
#include <vtkFloatArray.h>
#include <vtkArrayCalculator.h>
#include <vtkSmartPointer.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkProperty.h>

#include <vtkAxesActor.h>
#include <vtkCubeAxesActor.h>

#include <vtkStatisticalOutlierRemoval.h>
#include <vtkVertexGlyphFilter.h>
#include <vtkElevationFilter.h>
#include <vtkPolyDataMapper.h>

#include <vtkTextProperty.h>
#include <vtkShaderProperty.h>

#include <algorithm>
#include <vtkHardwarePicker.h>

namespace copick3d::qtgui::graphics
{

struct PointCloudViewData : vtkObject
{
    static PointCloudViewData* New();
    vtkTypeMacro(PointCloudViewData, vtkObject);

    copick3d::Frame frame;

    vtkNew<vtkRenderer> renderer;

    vtkNew<vtkCameraOrientationWidget> cameraWidget;
    vtkObject* orientationWidget;

    vtkNew<vtkPoints> points;
    vtkNew<vtkPolyData> polyData;

    vtkNew<vtkElevationFilter> elevFilter;
    vtkNew<vtkVertexGlyphFilter> glyphFilter;
    vtkNew<vtkPolyDataMapper> mapper;
    vtkNew<vtkActor> actor;

    vtkNew<vtkCubeAxesActor> cubeAxesActor;

    vtkNew<vtkFloatArray> positions;
    vtkNew<vtkFloatArray> normals;
    vtkNew<vtkFloatArray> colors;
};

vtkStandardNewMacro(PointCloudViewData);

PointCloudView::PointCloudView(QQuickItem *parent)
    : QQuickVTKItem(parent)
{
}

QQuickVTKItem::vtkUserData PointCloudView::initializeVTK(vtkRenderWindow *renderWindow)
{
    vtkNew<PointCloudViewData> data;
    data->renderer->SetBackground(0.1, 0.1, 0.1);

    renderWindow->AddRenderer(data->renderer);
    renderWindow->SetMultiSamples(8);
    qDebug() << "Mutlisamples:" << renderWindow->GetMultiSamples();

    vtkNew<vtkInteractorStyleTrackballCamera> style;
    style->SetDefaultRenderer(data->renderer);
    renderWindow->GetInteractor()->SetInteractorStyle(style);

    // data->frame = copick3d::Frame::LoadImageSet("D:\\data\\copick_images\\rack\\test_IMG_Texture_8Bit.png");
    // Vec3fArray points = data->frame.GetPointCloud()->Points();
    // Vec3fArray normals = data->frame.GetPointCloud()->Normals();
    // ColorBGRfArray colors = data->frame.GetPointCloud()->Colors();

    data->positions->SetNumberOfComponents(3);
    data->positions->SetNumberOfTuples(0);
    data->positions->SetName("Positions");
    // data->positions->SetVoidArray(points.Data(), points.Size() * 3, 1);

    data->normals->SetNumberOfComponents(3);
    data->normals->SetNumberOfTuples(0);
    data->normals->SetName("Normals");
    // data->normals->SetVoidArray(normals.Data(), normals.Size() * 3, 1);

    data->colors->SetNumberOfComponents(3);
    data->colors->SetNumberOfTuples(0);
    data->colors->SetName("Colors");
    // data->colors->SetNumberOfTuples(colors.Size());

    // float* colorsPtr = data->colors->GetPointer(0);
    
    // std::transform(
    //     colors.begin(), colors.end(), 
    //     (copick3d::Vec3f*)colorsPtr, 
    //     [](const copick3d::ColorBGRf& c) { return copick3d::Vec3f{c.r, c.g, c.b}; }
    // );

    data->points->SetData(data->positions);
    data->polyData->SetPoints(data->points);
    data->polyData->GetPointData()->SetNormals(data->normals);
    data->polyData->GetPointData()->SetScalars(data->colors);

// #if !defined(_DEBUG)
//     vtkNew<vtkStatisticalOutlierRemoval> sor;
//     sor->SetInputData(data->polyData);
//     sor->SetSampleSize(100);
//     sor->SetStandardDeviationFactor(1.0);
//     sor->Update();
//     data->polyData->DeepCopy(sor->GetOutput());
// #endif

    data->glyphFilter->SetInputData(data->polyData);
    data->mapper->SetInputConnection(data->glyphFilter->GetOutputPort());
    data->mapper->SetColorModeToDirectScalars();

    data->actor->SetMapper(data->mapper);
    data->actor->GetProperty()->SetPointSize(2);
    data->actor->GetProperty()->SetLighting(false);
    data->actor->GetProperty()->SetColor(1.0, 1.0, 1.0);

    auto shader = data->actor->GetShaderProperty();
    shader->AddVertexShaderReplacement(
        "//VTK::ValuePass::Impl",  // replace the normal block
        true,                      // before the standard replacements
        "gl_PointSize = clamp(500.0 / gl_Position.w, 1.0, 500.0);\n"
        "///VTK::ValuePass::Impl\n", // we still want the default
        false                        // only do it once
    );

    data->renderer->AddActor(data->actor);

    // auto cameraPosActor = vtkSmartPointer<vtkAxesActor>::New();
    // cameraPosActor->SetTotalLength(50.0, 50.0, 50.0);
    // cameraPosActor->AxisLabelsOff();
    // data->renderer->AddActor(cameraPosActor);

    // data->cubeAxesActor->SetBounds(data->polyData->GetBounds());
    data->cubeAxesActor->SetCamera(data->renderer->GetActiveCamera());
    data->cubeAxesActor->SetGridLineLocation(vtkCubeAxesActor::VTK_GRID_LINES_FURTHEST);
    data->cubeAxesActor->DrawXGridlinesOn();
    data->cubeAxesActor->DrawYGridlinesOn();
    data->cubeAxesActor->DrawZGridlinesOn();

    data->renderer->AddActor(data->cubeAxesActor);

    // data->renderer->ResetCamera();

    // auto rep = vtkSmartPointer<vtkAxesActor>::New();
    // data->orientationWidget->SetOrientationMarker(rep);
    // data->orientationWidget->SetInteractor(renderWindow->GetInteractor());
    // data->orientationWidget->SetEnabled(1);
    // data->orientationWidget->InteractiveOff();

    // auto orientationWidget = vtkOrientationMarkerWidget::New();
    // orientationWidget->SetOrientationMarker(vtkSmartPointer<vtkAxesActor>::New());
    // orientationWidget->SetInteractor(renderWindow->GetInteractor());
    // orientationWidget->SetEnabled(1);
    // orientationWidget->InteractiveOff();
    // data->orientationWidget = orientationWidget;

    // data->cameraWidget->SetParentRenderer(data->renderer);
    // data->cameraWidget->SetInteractor(renderWindow->GetInteractor());
    // data->cameraWidget->SetEnabled(1);
    // data->cameraWidget->AnimateOff();
    // data->cameraWidget->SetProcessEvents(false);

    auto cam = data->renderer->GetActiveCamera();
    cam->SetPosition(0, 0, 0);
    cam->SetViewUp(0, -1, 0);
    cam->SetFocalPoint(0, 0, 1000);
    cam->SetClippingRange(1.0, 2000.0);

    // vtkNew<vtkHardwarePicker> picker;
    // picker->SetSnapToMeshPoint(true);
    // renderWindow->GetInteractor()->SetPicker(picker);

    return data;
}

void PointCloudView::setFrame(QSharedPointer<copick3d::Frame> frame)
{
    if (frame == m_frame)
        return;

    m_frame = frame;
    Q_EMIT frameChanged(frame);

    dispatch_async([frame](vtkRenderWindow* renderWindow, vtkUserData userData)
    {
        qDebug() << "Setting frame in render thread:" << (frame ? frame->GetFrameIndex() : -1);

        auto data = PointCloudViewData::SafeDownCast(userData);
        if (!data)
            return;

        data->frame = *frame;

        Vec3fArray points = data->frame.GetPointCloud()->Points();
        Vec3fArray normals = data->frame.GetPointCloud()->Normals();
        ColorBGRfArray colors = data->frame.GetPointCloud()->Colors();

        data->positions->SetVoidArray(points.Data(), points.Size() * 3, 1);
        data->normals->SetVoidArray(normals.Data(), normals.Size() * 3, 1);

        if (data->colors->GetNumberOfTuples() != colors.Size())
            data->colors->SetNumberOfTuples(colors.Size());

        float* colorsPtr = data->colors->GetPointer(0);
        
        std::transform(
            colors.begin(), colors.end(), 
            (copick3d::Vec3f*)colorsPtr, 
            [](const copick3d::ColorBGRf& c) { return copick3d::Vec3f{c.r, c.g, c.b}; }
        );

        data->positions->Modified();
        data->normals->Modified();
        data->colors->Modified();

        data->cubeAxesActor->SetBounds(data->polyData->GetBounds());

        data->renderer->ResetCamera();

        qDebug() << "Point cloud updated. Num points:" << data->polyData->GetNumberOfPoints();
    });
}

void PointCloudView::setParallelProjection(bool enable)
{
    if (enable == m_parallelProjection)
        return;

    m_parallelProjection = enable;
    Q_EMIT parallelProjectionChanged(enable);

    dispatch_async([enable](vtkRenderWindow* renderWindow, vtkUserData userData)
    {
        auto data = PointCloudViewData::SafeDownCast(userData);
        if (!data)
            return;

        auto shader = data->actor->GetShaderProperty();
        if (!enable)
        {
            shader->AddVertexShaderReplacement(
                "//VTK::ValuePass::Impl",  // replace the normal block
                true,                      // before the standard replacements
                "gl_PointSize = clamp(500.0 / gl_Position.w, 1.0, 500.0);\n"
                "///VTK::ValuePass::Impl\n", // we still want the default
                false                        // only do it once
            );
        }
        else
        {
            shader->ClearAllVertexShaderReplacements();
        }

        data->renderer->GetActiveCamera()->SetParallelProjection(enable);
        renderWindow->Render();
    });
}

void PointCloudView::setColorMode(PointCloudColorMode mode)
{
    if (mode == m_colorMode)
        return;
    m_colorMode = mode;
    Q_EMIT colorModeChanged(mode);

    dispatch_async(std::bind(&PointCloudView::updateColorModeImpl, this, 
                             std::placeholders::_1, std::placeholders::_2));
}

void PointCloudView::updateColorModeImpl(vtkRenderWindow* renderWindow, vtkUserData userData)
{
    auto data = PointCloudViewData::SafeDownCast(userData);
    if (!data)
        return;

    switch (m_colorMode)
    {
    case PointCloudColorMode::Texture:
        data->glyphFilter->SetInputData(data->polyData);
        data->mapper->SetColorModeToDirectScalars();
        data->mapper->SetScalarModeToUsePointFieldData();
        data->mapper->SelectColorArray("Colors");
        data->mapper->SetScalarVisibility(1);
        break;
    case PointCloudColorMode::Normal:
        data->glyphFilter->SetInputData(data->polyData);
        data->mapper->SetColorModeToDirectScalars();
        data->mapper->SetScalarModeToUsePointFieldData();
        data->mapper->SelectColorArray("Normals");
        data->mapper->SetScalarVisibility(1);
        break;
    case PointCloudColorMode::Depth:
        data->elevFilter->SetInputData(data->polyData);
        data->elevFilter->SetLowPoint(0, 0, data->polyData->GetBounds()[5]);
        data->elevFilter->SetHighPoint(0, 0, data->polyData->GetBounds()[4]);
        data->elevFilter->Update();
        data->glyphFilter->SetInputData(data->elevFilter->GetOutput());
        data->mapper->SetScalarModeToUsePointFieldData();
        data->mapper->SelectColorArray("Elevation");
        data->mapper->SetColorModeToDirectScalars();
        data->mapper->SetScalarVisibility(1);
        break;
    case PointCloudColorMode::SolidColor:
        data->glyphFilter->SetInputData(data->polyData);
        data->mapper->SetScalarVisibility(0);
        data->actor->GetProperty()->SetColor(1.0, 1.0, 1.0);
        break;
    default:
        break;
    }

    renderWindow->Render();
}

} // namespace copick3d::qtgui::graphics
