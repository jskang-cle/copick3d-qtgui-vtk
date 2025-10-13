#include "DepthFilter.hpp"

#include <vtkObjectFactory.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkDataSet.h>
#include <vtkDataArray.h>
#include <vtkPointData.h>
#include <vtkFloatArray.h>

#include <qdebug.h>

vtkStandardNewMacro(DepthFilter);

int DepthFilter::RequestData(vtkInformation* vtkNotUsed(request),
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector)
{
    qDebug() << "DepthFilter::RequestData called";

    // get the info objects
    vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
    vtkInformation* outInfo = outputVector->GetInformationObject(0);

    // get the input and output
    vtkDataSet* input = vtkDataSet::SafeDownCast(
        inInfo->Get(vtkDataObject::DATA_OBJECT()));
    vtkDataSet* output = vtkDataSet::SafeDownCast(
        outInfo->Get(vtkDataObject::DATA_OBJECT()));

    if (input == nullptr || output == nullptr)
    {
        vtkErrorMacro("Invalid input or output data.");
        return 0;
    }

    // Create an array to hold depth values
    // which is just the Z coordinate in camera space mapped to [0, 1]
    vtkNew<vtkFloatArray> depthArray;
    depthArray->SetName("Depth");
    depthArray->SetNumberOfComponents(1);
    depthArray->SetNumberOfTuples(input->GetNumberOfPoints());

    double* bounds = input->GetBounds();
    double zMin = bounds[4];
    double zMax = bounds[5];

    // Compute depth for each point
    for (vtkIdType i = 0; i < input->GetNumberOfPoints(); ++i)
    {
        double point[3];
        input->GetPoint(i, point);
        // Normalize depth to [0, 1]
        float depth = static_cast<float>((zMax - point[2]) / (zMax - zMin));
        depthArray->SetValue(i, depth);
    }

    // Add the depth array to the output point data
    output->ShallowCopy(input);
    output->GetPointData()->AddArray(depthArray);

    return 1;
}