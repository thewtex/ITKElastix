#==========================================================================
#
#   Copyright NumFOCUS
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#          https://www.apache.org/licenses/LICENSE-2.0.txt
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.
#
#==========================================================================*/

import itk

Dimension = 2
ImageType = itk.Image[itk.F, Dimension]
MeshType = itk.Mesh[itk.F, Dimension]

parameter_map = {
    "Direction": ("1", "0", "0", "1"),
    "FixedImageDimension": ("2",),
    "Index": ("0", "0"),
    "MovingImageDimension": ("2",),
    "NumberOfParameters": ("2",),
    "Origin": ("0", "0"),
    "Size": ("1", "1"),
    "Spacing": ("1", "1"),
    "Transform": ("TranslationTransform",),
    "TransformParameters": ("1.0", "-2.0"),
}

parameter_object = itk.ParameterObject.New()
parameter_object.SetParameterMap(parameter_map)

input_mesh = MeshType.New()
input_mesh.SetPoint(0, [0.0, 0.0])
input_mesh.SetPoint(1, [1.0, 2.0])

# Test 1: Object-oriented interface, mesh-only (no moving image)
transformix = itk.TransformixFilter[ImageType].New()
transformix.SetInputMesh(input_mesh)
transformix.SetTransformParameterObject(parameter_object)
transformix.UpdateLargestPossibleRegion()

output_mesh = transformix.GetOutputMesh()
assert output_mesh is not None, "GetOutputMesh() returned None"
assert output_mesh.GetNumberOfPoints() == 2

p0 = output_mesh.GetPoint(0)
p1 = output_mesh.GetPoint(1)
assert abs(p0[0] - 1.0) < 1e-6 and abs(p0[1] - (-2.0)) < 1e-6, f"Unexpected p0: {p0}"
assert abs(p1[0] - 2.0) < 1e-6 and abs(p1[1] - 0.0) < 1e-6, f"Unexpected p1: {p1}"

print("OO interface mesh-only test passed")

# Test 2: Convenience function with ParameterObject
output_mesh2 = itk.transformix_mesh(input_mesh, parameter_object)
assert output_mesh2 is not None, "transformix_mesh returned None"
assert output_mesh2.GetNumberOfPoints() == 2

p0 = output_mesh2.GetPoint(0)
p1 = output_mesh2.GetPoint(1)
assert abs(p0[0] - 1.0) < 1e-6 and abs(p0[1] - (-2.0)) < 1e-6
assert abs(p1[0] - 2.0) < 1e-6 and abs(p1[1] - 0.0) < 1e-6

print("Convenience function test passed")

# Test 3: Convenience function with dict parameter
output_mesh3 = itk.transformix_mesh(input_mesh, parameter_map)
assert output_mesh3 is not None, "transformix_mesh with dict returned None"
assert output_mesh3.GetNumberOfPoints() == 2

p0 = output_mesh3.GetPoint(0)
p1 = output_mesh3.GetPoint(1)
assert abs(p0[0] - 1.0) < 1e-6 and abs(p0[1] - (-2.0)) < 1e-6
assert abs(p1[0] - 2.0) < 1e-6 and abs(p1[1] - 0.0) < 1e-6

print("Dict parameter test passed")
print("All mesh transformation tests passed")
