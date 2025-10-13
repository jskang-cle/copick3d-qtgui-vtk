#include "PointCloudActor.hpp"

#include "copick3d/core/frame.hpp"

#include "qdebug.h"

#include <vtkObjectFactory.h>

#include <vtkFloatArray.h>
#include <vtkPoints.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkTrivialProducer.h>
#include <vtkPolyDataMapper.h>
#include <vtkOpenGLPolyDataMapper.h>
#include <vtkPlaneSource.h>
#include <vtkArrowSource.h>
#include <vtkGlyph3DMapper.h>
#include <vtkOpenGLGlyph3DMapper.h>
#include <vtkElevationFilter.h>
#include <vtkVertexGlyphFilter.h>
#include <vtkStatisticalOutlierRemoval.h>

#include <vtkArrayDispatch.h>

#include <vtkProperty.h>
#include <vtkShaderProperty.h>
#include <vtkUniforms.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkViewport.h>
#include <vtkCamera.h>

#include "vtk/DepthFilter.hpp"

namespace copick3d::qtgui::graphics
{
    
vtkStandardNewMacro(PointCloudActor);

PointCloudActor::PointCloudActor()
{
    this->InitializePipeline();
    this->UpdatePipeline();

    this->Modified();
}

PointCloudActor::~PointCloudActor() = default;

void PointCloudActor::PrintSelf(ostream& os, vtkIndent indent)
{
    this->Superclass::PrintSelf(os, indent);
    os << indent << "PointCloudActor: " << this << "\n";
    os << indent << "PointSize: " << this->PointSize << "\n";
    os << indent << "ColorMode: " << this->ColorMode << "\n";
    os << indent << "ColorMap: " << this->ColorMap << "\n";
}

void PointCloudActor::InitializePipeline()
{    
    this->Positions->SetName("Positions");
    this->Positions->SetNumberOfComponents(3);

    this->Normals->SetName("Normals");
    this->Normals->SetNumberOfComponents(3);

    this->Colors->SetName("Colors");
    this->Colors->SetNumberOfComponents(3);

    this->Points->SetData(this->Positions);
    qDebug() << "PointCloudActor::InitializePipeline(): Number of points =" << this->Points->GetNumberOfPoints();

    //    PolyDataProducer(PolyData(Points, Normals, Colors)) 
    // -> SORFilter  (noise filtering, disabled in debug build due to performance)
    // -> ElevFilter (add depth scalar array to polydata)

    this->PolyData->SetPoints(this->Points);
    this->PolyData->GetPointData()->SetNormals(this->Normals);
    this->PolyData->GetPointData()->SetScalars(this->Colors);

    this->PolyDataProducer->SetOutput(this->PolyData);
    
#if defined(DEBUG) || defined(_DEBUG)
    this->DepthFilter->SetInputConnection(this->PolyDataProducer->GetOutputPort());
#else
    this->SORFilter->SetInputConnection(this->PolyDataProducer->GetOutputPort());
    this->SORFilter->SetSampleSize(25);
    this->SORFilter->SetStandardDeviationFactor(1.0);
    this->DepthFilter->SetInputConnection(this->SORFilter->GetOutputPort());
#endif

    //    VertexGlyphFilter (map points to vertex) 
    // -> VertexGlyphMapper (map points to GL_POINTS)
    // -> this (actor)

    this->VertexGlyphFilter->SetInputConnection(this->DepthFilter->GetOutputPort());

    this->VertexGlyphMapper->SetInputConnection(this->VertexGlyphFilter->GetOutputPort());
    this->VertexGlyphMapper->SetScalarModeToUsePointFieldData();

    // this->VertexGlyphMapper->SetColorModeToDirectScalars();
    this->VertexGlyphMapper->SetScalarVisibility(0);

    this->GetProperty()->SetPointSize(1);

    // this->GetProperty()->SetPointSize(1);
    // this->GetProperty()->SetColor(1.0, 1.0, 1.0);
    // this->GetProperty()->SetInterpolationToFlat();
    // this->GetProperty()->SetLighting(false);

    //    RectSource or ArrowSource (shape generation)
    // -> Glyph3DMapper (map points to shape)
    // -> this (actor)

    this->RectSource->SetResolution(1, 1);
    this->RectSource->SetOrigin(0.0, -0.5, -0.5);
    this->RectSource->SetPoint1(0.0, 0.5, -0.5);
    this->RectSource->SetPoint2(0.0, -0.5, 0.5);

    this->ArrowSource->SetTipResolution(2);
    this->ArrowSource->SetShaftResolution(1);

    this->Glyph3DMapper->SetInputConnection(this->DepthFilter->GetOutputPort());
    this->Glyph3DMapper->SetSourceConnection(this->RectSource->GetOutputPort());
    this->Glyph3DMapper->SetOrientationMode(vtkGlyph3DMapper::DIRECTION);
    this->Glyph3DMapper->SetOrientationArray("Normals");

    this->SetMapper(this->Glyph3DMapper);
}

void PointCloudActor::UpdatePipeline()
{
    qDebug() << "PointCloudActor::UpdatePipeline()";

    auto shaderProp = this->GetShaderProperty();
    shaderProp->ClearAllShaderReplacements();
    vtkMapper* mapper;
    mapper = this->Glyph3DMapper;

    if (this->RenderType == POINT_RENDER_TYPE_VERTEX)
    {
        mapper = this->VertexGlyphMapper;
        this->VertexGlyphMapper->SetUseProgramPointSize(false);
        this->GetProperty()->SetPointSize(this->PointSize);
        qDebug() << "PointCloudActor::UpdatePipeline(): Set point size =" << this->PointSize;
        
        // shaderProp->AddVertexShaderReplacement(
        //     "//VTK::ValuePass::Impl",  // replace the normal block
        //     true,                      // before the standard replacements
        //     "gl_PointSize = clamp(500.0 / gl_Position.w, 1.0, 500.0);\n"
        //     "///VTK::ValuePass::Impl\n", // we still want the default
        //     false                        // only do it once
        // );
    }
    else
    {
        double pointScaleUniform = 1.0;
        if (this->RenderType == POINT_RENDER_TYPE_SQUARE)
        {
            this->Glyph3DMapper->SetSourceConnection(this->RectSource->GetOutputPort());

            // this->Glyph3DMapper->SetScaleFactor(this->PointSize * this->PointScale); 
            /// this rebuilds entire VBO every time which is very slow
            /// so we use a uniform to scale in the vertex shader instead

            pointScaleUniform = this->PointSize * this->PointScale;
        }
        else if (this->RenderType == POINT_RENDER_TYPE_ARROW)
        {
            this->Glyph3DMapper->SetSourceConnection(this->ArrowSource->GetOutputPort());

            // this->Glyph3DMapper->SetScaleFactor(this->PointSize * this->PointScale * 2.0);

            pointScaleUniform = this->PointSize * this->PointScale * 2.0;
        }

        shaderProp->AddVertexShaderReplacement(
            "//VTK::Normal::Dec",  // replace the normal block
            true,                      // before the standard replacements
            "uniform float pointScale;\n"
            "//VTK::Normal::Dec",
            false                        // only do it once
        );

        shaderProp->AddVertexShaderReplacement(
            "vec4 vertex = GCMCMatrix * vertexMC;",  // replace the vertex position calculation
            false,                      // after the standard replacements
            "vec4 vertex = GCMCMatrix * vec4(vertexMC.xyz * pointScale, vertexMC.w);",
            false                        // only do it once
        );
        
        shaderProp->GetVertexCustomUniforms()->SetUniformf("pointScale", pointScaleUniform);
        mapper = this->Glyph3DMapper;
    }

    if (this->ColorMode == POINT_COLOR_MODE_RGB)
    {
        mapper->SelectColorArray("Colors");
        mapper->SetScalarModeToUsePointData();
        mapper->SetColorModeToDirectScalars();
        mapper->SetScalarVisibility(1);
        
        this->GetProperty()->SetLighting(false);

        if (this->RenderType == POINT_RENDER_TYPE_VERTEX)
        {
            shaderProp->AddVertexShaderReplacement(
                "= scalarColor", // replace the color implementation block
                false,                 // after the standard replacements
                "= vec4(scalarColor.bgr, 1.0)",
                false // only do it once
            );
        }
        else
        {
            shaderProp->AddVertexShaderReplacement(
                "=  glyphColor;", // replace the color implementation block
                false,                 // after the standard replacements
                "=  vec4(glyphColor.bgr, 1.0);",
                false // only do it once
            );
        }
    }
    else if (this->ColorMode == POINT_COLOR_MODE_NORMAL)
    {
        // enable lighting. this also binds normal buffer to shader
        this->GetProperty()->SetLighting(true);
        
        // disable default scalar coloring
        mapper->SetScalarVisibility(0);

        if (this->RenderType == POINT_RENDER_TYPE_VERTEX)
        {
            // add color output variable
            shaderProp->AddVertexShaderReplacement(
                "//VTK::Color::Dec", // replace the normal block
                true,                 // before the standard replacements
                "out vec4 vertexColorVSOutput;",
                false // only do it once
            );
            // calculate color from normal in vertex shader
            shaderProp->AddVertexShaderReplacement(
                "//VTK::Color::Impl", // replace the color implementation block
                true,                 // before the standard replacements
                "vertexColorVSOutput = vec4(normalMC.xyz * 0.5 + 0.5, 1.0);",
                false // only do it once
            );
            // use the color calculated in vertex shader in fragment shader
            shaderProp->AddFragmentShaderReplacement(
                "//VTK::Color::Dec", // replace the normal block
                true,                 // before the standard replacements
                "in vec4 vertexColorVSOutput;\n"
                "//VTK::Color::Dec",
                false // only do it once
            );
            // set fragment color to the color calculated in vertex shader
            shaderProp->AddFragmentShaderReplacement(
                "//VTK::Light::Impl", // replace the color implementation block
                true,                 // before the standard replacements
                "fragOutput0 = vec4(vertexColorVSOutput.rgb, opacity);",
                false // only do it once
            );
        }
        else
        {
            shaderProp->AddVertexShaderReplacement(
                "=  glyphColor;", // replace the color implementation block
                false,                 // after the standard replacements
                "=  vec4(normalize(vec3(glyphNormalMatrix[0])) * 0.5 + 0.5, 1.0);",
                false // only do it once
            );
            
            shaderProp->AddFragmentShaderReplacement(
                "//VTK::Light::Impl", // replace the color implementation block
                true,                 // before the standard replacements
                "fragOutput0 = vec4(vertexColorVSOutput.rgb, opacity);",
                false // only do it once
            );
        }
    }
    else if (this->ColorMode == POINT_COLOR_MODE_DEPTH)
    {
        this->GetProperty()->SetLighting(false);

        mapper->SelectColorArray("Depth");
        mapper->SetColorModeToDirectScalars();
        mapper->SetScalarModeToUsePointFieldData();
        mapper->SetScalarVisibility(1);
    }
    else if (this->ColorMode == POINT_COLOR_MODE_SOLID)
    {
        mapper->SetScalarVisibility(0);
        this->GetProperty()->SetLighting(true);
    }

    if (this->Mapper != mapper)
    {
        this->SetMapper(mapper);
    }

    this->BuildTime.Modified();
}

int PointCloudActor::RenderOpaqueGeometry(vtkViewport* viewport)
{
    if (this->BuildTime < this->GetMTime() ||
        this->BuildTime < this->Positions->GetMTime() ||
        this->BuildTime < this->Normals->GetMTime() ||
        this->BuildTime < this->Colors->GetMTime())
    {
        this->UpdatePipeline();
    }
    
    return Superclass::RenderOpaqueGeometry(viewport);

    // vtkRenderer* ren = static_cast<vtkRenderer*>(viewport);

    // this->Property->Render(this, ren);
    // this->Mapper->Render(ren, this);
    // this->Property->PostRender(this, ren);
}

void PointCloudActor::SetFrame(QSharedPointer<copick3d::Frame> frame)
{
    if (this->FramePtr == frame)
        return;

    this->FramePtr = frame;

    if (this->FramePtr && this->FramePtr->GetPointCloud())
    {
        const auto& pointCloud = this->FramePtr->GetPointCloud();

        const auto& points = pointCloud->Points();
        const auto& normals = pointCloud->Normals();
        const auto& colors = pointCloud->Colors();

        this->Positions->SetVoidArray((void*)points.Data(), points.Size() * 3, 1);
        this->Normals->SetVoidArray((void*)normals.Data(), normals.Size() * 3, 1);
        this->Colors->SetVoidArray((void*)colors.Data(), colors.Size() * 3, 1);
    }
    else
    {
        this->Positions->SetNumberOfTuples(0);
        this->Normals->SetNumberOfTuples(0);
        this->Colors->SetNumberOfTuples(0);
        this->PointScale = 1.0;
    }

    // calculate point scale based on frame width (1224 as default width)

    this->Positions->Modified();
    this->Normals->Modified();
    this->Colors->Modified();

    this->PolyData->GetPointData()->SetActiveNormals("Normals");
    this->PolyData->GetPointData()->SetActiveScalars("Colors");

    if (this->FramePtr)
    {
        const double defaultWidth = 1224.0;
        const double frameWidth = static_cast<double>(this->FramePtr->GetWidth());
        const double* bounds = this->PolyData->GetBounds();
        this->PointScale = 0.0007 * (bounds[1] - bounds[0]) / (frameWidth / defaultWidth);
    }
    else
    {
        this->PointScale = 1.0;
    }

    this->Modified();
}

} // namespace copick3d::qtgui::graphics
