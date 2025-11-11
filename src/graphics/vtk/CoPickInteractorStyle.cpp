#include "CoPickInteractorStyle.hpp"

#include <vtkActor.h>
#include <vtkAbstractPicker.h>
#include <vtkAbstractPropPicker.h>
#include <vtkAxisActor2D.h>
#include <vtkCallbackCommand.h>
#include <vtkCamera.h>
#include <vtkExtractEdges.h>
#include <vtkMath.h>
#include <vtkObjectFactory.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkSphereSource.h>
#include <vtkCursor3D.h>
#include <vtkProperty.h>
#include <vtkProperty2D.h>
#include <vtkRendererCollection.h>
#include <vtkAxisActor.h>
#include <vtkCaptionActor2D.h>
#include <vtkTextActor.h>
#include <vtkTextProperty.h>
#include <vtkLineSource.h>
#include <vtkPolyDataMapper.h>
#include <vtkGlyph3DMapper.h>
#include <vtkLogger.h>

#include <sstream>

vtkStandardNewMacro(CoPickInteractorStyle);

CoPickInteractorStyle::CoPickInteractorStyle()
{
    this->MotionFactor = 10.0;
    
    vtkNew<vtkCursor3D> cross;
    cross->AllOff();
    cross->AxesOn();

    vtkNew<vtkPolyDataMapper> mapper;
    mapper->SetInputConnection(cross->GetOutputPort());
    // Disabling it gives better results when zooming close
    // to the picked actor in the scene
    mapper->SetResolveCoincidentTopologyToOff();

    double CURSOR_COLOR[3] = {1.0, 1.0, 1.0};
    double CROSS_LINE_WIDTH = 3.0;

    this->PointHighlightActor->SetMapper(mapper);
    this->PointHighlightActor->GetProperty()->SetColor(CURSOR_COLOR);
    this->PointHighlightActor->GetProperty()->SetLineWidth(CROSS_LINE_WIDTH);
    this->PointHighlightActor->UseBoundsOff();

    this->P1Actor->SetMapper(mapper);
    this->P1Actor->GetProperty()->SetColor(1.0, 0.0, 0.0);
    this->P1Actor->GetProperty()->SetLineWidth(CROSS_LINE_WIDTH);
    this->P1Actor->UseBoundsOff();

    this->P2Actor->SetMapper(mapper);
    this->P2Actor->GetProperty()->SetColor(0.0, 0.0, 1.0);
    this->P2Actor->GetProperty()->SetLineWidth(CROSS_LINE_WIDTH);
    this->P2Actor->UseBoundsOff();

    this->DistanceLineActor->GetProperty()->SetLineWidth(2.0);
    // this->DistanceLineActor->GetProperty()->SetColor(1.0, 1.0, 0.0);

    this->DistanceLabelActor->SetBorder(false);
    this->DistanceLabelActor->GetCaptionTextProperty()->FrameOn();
    this->DistanceLabelActor->GetCaptionTextProperty()->SetBackgroundColor(0.0, 0.0, 0.0);
    this->DistanceLabelActor->GetCaptionTextProperty()->SetBackgroundOpacity(0.5);
    this->DistanceLabelActor->GetCaptionTextProperty()->ItalicOff();
    this->DistanceLabelActor->GetCaptionTextProperty()->BoldOff();
    this->DistanceLabelActor->GetCaptionTextProperty()->SetFontSize(14);
    this->DistanceLabelActor->GetCaptionTextProperty()->SetFontFamilyToCourier();
    this->DistanceLabelActor->GetCaptionTextProperty()->SetLineOffset(2);
    this->DistanceLabelActor->GetTextActor()->SetTextScaleModeToNone();

    this->P1LabelActor->SetBorder(false);
    this->P1LabelActor->GetCaptionTextProperty()->FrameOn();
    this->P1LabelActor->GetCaptionTextProperty()->SetBackgroundColor(0.0, 0.0, 0.0);
    this->P1LabelActor->GetCaptionTextProperty()->SetBackgroundOpacity(0.5);
    this->P1LabelActor->GetCaptionTextProperty()->ItalicOff();
    this->P1LabelActor->GetCaptionTextProperty()->BoldOff();
    this->P1LabelActor->GetCaptionTextProperty()->SetFontSize(14);
    this->P1LabelActor->GetCaptionTextProperty()->SetFontFamilyToCourier();
    this->P1LabelActor->GetCaptionTextProperty()->SetLineOffset(2);
    this->P1LabelActor->GetTextActor()->SetTextScaleModeToNone();

    this->P2LabelActor->SetBorder(false);
    this->P2LabelActor->GetCaptionTextProperty()->FrameOn();
    this->P2LabelActor->GetCaptionTextProperty()->SetBackgroundColor(0.0, 0.0, 0.0);
    this->P2LabelActor->GetCaptionTextProperty()->SetBackgroundOpacity(0.5);
    this->P2LabelActor->GetCaptionTextProperty()->ItalicOff();
    this->P2LabelActor->GetCaptionTextProperty()->BoldOff();
    this->P2LabelActor->GetCaptionTextProperty()->SetFontSize(14);
    this->P2LabelActor->GetCaptionTextProperty()->SetFontFamilyToCourier();
    this->P2LabelActor->GetCaptionTextProperty()->SetLineOffset(2);
    this->P2LabelActor->GetTextActor()->SetTextScaleModeToNone();
}

CoPickInteractorStyle::~CoPickInteractorStyle() = default;

void CoPickInteractorStyle::PrintSelf(ostream &os, vtkIndent indent)
{
    this->Superclass::PrintSelf(os, indent);
    os << indent << "MotionFactor: " << this->MotionFactor << "\n";
}

void CoPickInteractorStyle::OnMouseMove()
{
    int x = this->Interactor->GetEventPosition()[0];
    int y = this->Interactor->GetEventPosition()[1];
    
    vtkRenderWindowInteractor *rwi = this->Interactor;
    int shift = rwi->GetShiftKey();

    if (shift)
    {
        double cursorPos[3];
        if (this->GetPickedPoint(cursorPos))
        {
            HoveredPoint[0] = cursorPos[0];
            HoveredPoint[1] = cursorPos[1];
            HoveredPoint[2] = cursorPos[2];

            this->PointHighlightActor->SetPosition(cursorPos);
            this->PointHighlightActor->SetVisibility(true);
            this->Interactor->GetRenderWindow()->GetRenderers()->GetFirstRenderer()->AddActor(this->PointHighlightActor);

            this->InvokeEvent(PointHovered, cursorPos);
        }
    }

    switch (this->State)
    {
    case VTKIS_ENV_ROTATE:
        this->FindPokedRenderer(x, y);
        this->EnvironmentRotate();
        this->InvokeEvent(vtkCommand::InteractionEvent, nullptr);
        break;

    case VTKIS_ROTATE:
        this->FindPokedRenderer(x, y);
        this->Rotate();
        this->InvokeEvent(vtkCommand::InteractionEvent, nullptr);
        break;

    case VTKIS_PAN:
        this->FindPokedRenderer(x, y);
        this->Pan();
        this->InvokeEvent(vtkCommand::InteractionEvent, nullptr);
        break;

    case VTKIS_DOLLY:
        this->FindPokedRenderer(x, y);
        this->Dolly();
        this->InvokeEvent(vtkCommand::InteractionEvent, nullptr);
        break;

    case VTKIS_SPIN:
        this->FindPokedRenderer(x, y);
        this->Spin();
        this->InvokeEvent(vtkCommand::InteractionEvent, nullptr);
        break;
    }
}

void CoPickInteractorStyle::OnLeftButtonDown()
{
    this->FindPokedRenderer(this->Interactor->GetEventPosition()[0], this->Interactor->GetEventPosition()[1]);
    if (this->CurrentRenderer == nullptr)
    {
        return;
    }

    if (this->Interactor->GetShiftKey())
    {
        float *p1 = this->PreviousClickedPoint;
        float *p2 = this->CurrentClickedPoint;
        float *h = this->HoveredPoint;

        if (p1[0] != 0 && p1[1] != 0 && p1[2] != 0)
        {
            this->CurrentRenderer->RemoveActor(this->DistanceLineActor);
            this->CurrentRenderer->RemoveActor(this->DistanceLabelActor);

            p1[0] = p2[0] = 0.0f;
            p1[1] = p2[1] = 0.0f;
            p1[2] = p2[2] = 0.0f;

            this->CurrentRenderer->RemoveActor(this->P1Actor);
            this->CurrentRenderer->RemoveActor(this->P2Actor);
            this->CurrentRenderer->RemoveActor(this->P1LabelActor);
            this->CurrentRenderer->RemoveActor(this->P2LabelActor);
        }

        p1[0] = p2[0];
        p1[1] = p2[1];
        p1[2] = p2[2];

        p2[0] = h[0];
        p2[1] = h[1];
        p2[2] = h[2];

        this->P1Actor->SetPosition(p1[0], p1[1], p1[2]);
        this->P2Actor->SetPosition(p2[0], p2[1], p2[2]);
        this->CurrentRenderer->AddActor(this->P1Actor);
        this->CurrentRenderer->AddActor(this->P2Actor);

        vtkLog(INFO, "Clicked point at: ("
            << p2[0] << ", "
            << p2[1] << ", "
            << p2[2] << ")");

        if (PreviousClickedPoint[0] != 0 && PreviousClickedPoint[1] != 0 && PreviousClickedPoint[2] != 0)
        {
            vtkLog(INFO, "Drawing distance line between point 1 ("
                << p1[0] << ", "
                << p1[1] << ", "
                << p1[2] << ") and point 2 ("
                << p2[0] << ", "
                << p2[1] << ", "
                << p2[2] << ")");

            vtkRenderer* renderer = this->CurrentRenderer;

            this->DistanceLineActor->SetPoint1(p1[0], p1[1], p1[2]);
            this->DistanceLineActor->SetPoint2(p2[0], p2[1], p2[2]);

            renderer->AddActor(this->DistanceLineActor);

            double distance = sqrt(vtkMath::Distance2BetweenPoints(p1, p2));
            std::string distanceText = "Distance: " + std::to_string(distance);
            this->DistanceLabelActor->SetCaption(distanceText.c_str());
            double midPoint[3] = {
                (p1[0] + p2[0]) / 2.0,
                (p1[1] + p2[1]) / 2.0,
                (p1[2] + p2[2]) / 2.0
            };
            this->DistanceLabelActor->SetAttachmentPoint(midPoint);
            this->DistanceLabelActor->SetVisibility(true);

            renderer->AddActor(this->DistanceLabelActor);

            std::stringstream ss;
            ss << std::fixed << std::setprecision(2) << "P1 (" << p1[0] << ", " << p1[1] << ", " << p1[2] << ")";
            this->P1LabelActor->SetCaption(ss.str().c_str());
            this->P1LabelActor->SetAttachmentPoint(p1[0], p1[1], p1[2]);
            this->P1LabelActor->SetVisibility(true);

            ss.str(std::string());
            ss << std::fixed << std::setprecision(2) << "P2 (" << p2[0] << ", " << p2[1] << ", " << p2[2] << ")";

            this->P2LabelActor->SetCaption(ss.str().c_str());
            this->P2LabelActor->SetAttachmentPoint(p2[0], p2[1], p2[2]);
            this->P2LabelActor->SetVisibility(true);
            this->CurrentRenderer->AddActor(this->P1LabelActor);
            this->CurrentRenderer->AddActor(this->P2LabelActor);
        }

        return;
    }

    this->GrabFocus(this->EventCallbackCommand);
    this->StartRotate();
}

void CoPickInteractorStyle::OnLeftButtonUp()
{
    // switch to if-else for readability
    if (this->State == VTKIS_DOLLY)
    {
        this->EndDolly();
    }
    else if (this->State == VTKIS_PAN)
    {
        this->EndPan();
    }
    else if (this->State == VTKIS_SPIN)
    {
        this->EndSpin();
    }
    else if (this->State == VTKIS_ROTATE)
    {
        this->EndRotate();
    }

    if (this->Interactor)
    {
        this->ReleaseFocus();
    }
}

void CoPickInteractorStyle::OnMiddleButtonDown()
{
    this->FindPokedRenderer(
        this->Interactor->GetEventPosition()[0], this->Interactor->GetEventPosition()[1]);
    if (this->CurrentRenderer == nullptr)
    {
        return;
    }

    this->GrabFocus(this->EventCallbackCommand);
    this->StartDolly();
}

void CoPickInteractorStyle::OnMiddleButtonUp()
{
    if (this->State == VTKIS_DOLLY)
    {
        this->EndDolly();

        if (this->Interactor)
        {
            this->ReleaseFocus();
        }
    }
}

void CoPickInteractorStyle::OnRightButtonDown()
{
    this->FindPokedRenderer(
        this->Interactor->GetEventPosition()[0], this->Interactor->GetEventPosition()[1]);
    if (this->CurrentRenderer == nullptr)
    {
        return;
    }

    this->GrabFocus(this->EventCallbackCommand);
    this->StartPan();
}

void CoPickInteractorStyle::OnRightButtonUp()
{
    if (this->State == VTKIS_PAN)
    {
        this->EndPan();

        if (this->Interactor)
        {
            this->ReleaseFocus();
        }
    }
}

void CoPickInteractorStyle::OnMouseWheelForward()
{
    this->FindPokedRenderer(
        this->Interactor->GetEventPosition()[0], this->Interactor->GetEventPosition()[1]);
    if (this->CurrentRenderer == nullptr)
    {
        return;
    }

    this->GrabFocus(this->EventCallbackCommand);
    this->StartDolly();
    double factor = this->MotionFactor * 0.2 * this->MouseWheelMotionFactor;
    this->Dolly(pow(1.1, factor));
    this->EndDolly();
    this->ReleaseFocus();
}

void CoPickInteractorStyle::OnMouseWheelBackward()
{
    this->FindPokedRenderer(
        this->Interactor->GetEventPosition()[0], this->Interactor->GetEventPosition()[1]);
    if (this->CurrentRenderer == nullptr)
    {
        return;
    }

    this->GrabFocus(this->EventCallbackCommand);
    this->StartDolly();
    double factor = this->MotionFactor * -0.2 * this->MouseWheelMotionFactor;
    this->Dolly(pow(1.1, factor));
    this->EndDolly();
    this->ReleaseFocus();
}

void CoPickInteractorStyle::Rotate()
{
    if (this->CurrentRenderer == nullptr)
    {
        return;
    }

    vtkRenderWindowInteractor *rwi = this->Interactor;

    int dx = -(rwi->GetEventPosition()[0] - rwi->GetLastEventPosition()[0]);
    int dy = -(rwi->GetEventPosition()[1] - rwi->GetLastEventPosition()[1]);

    const int *size = this->CurrentRenderer->GetRenderWindow()->GetSize();

    double a = dx / static_cast<double>(size[0]) * 180.0;
    double e = dy / static_cast<double>(size[1]) * 180.0;

    if (rwi->GetShiftKey())
    {
        if (abs(dx) >= abs(dy))
        {
            e = 0.0;
        }
        else
        {
            a = 0.0;
        }
    }

    // Move the camera.
    // Make sure that we don't hit the north pole singularity.
    vtkCamera *camera = this->CurrentRenderer->GetActiveCamera();
    camera->Azimuth(a);

    double dop[3], vup[3];

    camera->GetDirectionOfProjection(dop);
    vtkMath::Normalize(dop);
    camera->GetViewUp(vup);
    vtkMath::Normalize(vup);

    double angle = vtkMath::DegreesFromRadians(acos(vtkMath::Dot(dop, vup)));
    if ((angle + e) > 179.0 || (angle + e) < 1.0)
    {
        e = 0.0;
    }

    camera->Elevation(e);

    if (this->AutoAdjustCameraClippingRange)
    {
        this->CurrentRenderer->ResetCameraClippingRange();
    }
}

void CoPickInteractorStyle::Pan()
{
    if (this->CurrentRenderer == nullptr)
    {
        return;
    }

    vtkRenderWindowInteractor *rwi = this->Interactor;

    // Get the vector of motion
    double fp[3], focalPoint[3], pos[3], v[3], p1[4], p2[4];

    vtkCamera *camera = this->CurrentRenderer->GetActiveCamera();
    camera->GetPosition(pos);
    camera->GetFocalPoint(fp);

    this->ComputeWorldToDisplay(fp[0], fp[1], fp[2], focalPoint);

    this->ComputeDisplayToWorld(
        rwi->GetEventPosition()[0], rwi->GetEventPosition()[1], focalPoint[2], p1);

    this->ComputeDisplayToWorld(
        rwi->GetLastEventPosition()[0], rwi->GetLastEventPosition()[1], focalPoint[2], p2);

    for (int i = 0; i < 3; i++)
    {
        v[i] = p2[i] - p1[i];
        pos[i] += v[i];
        fp[i] += v[i];
    }

    camera->SetPosition(pos);
    camera->SetFocalPoint(fp);

    if (rwi->GetLightFollowCamera())
    {
        this->CurrentRenderer->UpdateLightsGeometryToFollowCamera();
    }
}

void CoPickInteractorStyle::Dolly()
{
    if (this->CurrentRenderer == nullptr)
    {
        return;
    }

    vtkRenderWindowInteractor *rwi = this->Interactor;
    vtkCamera *camera = this->CurrentRenderer->GetActiveCamera();
    double *center = this->CurrentRenderer->GetCenter();

    int dy = rwi->GetEventPosition()[1] - rwi->GetLastEventPosition()[1];
    double dyf = this->MotionFactor * dy / center[1];
    double zoomFactor = pow(1.1, dyf);

    if (camera->GetParallelProjection())
    {
        camera->SetParallelScale(camera->GetParallelScale() / zoomFactor);
    }
    else
    {
        camera->Dolly(zoomFactor);
        if (this->AutoAdjustCameraClippingRange)
        {
            this->CurrentRenderer->ResetCameraClippingRange();
        }
    }

    if (rwi->GetLightFollowCamera())
    {
        this->CurrentRenderer->UpdateLightsGeometryToFollowCamera();
    }
}

void CoPickInteractorStyle::Dolly(double factor)
{
    if (this->CurrentRenderer == nullptr)
    {
        return;
    }

    vtkCamera *camera = this->CurrentRenderer->GetActiveCamera();
    if (camera->GetParallelProjection())
    {
        camera->SetParallelScale(camera->GetParallelScale() / factor);
    }
    else
    {
        camera->Dolly(factor);
        if (this->AutoAdjustCameraClippingRange)
        {
            this->CurrentRenderer->ResetCameraClippingRange();
        }
    }

    if (this->Interactor->GetLightFollowCamera())
    {
        this->CurrentRenderer->UpdateLightsGeometryToFollowCamera();
    }
}

void CoPickInteractorStyle::OnChar()
{
    this->FindPokedRenderer(
        this->Interactor->GetEventPosition()[0], this->Interactor->GetEventPosition()[1]);
        
    vtkRenderWindowInteractor *rwi = this->Interactor;
    std::string key = rwi->GetKeySym();
    std::transform(key.begin(), key.end(), key.begin(), ::toupper);

    switch (key[0])
    {
    case 'F':
        this->FlyToPickedPosition();
        return;
    case 'R':
        this->ResetToHomePosition();
        return;
    }

    this->Superclass::OnChar();
}

void CoPickInteractorStyle::FlyToPickedPosition()
{
    if (this->CurrentRenderer == nullptr)
    {
        vtkWarningMacro(<< "no current renderer on the interactor style.");
        return;
    }
    
    double pickedPos[3];
    if (!this->GetPickedPoint(pickedPos))
    {
        vtkLog(WARNING, "No point picked, cannot fly to position.");
        return;
    }

    double focFrom[3], *focTo, flyDiff[3], posFrom[3], posTo[3], viewUp[3];
    int flyFrames = this->Interactor->GetNumberOfFlyFrames();
    focTo = pickedPos;

    auto cam = this->CurrentRenderer->GetActiveCamera();
    cam->GetPosition(posFrom);
    cam->GetFocalPoint(focFrom);
    cam->GetViewUp(viewUp);

    vtkMath::Subtract(focTo, focFrom, flyDiff);
    double distance = vtkMath::Norm(flyDiff);

    vtkLog(INFO, "Flying to picked position: " 
        << focTo[0] << ", " << focTo[1] << ", " << focTo[2]
        << " from " 
        << focFrom[0] << ", " << focFrom[1] << ", " << focFrom[2]
        << " viewUp: " << viewUp[0] << ", " << viewUp[1] << ", " << viewUp[2]
        << " distance: " << distance
        );

    for (int i = 0; i < 3; ++i)
    {
        posTo[i] = posFrom[i] + flyDiff[i];
    }

    // cam->SetPosition(posTo);
    cam->SetFocalPoint(focTo);
    // cam->SetViewUp(0, -1, 0);
    // this->CurrentRenderer->ResetCameraClippingRange();
    this->AnimState = VTKIS_ANIM_OFF;
}

void CoPickInteractorStyle::ResetToHomePosition()
{
    if (this->CurrentRenderer == nullptr)
    {
        vtkWarningMacro(<< "no current renderer on the interactor style.");
        return;
    }

    vtkRenderWindowInteractor *rwi = this->Interactor;
    auto cam = this->CurrentRenderer->GetActiveCamera();
    cam->SetPosition(this->HomePosition);
    cam->SetViewUp(this->HomeUp);
    this->CurrentRenderer->ResetCamera();
    // rwi->Render();
}

bool CoPickInteractorStyle::GetPickedPoint(double pickedPos[3])
{
    auto renderer = this->CurrentRenderer;

    if (renderer == nullptr)
    {
        // this might happen if the user has never clicked in the render window
        // fall back to the first renderer in the window
        renderer = this->Interactor->GetRenderWindow()->GetRenderers()->GetFirstRenderer();
    }

    vtkRenderWindowInteractor *rwi = this->Interactor;
    int x = rwi->GetEventPosition()[0];
    int y = rwi->GetEventPosition()[1];

    vtkAbstractPropPicker *picker = vtkAbstractPropPicker::SafeDownCast(rwi->GetPicker());
    if (!picker)
    {
        vtkWarningMacro(<< "no picker set on the interactor.");
        return false;
    }

    if (picker->Pick(x, y, 0.0, this->CurrentRenderer) == 0)
    {
        return false;
    }

    picker->GetPickPosition(pickedPos);
    return true;
}