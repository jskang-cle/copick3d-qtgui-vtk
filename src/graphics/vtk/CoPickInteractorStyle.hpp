#pragma once

#include "vtkInteractorStyle.h"
#include "vtkSmartPointer.h"

// Forward declarations
class vtkCameraInterpolator;
class vtkActor;

class CoPickInteractorStyle : public vtkInteractorStyle
{
public:
    static CoPickInteractorStyle* New();
    vtkTypeMacro(CoPickInteractorStyle, vtkInteractorStyle);
    void PrintSelf(ostream& os, vtkIndent indent) override;

    enum CoPickInteractorEvent
    {
        PointHovered = 10000,
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

    vtkGetVector3Macro(HomePosition, double);
    vtkSetVector3Macro(HomePosition, double);

    vtkGetVector3Macro(HomeUp, double);
    vtkSetVector3Macro(HomeUp, double);

    vtkGetVector3Macro(HoveredPoint, double);

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

    double HoveredPoint[3] = {0.0, 0.0, 0.0};
};
