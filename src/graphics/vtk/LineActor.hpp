#pragma once

#include <vtkActor.h>
#include <vtkOpenGLActor.h>
#include <vtkActor2D.h>
#include <vtkNew.h>
#include <vtkLogger.h>

class vtkPoints;
class vtkCellArray;
class vtkPolyData;
class vtkPolyDataMapper;
class vtkPolyDataMapper2D;

class LineActor : public vtkOpenGLActor
{
public:
    static LineActor* New();
    vtkTypeMacro(LineActor, vtkOpenGLActor);

    void SetPoint1(double x, double y, double z)
    {
        this->Point1[0] = x;
        this->Point1[1] = y;
        this->Point1[2] = z;
        this->BuildLine();
    }
    void SetPoint2(double x, double y, double z)
    {
        this->Point2[0] = x;
        this->Point2[1] = y;
        this->Point2[2] = z;
        this->BuildLine();
    }

    vtkGetVector3Macro(Point1, double);
    vtkGetVector3Macro(Point2, double);

    // int RenderOpaqueGeometry(vtkViewport* viewport) override;

protected:
    LineActor();
    ~LineActor() override = default;

    double Point1[3] = { 0.0, 0.0, 0.0 };
    double Point2[3] = { 0.0, 0.0, 0.0 };

private:
    void BuildLine();

    LineActor(const LineActor&) = delete;
    void operator=(const LineActor&) = delete;

    vtkNew<vtkPoints> LinePoints;
    vtkNew<vtkCellArray> LineCells;
    vtkNew<vtkPolyData> LinePolyData;
    vtkNew<vtkPolyDataMapper> LineMapper;
};
