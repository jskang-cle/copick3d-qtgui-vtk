// #pragma once

// #include <vtkOpenGLPolyDataMapper.h>

// #include <vtkShaderProgram.h>

// #include <vtkOpenGLVertexBufferObject.h>
// #include <vtkOpenGLVertexBufferObjectGroup.h>

// class PointCloudMapper : public vtkOpenGLPolyDataMapper
// {
// public:
//     static PointCloudMapper *New();
//     vtkTypeMacro(PointCloudMapper, vtkOpenGLPolyDataMapper);
//     void PrintSelf(ostream &os, vtkIndent indent) override;

//     void Render(vtkRenderer *ren, vtkActor *act) override;

// protected:
//     PointCloudMapper();
//     ~PointCloudMapper() override;

//     // Description:
//     // Create the basic shaders before replacement
//     void GetShaderTemplate(std::map<vtkShader::Type, vtkShader *> shaders, vtkRenderer *ren, vtkActor *act) override
//     {
//         this->Superclass::GetShaderTemplate(shaders, ren, act);

//         shaders[vtkShader::Geometry]->SetSource(
//             R"(

// #version 330 core
// layout(points) in;
// layout(triangle_strip, max_vertices = 4) out;

//             )");
//     }

//     // Description:
//     // Perform string replacements on the shader templates
//     void ReplaceShaderColor(std::map<vtkShader::Type, vtkShader *> shaders, vtkRenderer *ren, vtkActor *act) override
//     {

//     }

//     void ReplaceShaderPositionVC(std::map<vtkShader::Type, vtkShader *> shaders, vtkRenderer *ren, vtkActor *act) override
//     {
//         std::string VSSource = shaders[vtkShader::Vertex]->GetSource();
//         std::string FSSource = shaders[vtkShader::Fragment]->GetSource();

//         vtkShaderProgram::Substitute(VSSource, "//VTK::Camera::Dec", "uniform mat4 MCDCMatrix;");
//         vtkShaderProgram::Substitute(VSSource, "//VTK::PositionVC::Impl", "  gl_Position = MCDCMatrix * vertexMC;\n");
//     }

//     // Description:
//     // Set the shader parameters related to the Camera
//     void SetCameraShaderParameters(vtkOpenGLHelper &cellBO, vtkRenderer *ren, vtkActor *act) override;

//     // Description:
//     // Set the shader parameters related to the actor/mapper
//     void SetMapperShaderParameters(vtkOpenGLHelper &cellBO, vtkRenderer *ren, vtkActor *act) override;

//     // Description:
//     // Does the VBO/IBO need to be rebuilt
//     bool GetNeedToRebuildBufferObjects(vtkRenderer *ren, vtkActor *act) override
//     {
//         if (this->VBOBuildTime < this->GetMTime() || this->VBOBuildTime < act->GetMTime() ||
//             this->VBOBuildTime < this->CurrentInput->GetMTime())
//         {
//             return true;
//         }
//         return false;
//     }

//     // Description:
//     // Update the VBO to contain point based values
//     void BuildBufferObjects(vtkRenderer *ren, vtkActor *act) override
//     {
//         vtkPolyData *poly = this->CurrentInput;
//         if (!poly)
//         {
//             return;
//         }

//         this->VBOs->CacheDataArray("vertexMC", poly->GetPoints()->GetData(), ren, VTK_FLOAT);

//         vtkDataArray *normals = poly->GetPointData()->GetNormals();
//         if (normals)
//         {
//             this->VBOs->CacheDataArray("normalMC", normals, ren, VTK_FLOAT);
//         }

//     }

//     void RenderPieceDraw(vtkRenderer *ren, vtkActor *act) override;

//     // Description:
//     // Does the shader source need to be recomputed
//     bool GetNeedToRebuildShaders(vtkOpenGLHelper &cellBO, vtkRenderer *ren, vtkActor *act) override;

// private:
//     PointCloudMapper(const PointCloudMapper &) = delete;
//     void operator=(const PointCloudMapper &) = delete;
// };

// vtkStandardNewMacro(PointCloudMapper);
