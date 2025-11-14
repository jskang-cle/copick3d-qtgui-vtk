#include "DepthFilter.hpp"

#include <vtkArrayDispatch.h>
#include <vtkCellData.h>
#include <vtkObjectFactory.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkDataSet.h>
#include <vtkDataArray.h>
#include <vtkPoints.h>
#include <vtkPointData.h>
#include <vtkPointSet.h>
#include <vtkFloatArray.h>
#include <vtkSMPTools.h>

#include <vtkLogger.h>

#include <chrono>

vtkStandardNewMacro(DepthFilter);

template <class PointArrayT>
class DepthFilterAlgorithm
{
public:
    float zMin;
    float zMax;
    PointArrayT* PointArray;
    vtkFloatArray* DepthArray;

    DepthFilterAlgorithm(float zmin, float zmax,
        PointArrayT* points, vtkFloatArray* depthArray)
        : zMin(zmin), zMax(zmax),
          PointArray(points), DepthArray(depthArray) {}

    void operator()(vtkIdType startId, vtkIdType endId)
    {
        // Using vtk::DataArrayRange to avoid float->double->float conversions
        const auto range = vtk::DataArrayTupleRange<3>(this->PointArray, startId, endId);
        float* depthIter = vtk::DataArrayValueRange(this->DepthArray, startId, endId).begin();

        using ConstTupleRef = typename decltype(range)::ConstTupleReferenceType;
        using ConstComponentRef = typename decltype(range)::ConstComponentReferenceType;

        for (ConstTupleRef tuple : range)
        {
            ConstComponentRef z = tuple[2];
            float depth = static_cast<float>((this->zMax - z) / (this->zMax - this->zMin));
            *depthIter++ = depth;
        }
    }
};

struct DepthFilterWorker
{
    template <typename PointArrayT>
    void operator()(PointArrayT* points, float zmin, float zmax,
        vtkFloatArray* depthArray)
    {
        DepthFilterAlgorithm<PointArrayT> algo(zmin, zmax, points, depthArray);
        vtkSMPTools::For(0, points->GetNumberOfTuples(), algo);
    }
};

int DepthFilter::RequestData(vtkInformation* vtkNotUsed(request),
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector)
{
    // get the info objects
    vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
    vtkInformation* outInfo = outputVector->GetInformationObject(0);

    // get the input and output
    vtkDataSet* input = vtkDataSet::SafeDownCast(
        inInfo->Get(vtkDataObject::DATA_OBJECT()));
    vtkDataSet* output = vtkDataSet::SafeDownCast(
        outInfo->Get(vtkDataObject::DATA_OBJECT()));

    output->CopyStructure(input);

    if (input->GetNumberOfPoints() < 1)
    {
        vtkDebugMacro(<< "No input!");
        return 1;
    }

    vtkPointSet* points = vtkPointSet::SafeDownCast(input);

    if (!points)
    {
        return 1;
    }

    vtkPoints* inputPoints = points->GetPoints();
    vtkDataArray* pointData = inputPoints->GetData();
    vtkIdType numPoints = input->GetNumberOfPoints();

    if (pointData->GetNumberOfComponents() != 3)
    {
        vtkErrorMacro("Input points must have exactly 3 components.");
        return 0;
    }

    // Create an array to hold depth values
    // which is just the Z coordinate in camera space mapped to [0, 1]
    vtkNew<vtkFloatArray> depthArray;
    depthArray->SetName("Depth");
    depthArray->SetNumberOfComponents(1);
    depthArray->SetNumberOfTuples(numPoints);

    double* bounds = points->GetBounds();
    float zMin = bounds[4];
    float zMax = bounds[5];

    // std::chrono::high_resolution_clock::time_point startTime = std::chrono::high_resolution_clock::now();

    DepthFilterWorker worker;
    using Dispatcher = vtkArrayDispatch::DispatchByValueType<vtkArrayDispatch::Reals>;
    if (!Dispatcher::Execute(pointData, worker, zMin, zMax, depthArray))
    {
        worker(pointData, zMin, zMax, depthArray);
    }

    // std::chrono::high_resolution_clock::time_point endTime = std::chrono::high_resolution_clock::now();
    // std::chrono::duration<double, std::milli> duration = endTime - startTime;
    // vtkLogF(INFO, "Depth computation took %.3f ms", duration.count());

    // Add the depth array to the output point data
    output->GetPointData()->PassData(input->GetPointData());
    output->GetCellData()->PassData(input->GetCellData());
    output->GetPointData()->AddArray(depthArray);

    return 1;
}
