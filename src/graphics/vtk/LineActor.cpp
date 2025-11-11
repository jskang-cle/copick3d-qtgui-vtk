#include "LineActor.hpp"

#include <vtkSetGet.h>
#include <vtkPoints.h>
#include <vtkCellArray.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkPolyDataMapper2D.h>
#include <vtkProperty.h>
#include <vtkProperty2D.h>
#include <vtkLine.h>

#include <vtkLogger.h>

#include <array>

vtkStandardNewMacro(LineActor);

LineActor::LineActor()
{
	this->LinePolyData->SetPoints(this->LinePoints);
	this->LinePolyData->SetLines(this->LineCells);
	this->LineMapper->SetInputData(this->LinePolyData);
	this->SetMapper(this->LineMapper);
}

void LineActor::BuildLine()
{
	double* p1 = this->Point1;
	double* p2 = this->Point2;

	this->LinePoints->Reset();
	this->LineCells->Reset();

	this->LinePoints->InsertNextPoint(p1);
	this->LinePoints->InsertNextPoint(p2);
	this->LineCells->InsertNextCell(2, std::array<vtkIdType, 2>{0, 1}.data());

    vtkLog(INFO, "LineActor::BuildLine - Point1: (" << this->Point1[0] << ", " << this->Point1[1] << ", " << this->Point1[2] << "), "
             << "Point2: (" << this->Point2[0] << ", " << this->Point2[1] << ", " << this->Point2[2] << ")");
}

