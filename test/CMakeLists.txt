itk_module_test()

set(ElastixTests
  itkElastixRegistrationMethodTest.cxx
  )

CreateTestDriver(Elastix "${Elastix-Test_LIBRARIES}" "${ElastixTests}")

itk_add_test(NAME itkElastixRegistrationMethodTest
  COMMAND ElastixTestDriver
  --compareNumberOfPixelsTolerance 25
  --compareRadiusTolerance 3
  --compareIntensityTolerance 50
  --compare
    ${ITK_TEST_OUTPUT_DIR}/itkElastixRegistrationMethodTestOutput.mha
    DATA{Baseline/itkElastixRegistrationMethodTestOutput.mha}
  itkElastixRegistrationMethodTest
    DATA{Input/CT_3D_lung_fixed.mha}
    DATA{Input/CT_3D_lung_moving.mha}
    ${ITK_TEST_OUTPUT_DIR}/itkElastixRegistrationMethodTestOutput.mha
  )
