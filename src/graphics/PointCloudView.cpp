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

#include <vtkSphere.h>
#include <vtkSphereSource.h>

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
#include <vtkPointPicker.h>

#include "vtk/CoPickInteractorStyle.hpp"
#include "vtk/PointPickerUsingLocator.hpp"
#include "vtk/vtkCameraOrientationWidgetEx.hpp"

#include "PointCloudActor.hpp"

namespace copick3d::qtgui::graphics
{

struct PointCloudViewData : vtkObject
{
    static PointCloudViewData* New();
    vtkTypeMacro(PointCloudViewData, vtkObject);

    copick3d::Frame frame;

    vtkNew<vtkRenderer> renderer;

    vtkNew<vtkCameraOrientationWidgetEx> cameraWidget;
    vtkObject* orientationWidget;

    vtkNew<PointCloudActor> actor;

    vtkNew<vtkCubeAxesActor> cubeAxesActor;
};

vtkStandardNewMacro(PointCloudViewData);

PointCloudView::PointCloudView(QQuickItem *parent)
    : QQuickVTKItemEx(parent)
{
}

QQuickVTKItem::vtkUserData PointCloudView::initializeVTK(vtkRenderWindow *renderWindow)
{
    vtkNew<PointCloudViewData> data;
    data->renderer->SetBackground(0.1, 0.1, 0.1);

    renderWindow->AddRenderer(data->renderer);
    renderWindow->SetMultiSamples(8);
    qDebug() << "Mutlisamples:" << renderWindow->GetMultiSamples();

    vtkNew<CoPickInteractorStyle> style;
    style->SetDefaultRenderer(data->renderer);
    renderWindow->GetInteractor()->SetInteractorStyle(style);

    data->renderer->AddActor(data->actor);

    data->cubeAxesActor->SetBounds(data->actor->GetBounds());
    data->cubeAxesActor->SetCamera(data->renderer->GetActiveCamera());
    data->cubeAxesActor->SetGridLineLocation(vtkCubeAxesActor::VTK_GRID_LINES_FURTHEST);
    data->cubeAxesActor->DrawXGridlinesOn();
    data->cubeAxesActor->DrawYGridlinesOn();
    data->cubeAxesActor->DrawZGridlinesOn();
    data->cubeAxesActor->PickableOff();

    // data->renderer->AddActor(data->cubeAxesActor);

    // data->renderer->ResetCamera();

    // auto rep = vtkSmartPointer<vtkAxesActor>::New();
    // data->orientationWidget->SetOrientationMarker(rep);
    // data->orientationWidget->SetInteractor(renderWindow->GetInteractor());
    // data->orientationWidget->SetEnabled(1);
    // data->orientationWidget->InteractiveOff();

    auto orientationWidget = vtkOrientationMarkerWidget::New();
    orientationWidget->SetOrientationMarker(vtkSmartPointer<vtkAxesActor>::New());
    orientationWidget->SetInteractor(renderWindow->GetInteractor());
    orientationWidget->SetEnabled(1);
    orientationWidget->InteractiveOff();
    data->orientationWidget = orientationWidget;

    data->cameraWidget->SetParentRenderer(data->renderer);
    data->cameraWidget->SetInteractor(renderWindow->GetInteractor());
    data->cameraWidget->SetEnabled(1);
    // data->cameraWidget->AnimateOff();
    // data->cameraWidget->SetProcessEvents(false);

    auto cam = data->renderer->GetActiveCamera();
    cam->SetPosition(0, 0, 0);
    cam->SetViewUp(0, -1, 0);
    cam->SetFocalPoint(0, 0, 1000);
    cam->SetClippingRange(1.0, 2000.0);

    vtkNew<PointPickerUsingLocator> picker;
    // picker->SetSnapToMeshPoint(true);
    renderWindow->GetInteractor()->SetPicker(picker);

    // // add sphere actor for testing
    // vtkNew<vtkSphereSource> sphere;
    // sphere->SetCenter(0.0, 0.0, 0.0);
    // sphere->SetRadius(5.0);
    // sphere->SetThetaResolution(32);
    // sphere->SetPhiResolution(32);

    // vtkNew<vtkPolyDataMapper> sphereMapper;
    // sphereMapper->SetInputConnection(sphere->GetOutputPort());
    // vtkNew<vtkActor> sphereActor;
    // sphereActor->SetMapper(sphereMapper);

    // data->renderer->AddActor(sphereActor);

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
        auto data = PointCloudViewData::SafeDownCast(userData);
        if (!data)
            return;

        data->actor->SetFrame(frame);
        data->cubeAxesActor->SetBounds(data->actor->GetBounds());

        data->renderer->ResetCameraClippingRange();
        data->renderer->ResetCamera();
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

        data->renderer->GetActiveCamera()->SetParallelProjection(enable);
    });
}

void PointCloudView::setColorMode(PointCloudColorMode mode)
{
    if (mode == m_colorMode)
        return;
    m_colorMode = mode;
    Q_EMIT colorModeChanged(mode);

    dispatch_async([mode](vtkRenderWindow* renderWindow, vtkUserData userData)
    {
        auto data = PointCloudViewData::SafeDownCast(userData);
        if (!data)
            return;

        PointCloudActor* actor = data->actor;
        actor->SetColorMode(static_cast<PointCloudActor::PointColorMode>(mode));
    });
}

void PointCloudView::setPointSize(float size)
{
    if (size <= 0.0f)
        return;
    if (size == m_pointSize)
        return;

    dispatch_async([size](vtkRenderWindow* renderWindow, vtkUserData userData)
    {
        auto data = PointCloudViewData::SafeDownCast(userData);
        if (!data)
            return;

        PointCloudActor* actor = data->actor;
        actor->SetPointSize(static_cast<double>(size));
    });
}

} // namespace copick3d::qtgui::graphics
