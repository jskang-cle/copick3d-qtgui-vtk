#pragma once

#include <vtkActor.h>
#include <vtkNew.h>

namespace copick3d::qtgui::graphics
{

class PointCloudActor : public vtkActor
{
public:
    static PointCloudActor* New();
    vtkTypeMacro(PointCloudActor, vtkActor);
    void PrintSelf(ostream& os, vtkIndent indent) override;
};

} // namespace copick3d::qtgui::graphics
