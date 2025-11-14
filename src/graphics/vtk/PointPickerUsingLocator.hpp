#pragma once

#include <vtkPointPicker.h>
#include <vtkSmartPointer.h>
#include <vtkPointSet.h>
#include <vtkAbstractPointLocator.h>
#include <vtkStaticPointLocator.h>
#include <vtkLogger.h>
#include <vtkMath.h>
#include <vtkLine.h>

#include <QDebug>

class PointPickerUsingLocator : public vtkPointPicker
{
public:
    static PointPickerUsingLocator *New();
    vtkTypeMacro(PointPickerUsingLocator, vtkPointPicker);

protected:
    PointPickerUsingLocator() = default;
    ~PointPickerUsingLocator() override = default;

    double IntersectWithLine(const double p1[3], const double p2[3], double tol,
        vtkAssemblyPath* path, vtkProp3D* p, vtkAbstractMapper3D* m) override;

private:
    PointPickerUsingLocator(const PointPickerUsingLocator &) = delete;
    void operator=(const PointPickerUsingLocator &) = delete;
};

vtkStandardNewMacro(PointPickerUsingLocator);

vtkIdType RobustIntersectWithLine(vtkStaticPointLocator* locator, 
                                  double a0[3], double a1[3], 
                                  double baseTolerance,
                                  double& t, double lineX[3], 
                                  double ptX[3], vtkIdType& ptId)
{
    // Try multiple tolerance values
    double tolerances[] = {baseTolerance, baseTolerance * 5.0, baseTolerance * 10.0};
    
    for (int i = 0; i < 4; i++)
    {
        if (locator->IntersectWithLine(a0, a1, tolerances[i], t, lineX, ptX, ptId))
        {
            return 1;
        }
    }
    return 0;
}

double PointPickerUsingLocator::IntersectWithLine(
    const double p1[3], const double p2[3], double tol,
    vtkAssemblyPath* path, vtkProp3D* p, vtkAbstractMapper3D* m)
{
    vtkMapper* mapper = vtkMapper::SafeDownCast(m);
    if (!mapper)
    {
        return Superclass::IntersectWithLine(p1, p2, tol, path, p, m);
    }
    vtkDataSet* dataSet = mapper->GetInput();
    if (!dataSet)
    {
        return Superclass::IntersectWithLine(p1, p2, tol, path, p, m);
    }

    vtkPointSet *pointSet = vtkPointSet::SafeDownCast(dataSet);

    if (!this->UseCells                 // we are picking points
        && pointSet != nullptr          // we have a point set
        && !pointSet->GetEditable()     // the point set is not editable (static)
        && pointSet->GetNumberOfPoints() > 10000) // and has enough points to justify building a static locator
    {
        pointSet->BuildPointLocator();
        vtkAbstractPointLocator *locator = pointSet->GetPointLocator();
        vtkStaticPointLocator *staticLocator  = vtkStaticPointLocator::SafeDownCast(locator);

        if (staticLocator)
        {
            vtkIdType ptId = -1;
            double t = 0.0;
            double lineX[3];
            double ptX[3];

            // vtkLog(INFO, << "Using static point locator for picking"
            //              << " p1: " << p1[0] << ", " << p1[1] << ", " << p1[2]
            //              << " p2: " << p2[0] << ", " << p2[1] << ", " << p2[2]
            //              << " tol: " << tol);

            // if (staticLocator->IntersectWithLine((double*)p1, (double*)p2, 2.0, t, lineX, ptX, ptId))
            if (RobustIntersectWithLine(staticLocator, (double*)p1, (double*)p2, 0.1, t, lineX, ptX, ptId))
            {
                if (t >= 0 && t < this->GlobalTMin && ptId >= 0)
                {
                    this->MarkPickedData(path, t, ptX, mapper, dataSet, -1);
                    this->PointId = ptId;
                    // vtkLog(INFO, << "Picked point id: " << ptId
                    //              << " at: " << ptX[0] << ", " << ptX[1] << ", " << ptX[2]
                    //              << " t: " << t);
                    return t;
                }
            }

            return -1.0; // no pick
        }
    }
    
    return -1.0;
}
