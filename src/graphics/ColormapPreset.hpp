#pragma once

#include <vtkSmartPointer.h>
#include <vtkLookupTable.h>

#include <algorithm>

namespace copick3d::qtgui::graphics {

enum class ColormapPreset {
    Gray,
    Plasma,
    Viridis,
    Twilight,
    Turbo,
};

vtkSmartPointer<vtkLookupTable> GetColormapLookupTable(ColormapPreset preset);

} // namespace copick3d::qtgui::graphics
