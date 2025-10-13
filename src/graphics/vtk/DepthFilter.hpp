#pragma once

#include "vtkDataSetAlgorithm.h"

class DepthFilter : public vtkDataSetAlgorithm
{
public:
    static DepthFilter* New();
    vtkTypeMacro(DepthFilter, vtkDataSetAlgorithm);

protected:
    DepthFilter() = default;
    ~DepthFilter() override = default;

    int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
    DepthFilter(const DepthFilter&) = delete;
    void operator=(const DepthFilter&) = delete;
};
