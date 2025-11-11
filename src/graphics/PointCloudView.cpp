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
#include <vtkCellArray.h>
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

#include "vtkCallbackCommand.h"

#include "PointCloudActor.hpp"
#include "vtk/LineActor.hpp"

namespace copick3d::qtgui::graphics
{

struct PointCloudViewData : vtkObject
{
    static PointCloudViewData* New();
    vtkTypeMacro(PointCloudViewData, vtkObject);

    copick3d::Frame frame;

    vtkNew<vtkRenderer> renderer;

    vtkNew<vtkCameraOrientationWidgetEx> cameraWidget;

    vtkNew<PointCloudActor> actor;
    vtkNew<vtkCubeAxesActor> cubeAxesActor;
};

vtkStandardNewMacro(PointCloudViewData);

PointCloudView::PointCloudView(QQuickItem *parent)
    : QQuickVTKItemEx(parent)
{
}

vtkSmartPointer<vtkPoints> GetPtOfCircle(int numOfPts, double radius, double* center)
{
	vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
	for (unsigned int i = 0; i < numOfPts; i++)
	{
		const double angle = 2.0 * vtkMath::Pi() * (double)i / (double)numOfPts;
		points->InsertPoint((vtkIdType)i, radius*cos(angle)+center[0], radius*sin(angle)+center[1], 0+center[2]);
	}
	return points;
}

QQuickVTKItem::vtkUserData PointCloudView::initializeVTK(vtkRenderWindow *renderWindow)
{
    PointCloudViewData* data = PointCloudViewData::New();
    data->renderer->SetBackground(this->m_backgroundColor.redF(),
                                  this->m_backgroundColor.greenF(),
                                  this->m_backgroundColor.blueF());

    renderWindow->AddRenderer(data->renderer);
    renderWindow->SetMultiSamples(8);
    qDebug() << "Mutlisamples:" << renderWindow->GetMultiSamples();

    vtkNew<CoPickInteractorStyle> style;
    style->SetDefaultRenderer(data->renderer);
    renderWindow->GetInteractor()->SetInteractorStyle(style);

    vtkCallbackCommand* hoveredCommand = vtkCallbackCommand::New();
    hoveredCommand->SetClientData(this);
    hoveredCommand->SetCallback([](vtkObject* caller, unsigned long, void* clientdata, void* calldata)
    {
        double* pos = static_cast<double*>(calldata);
        QVector3D pickedPoint(static_cast<float>(pos[0]),
                              static_cast<float>(pos[1]),
                              static_cast<float>(pos[2]));

        auto item = static_cast<PointCloudView*>(clientdata);
        item->m_pickedPoint = pickedPoint;
        Q_EMIT item->pickedPointChanged(pickedPoint);
    });

    style->AddObserver(CoPickInteractorStyle::PointHovered, hoveredCommand);

    data->renderer->AddActor(data->actor);

    data->cubeAxesActor->SetBounds(data->actor->GetBounds());
    data->cubeAxesActor->SetCamera(data->renderer->GetActiveCamera());
    data->cubeAxesActor->SetGridLineLocation(vtkCubeAxesActor::VTK_GRID_LINES_FURTHEST);
    data->cubeAxesActor->DrawXGridlinesOn();
    data->cubeAxesActor->DrawYGridlinesOn();
    data->cubeAxesActor->DrawZGridlinesOn();
    data->cubeAxesActor->PickableOff();

    data->renderer->AddActor(data->cubeAxesActor);

    data->cameraWidget->SetParentRenderer(data->renderer);
    data->cameraWidget->SetInteractor(renderWindow->GetInteractor());
    data->cameraWidget->SetEnabled(1);
    data->cameraWidget->GetDefaultRenderer()->GetActiveCamera()->ParallelProjectionOn();
    // data->cameraWidget->AnimateOff();
    // data->cameraWidget->SetProcessEvents(false);

    auto cam = data->renderer->GetActiveCamera();
    cam->SetPosition(0, 0, 0);
    cam->SetViewUp(0, -1, 0);
    cam->SetFocalPoint(0, 0, 1000);
    cam->SetClippingRange(1.0, 2000.0);

    renderWindow->GetInteractor()->LightFollowCameraOff();
    // data->renderer->LightFollowCameraOff();

    vtkNew<PointPickerUsingLocator> picker;
    // picker->SetSnapToMeshPoint(true);
    renderWindow->GetInteractor()->SetPicker(picker);

    vtkNew<vtkCallbackCommand> objectModifiedCallback;
    objectModifiedCallback->SetClientData(data);
    objectModifiedCallback->SetCallback([](vtkObject* caller, unsigned long, void* clientdata, void*)
    {
        auto data = static_cast<PointCloudViewData*>(clientdata);
        if (!data)
            return;
        data->cubeAxesActor->SetBounds(data->actor->GetBounds());
        data->renderer->ResetCameraClippingRange();
    });

    data->actor->AddObserver(vtkCommand::ModifiedEvent, objectModifiedCallback);

    return data;
}

/// Macro to define property setter then apply the change in the render thread using dispatch_async.
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
            if (!data) \
                return; \
            dispatchCode \
        }); \
    }

SET_WITH_DISPATCH(setFrame, QSharedPointer<copick3d::Frame>, m_frame, frame, frameChanged,
    data->actor->SetFrame(frame);

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


} // namespace copick3d::qtgui::graphics
