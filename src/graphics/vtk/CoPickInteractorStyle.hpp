#pragma once

#include <vtkInteractorStyle.h>
#include <vtkSmartPointer.h>
#include <vtkVector.h>

#include "LineActor.hpp"

// Forward declarations
class vtkCameraInterpolator;
class vtkActor;

class vtkAxisActor;
class vtkAxisActor2D;
class vtkCaptionActor2D;

class CoPickInteractorStyle : public vtkInteractorStyle
{
public:
    static CoPickInteractorStyle* New();
    vtkTypeMacro(CoPickInteractorStyle, vtkInteractorStyle);
    void PrintSelf(ostream& os, vtkIndent indent) override;

    enum CoPickInteractorEvent
    {
        PointPicked = 10000,
        PointHovered = 10001,
    };

    ///@{
    /**
     * Event bindings controlling the effects of pressing mouse buttons
     * or moving the mouse.
     */
    void OnMouseMove() override;
    void OnLeftButtonDown() override;
    void OnLeftButtonUp() override;
    void OnMiddleButtonDown() override;
    void OnMiddleButtonUp() override;
    void OnRightButtonDown() override;
    void OnRightButtonUp() override;
    void OnMouseWheelForward() override;
    void OnMouseWheelBackward() override;
    ///@}

    /**
     * Override the "fly-to" (f keypress) for images.
     */
    void OnChar() override;

    // These methods for the different interactions in different modes
    // are overridden in subclasses to perform the correct motion.
    void Rotate() override;
    void Pan() override;
    void Dolly() override;

protected:
    CoPickInteractorStyle();
    ~CoPickInteractorStyle() override;

    virtual void Dolly(double factor);
    virtual void FlyToPickedPosition();
    virtual void ResetToHomePosition();

    double MotionFactor;
    int flyAnimationFrames = 30;
    double HomePosition[3] = {0.0, 0.0, 0.0};
    double HomeUp[3] = {0.0, -1.0, 0.0};

private:
    CoPickInteractorStyle(const CoPickInteractorStyle&) = delete;
    void operator=(const CoPickInteractorStyle&) = delete;

    bool GetPickedPoint(double pickedPos[3]);

    vtkNew<vtkActor> PointHighlightActor;

    vtkNew<vtkActor> P1Actor;
    vtkNew<vtkActor> P2Actor;
    vtkNew<vtkCaptionActor2D> P1LabelActor;
    vtkNew<vtkCaptionActor2D> P2LabelActor;

    vtkNew<LineActor> DistanceLineActor;
    vtkNew<vtkCaptionActor2D> DistanceLabelActor;

    float HoveredPoint[3] = {0.0f, 0.0f, 0.0f};
    float PreviousClickedPoint[3] = {0.0f, 0.0f, 0.0f};
    float CurrentClickedPoint[3] = {0.0f, 0.0f, 0.0f};
};
