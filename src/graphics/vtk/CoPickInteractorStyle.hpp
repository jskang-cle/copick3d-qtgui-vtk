#pragma once

#include "vtkInteractorStyle.h"

// Forward declarations
class vtkCameraInterpolator;

class CoPickInteractorStyle : public vtkInteractorStyle
{
public:
    static CoPickInteractorStyle* New();
    vtkTypeMacro(CoPickInteractorStyle, vtkInteractorStyle);
    void PrintSelf(ostream& os, vtkIndent indent) override;

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
};
