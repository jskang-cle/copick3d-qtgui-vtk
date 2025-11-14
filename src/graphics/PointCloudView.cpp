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
    data->renderer->SetBackground(this->m_backgroundColor.redF(),
                                 this->m_backgroundColor.greenF(),
                                 this->m_backgroundColor.blueF());

    renderWindow->AddRenderer(data->renderer);
    renderWindow->SetMultiSamples(8);

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

    data->cameraWidget->SetParentRenderer(data->renderer);
    data->cameraWidget->SetInteractor(renderWindow->GetInteractor());
    data->cameraWidget->SetEnabled(1);
    data->cameraWidget->GetDefaultRenderer()->GetActiveCamera()->ParallelProjectionOn();
    // data->cameraWidget->AnimateOff();
    // data->cameraWidget->SetProcessEvents(false);
    auto rep = vtkCameraOrientationRepresentation::SafeDownCast(data->cameraWidget->GetRepresentation());
    if (rep)
    {
        rep->SetPadding(this->m_innerPadding, this->m_innerPadding);
    }

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

void PointCloudView::resetCamera()
{
    qDebug() << "PointCloudView::resetCamera";
    dispatch_async([](vtkRenderWindow* renderWindow, vtkUserData userData)
    {
        auto data = PointCloudViewData::SafeDownCast(userData);
        if (!data) return;
        qDebug() << "PointCloudView::resetCamera - in dispatch";
        data->renderer->GetActiveCamera()->SetPosition(0, 0, 0);
        data->renderer->GetActiveCamera()->SetViewUp(0, -1, 0);
        data->renderer->ResetCameraClippingRange();
        data->renderer->ResetCamera();
    });
}

#define SET_WITH_DISPATCH(method, type, member, value, signal, dispatchCode) \
    void PointCloudView::method(type value) \
    { \
        if (value == member) \
            return; \
        member = value; \
        Q_EMIT signal(value); \
        dispatch_async([value](vtkRenderWindow* renderWindow, vtkUserData userData) \
        { \
            auto data = PointCloudViewData::SafeDownCast(userData); \
            if (!data) return; \
            dispatchCode \
        }); \
    }

SET_WITH_DISPATCH(setFrame, QSharedPointer<copick3d::Frame>, m_frame, frame, frameChanged,
    data->actor->SetFrame(frame);
    data->cubeAxesActor->SetBounds(data->actor->GetBounds());

    data->renderer->ResetCameraClippingRange();
    data->renderer->ResetCamera();
)

SET_WITH_DISPATCH(setParallelProjection, bool, m_parallelProjection, enable, parallelProjectionChanged,
    data->renderer->GetActiveCamera()->SetParallelProjection(enable);
)

SET_WITH_DISPATCH(setFixedPointSize, bool, m_fixedPointSize, enable, fixedPointSizeChanged,
    PointCloudActor* actor = data->actor;
    actor->SetFixedPointSize(enable);
)

SET_WITH_DISPATCH(setColorMode, PointCloudColorMode, m_colorMode, mode, colorModeChanged,
    PointCloudActor* actor = data->actor;
    actor->SetColorMode(static_cast<PointCloudActor::PointColorMode>(mode));
)

SET_WITH_DISPATCH(setPointSize, float, m_pointSize, size, pointSizeChanged,
    PointCloudActor* actor = data->actor;
    actor->SetPointSize(static_cast<double>(size));
);

SET_WITH_DISPATCH(setBackgroundColor, const QColor, m_backgroundColor, color, backgroundColorChanged,
    data->renderer->SetBackground(color.redF(), color.greenF(), color.blueF());
)

SET_WITH_DISPATCH(setAxisGridVisible, bool, m_axisGridVisible, visible, axisGridVisibleChanged,
    data->cubeAxesActor->SetVisibility(visible);
);

SET_WITH_DISPATCH(setInnerPadding, float, m_innerPadding, padding, innerPaddingChanged,
    auto rep = vtkCameraOrientationRepresentation::SafeDownCast(data->cameraWidget->GetRepresentation());
    if (rep)
    {
        rep->SetPadding(padding, padding);
    }
);


} // namespace copick3d::qtgui::graphics
