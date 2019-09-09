/*=========================================================================
 *
 *  Copyright Insight Software Consortium
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *         http://www.apache.org/licenses/LICENSE-2.0.txt
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *=========================================================================*/
/*=========================================================================
 *
 *  Copyright UMC Utrecht and contributors
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0.txt
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *=========================================================================*/
#ifndef itkElastixFilter_hxx
#define itkElastixFilter_hxx

#include "itkElastixFilter.h"

namespace itk
{

/**
 * ********************* Constructor *********************
 */

template< typename TFixedImage, typename TMovingImage >
ElastixFilter< TFixedImage, TMovingImage >
::ElastixFilter()
{
  this->SetPrimaryInputName( "FixedImage" );
  this->SetPrimaryOutputName( "ResultImage" );

  this->AddRequiredInputName( "FixedImage" );
  this->AddRequiredInputName( "MovingImage" );
  this->AddRequiredInputName( "ParameterObject" );

  this->m_InitialTransformParameterFileName = "";
  this->m_FixedPointSetFileName             = "";
  this->m_MovingPointSetFileName            = "";

  this->m_OutputDirectory = "";
  this->m_LogFileName     = "";

  this->m_LogToConsole = false;
  this->m_LogToFile    = false;

  this->m_NumberOfThreads = 0;

  ParameterObjectPointer defaultParameterObject = elastix::ParameterObject::New();
  defaultParameterObject->AddParameterMap( elastix::ParameterObject::GetDefaultParameterMap( "translation" ) );
  defaultParameterObject->AddParameterMap( elastix::ParameterObject::GetDefaultParameterMap( "affine" ) );
  defaultParameterObject->AddParameterMap( elastix::ParameterObject::GetDefaultParameterMap( "bspline" ) );
  this->SetParameterObject( defaultParameterObject );

  this->m_InputUID = 0;
} // end Constructor


/**
 * ********************* GenerateData *********************
 */

template< typename TFixedImage, typename TMovingImage >
void
ElastixFilter< TFixedImage, TMovingImage >
::GenerateData()
{
  // Force compiler to instantiate the image dimensions, otherwise we may get
  //   Undefined symbols for architecture x86_64:
  //     "elastix::ElastixFilter<itk::Image<float, 2u> >::FixedImageDimension"
  // on some platforms.
  const unsigned int fixedImageDimension = FixedImageDimension;
  const unsigned int movingImageDimension = MovingImageDimension;

  DataObjectContainerPointer fixedImageContainer  = DataObjectContainerType::New();
  DataObjectContainerPointer movingImageContainer = DataObjectContainerType::New();
  DataObjectContainerPointer fixedMaskContainer   = nullptr;
  DataObjectContainerPointer movingMaskContainer  = nullptr;
  DataObjectContainerPointer resultImageContainer = nullptr;
  ElastixMainObjectPointer   transform            = nullptr;
  ParameterMapVectorType     transformParameterMapVector;
  FlatDirectionCosinesType   fixedImageOriginalDirection;

  // Split inputs into separate containers
  const NameArrayType inputNames = this->GetInputNames();
  for( unsigned int i = 0; i < inputNames.size(); ++i )
  {
    if( this->IsInputOfType( "FixedImage", inputNames[ i ] ) )
    {
      fixedImageContainer->push_back( this->GetInput( inputNames[ i ] ) );
      continue;
    }

    if( this->IsInputOfType( "MovingImage", inputNames[ i ] ) )
    {
      movingImageContainer->push_back( this->GetInput( inputNames[ i ] ) );
      continue;
    }

    if( this->IsInputOfType( "FixedMask", inputNames[ i ] ) )
    {
      if( fixedMaskContainer.IsNull() )
      {
        fixedMaskContainer = DataObjectContainerType::New();
      }

      fixedMaskContainer->push_back( this->GetInput( inputNames[ i ] ) );
      continue;
    }

    if( this->IsInputOfType( "MovingMask", inputNames[ i ] ) )
    {
      if( movingMaskContainer.IsNull() )
      {
        movingMaskContainer = DataObjectContainerType::New();
      }

      movingMaskContainer->push_back( this->GetInput( inputNames[ i ] ) );
    }
  }

  // Set ParameterMap
  ParameterObjectPointer parameterObject    = itkDynamicCastInDebugMode< elastix::ParameterObject * >( this->GetInput( "ParameterObject" ) );
  ParameterMapVectorType parameterMapVector = parameterObject->GetParameterMap();

  if( parameterMapVector.size() == 0 )
  {
    itkExceptionMacro( "Empty parameter map in parameter object." );
  }

  // Elastix must always write result image to guarantee that the ITK pipeline is in a consistent state
  parameterMapVector[ parameterMapVector.size() - 1 ][ "WriteResultImage" ] = ParameterValueVectorType( 1, "true" );

  // Setup argument map
  ArgumentMapType argumentMap;

  if( !this->m_InitialTransformParameterFileName.empty() )
  {
    argumentMap.insert( ArgumentMapEntryType( "-t0", this->m_InitialTransformParameterFileName ) );
  }

  if( !this->m_FixedPointSetFileName.empty() )
  {
    argumentMap.insert( ArgumentMapEntryType( "-fp", this->m_FixedPointSetFileName ) );
  }

  if( !this->m_MovingPointSetFileName.empty() )
  {
    argumentMap.insert( ArgumentMapEntryType( "-mp", this->m_MovingPointSetFileName ) );
  }

  // Setup output directory
  if( this->GetOutputDirectory().empty() )
  {
    if( this->GetLogToFile() )
    {
      itkExceptionMacro( "LogToFileOn() requires an output directory to be specified." )
    }

    // There must be an "-out" as this is checked later in the code
    argumentMap.insert( ArgumentMapEntryType( "-out", "output_path_not_set" ) );
  }
  else
  {
    if( !itksys::SystemTools::FileExists( this->GetOutputDirectory() ) )
    {
      itkExceptionMacro( "Output directory \"" << this->GetOutputDirectory() << "\" does not exist." );
    }

    if( this->GetOutputDirectory()[ this->GetOutputDirectory().size() - 1 ] != '/'
      && this->GetOutputDirectory()[ this->GetOutputDirectory().size() - 1 ] != '\\' )
    {
      this->SetOutputDirectory( this->GetOutputDirectory() + "/" );
    }

    argumentMap.insert( ArgumentMapEntryType( "-out", this->GetOutputDirectory() ) );
  }

  // Setup log file
  std::string logFileName;
  if( this->GetLogToFile() )
  {
    if( this->GetLogFileName().empty() )
    {
      logFileName = this->GetOutputDirectory() + "elastix.log";
    }
    else
    {
      logFileName = this->GetOutputDirectory() + this->GetLogFileName();
    }
  }

  // Set Number of threads
  if( this->m_NumberOfThreads > 0 )
  {
    argumentMap.insert( ArgumentMapEntryType( "-threads", std::to_string( this->m_NumberOfThreads ) ) );
  }

  // Setup xout
  if( elastix::xoutSetup( logFileName.c_str(), this->GetLogToFile(), this->GetLogToConsole() ) )
  {
    itkExceptionMacro( "Error while setting up xout" );
  }

  // Run the (possibly multiple) registration(s)
  for( unsigned int i = 0; i < parameterMapVector.size(); ++i )
  {
    // Set image dimension from input images (overrides user settings)
    parameterMapVector[ i ][ "FixedImageDimension" ]
      = ParameterValueVectorType( 1, std::to_string( fixedImageDimension ) ) ;
    parameterMapVector[ i ][ "MovingImageDimension" ]
      = ParameterValueVectorType( 1, std::to_string( movingImageDimension ) );
    parameterMapVector[ i ][ "ResultImagePixelType" ]
      = ParameterValueVectorType( 1, elastix::PixelType< typename TFixedImage::PixelType >::ToString() );

    // Initial transform parameter files are handled via arguments and enclosing loop, not InitialTransformParametersFileName
    if( parameterMapVector[ i ].find( "InitialTransformParametersFileName" ) != parameterMapVector[ i ].end() )
    {
      parameterMapVector[ i ][ "InitialTransformParametersFileName" ] = ParameterValueVectorType( 1, "NoInitialTransform" );
    }

    // Create new instance of ElastixMain
    ElastixMainPointer elastix = ElastixMainType::New();

    // Set elastix levels
    elastix->SetElastixLevel( i );
    elastix->SetTotalNumberOfElastixLevels( parameterMapVector.size() );

    // Set stuff we get from a previous registration
    elastix->SetInitialTransform( transform );
    elastix->SetFixedImageContainer( fixedImageContainer );
    elastix->SetMovingImageContainer( movingImageContainer );
    elastix->SetFixedMaskContainer( fixedMaskContainer );
    elastix->SetMovingMaskContainer( movingMaskContainer );
    elastix->SetResultImageContainer( resultImageContainer );
    elastix->SetOriginalFixedImageDirectionFlat( fixedImageOriginalDirection );

    // Start registration
    unsigned int isError = 0;
    try
    {
      isError = elastix->Run( argumentMap, parameterMapVector[ i ] );
    }
    catch( itk::ExceptionObject & e )
    {
      itkExceptionMacro( << "Errors occurred during registration: " << e.what() );
    }

    if( isError != 0 )
    {
      itkExceptionMacro( << "Internal elastix error: See elastix log (use LogToConsoleOn() or LogToFileOn())." );
    }

    // Get stuff in order to put it in the next registration
    transform                   = elastix->GetFinalTransform();
    fixedImageContainer         = elastix->GetFixedImageContainer();
    movingImageContainer        = elastix->GetMovingImageContainer();
    fixedMaskContainer          = elastix->GetFixedMaskContainer();
    movingMaskContainer         = elastix->GetMovingMaskContainer();
    resultImageContainer        = elastix->GetResultImageContainer();
    fixedImageOriginalDirection = elastix->GetOriginalFixedImageDirectionFlat();

    transformParameterMapVector.push_back( elastix->GetTransformParametersMap() );
    if( i > 0 )
    {
      transformParameterMapVector[ i ][ "InitialTransformParametersFileName" ]
        = ParameterValueVectorType( 1, std::to_string( i - 1 ) );
    }

    // TODO: Fix elastix corrupting default pixel value parameter
    transformParameterMapVector[ transformParameterMapVector.size() - 1 ][ "DefaultPixelValue" ]
      = parameterMapVector[ i ][ "DefaultPixelValue" ];
  } // End loop over registrations

  // Save result image
  if( resultImageContainer.IsNotNull() && resultImageContainer->Size() > 0 && resultImageContainer->ElementAt( 0 ).IsNotNull() )
  {
    this->GraftOutput( "ResultImage", resultImageContainer->ElementAt( 0 ) );
  }
  else
  {
    itkExceptionMacro( "Errors occured during registration: Could not read result image." );
  }

  // Save parameter map
  elastix::ParameterObject::Pointer transformParameterObject = elastix::ParameterObject::New();
  transformParameterObject->SetParameterMap( transformParameterMapVector );
  this->SetOutput( "TransformParameterObject", transformParameterObject );
}


/**
 * ********************* SetParameterObject *********************
 */

template< typename TFixedImage, typename TMovingImage >
void
ElastixFilter< TFixedImage, TMovingImage >
::SetParameterObject( ParameterObjectType * parameterObject )
{
  this->SetInput( "ParameterObject", parameterObject );
}


/**
 * ********************* GetParameterObject *********************
 */

template< typename TFixedImage, typename TMovingImage >
typename ElastixFilter< TFixedImage, TMovingImage >::ParameterObjectType *
ElastixFilter< TFixedImage, TMovingImage >
::GetParameterObject()
{
  return itkDynamicCastInDebugMode< ParameterObjectType * >( itk::ProcessObject::GetInput( "ParameterObject" ) );
}

/**
 * ********************* GetParameterObject *********************
 */

template< typename TFixedImage, typename TMovingImage >
const typename ElastixFilter< TFixedImage, TMovingImage >::ParameterObjectType *
ElastixFilter< TFixedImage, TMovingImage >
::GetParameterObject() const
{
  return itkDynamicCastInDebugMode< const ParameterObjectType * >( itk::ProcessObject::GetInput( "ParameterObject" ) );
}

/**
 * ********************* GetTransformParameterObject *********************
 */

template< typename TFixedImage, typename TMovingImage >
typename ElastixFilter< TFixedImage, TMovingImage >::ParameterObjectType *
ElastixFilter< TFixedImage, TMovingImage >
::GetTransformParameterObject()
{
  if( this->HasOutput( "TransformParameterObject") )
  {
    return itkDynamicCastInDebugMode< ParameterObjectType * >( itk::ProcessObject::GetOutput( "TransformParameterObject" ) );
  }

  itkExceptionMacro( "TransformParameterObject has not been generated. Update() ElastixFilter before requesting this output.")
}

/**
 * ********************* GetTransformParameterObject *********************
 */

template< typename TFixedImage, typename TMovingImage >
const typename ElastixFilter< TFixedImage, TMovingImage >::ParameterObjectType *
ElastixFilter< TFixedImage, TMovingImage >
::GetTransformParameterObject() const
{
  if( this->HasOutput( "TransformParameterObject") )
  {
    return itkDynamicCastInDebugMode< const ParameterObjectType * >( itk::ProcessObject::GetOutput( "TransformParameterObject" ) );
  }

  itkExceptionMacro( "TransformParameterObject has not been generated. Update() ElastixFilter before requesting this output.")
}


template< typename TFixedImage, typename TMovingImage >
void
ElastixFilter< TFixedImage, TMovingImage >
::SetFixedImage( TFixedImage * fixedImage )
{
  this->RemoveInputsOfType( "FixedImage" );
  this->SetInput( "FixedImage", fixedImage );
}


template< typename TFixedImage, typename TMovingImage >
void
ElastixFilter< TFixedImage, TMovingImage >
::AddFixedImage( TFixedImage * fixedImage )
{
  if( this->GetInput( "FixedImage" ) == ITK_NULLPTR )
  {
    this->SetFixedImage( fixedImage );
  }
  else
  {
    this->SetInput( this->MakeUniqueName( "FixedImage" ), fixedImage );
  }
}


template< typename TFixedImage, typename TMovingImage >
const typename ElastixFilter< TFixedImage, TMovingImage >::FixedImageType *
ElastixFilter< TFixedImage, TMovingImage >
::GetFixedImage() const
{
  if( this->GetNumberOfInputsOfType( "FixedImage" ) > 1 )
  {
    itkExceptionMacro( "Please provide an index when more than one fixed images are available." );
  }

  return itkDynamicCastInDebugMode< const TFixedImage * >( this->GetInput( "FixedImage" ) );
}


template< typename TFixedImage, typename TMovingImage >
const typename ElastixFilter< TFixedImage, TMovingImage >::FixedImageType *
ElastixFilter< TFixedImage, TMovingImage >
::GetFixedImage( const unsigned int index ) const
{
  unsigned int n = 0;
  NameArrayType inputNames = this->GetInputNames();
  for( unsigned int i = 0; i < inputNames.size(); ++i )
  {
    if( this->IsInputOfType( "FixedImage", inputNames[ i ] ) )
    {
      if( index == n )
      {
        return itkDynamicCastInDebugMode< const TFixedImage * >( this->GetInput( inputNames[ i ] ) );
      }

      n++;
    }
  }

  itkExceptionMacro( << "Index exceeds the number of fixed images (index: "
                     << index << ", "
                     << "number of fixed images: " << n << ")" );
}


/**
 * ********************* GetNumberOfFixedImages *********************
 */

template< typename TFixedImage, typename TMovingImage >
unsigned int
ElastixFilter< TFixedImage, TMovingImage >
::GetNumberOfFixedImages() const
{
  return this->GetNumberOfInputsOfType( "FixedImage" );
}


/**
 * ********************* SetMovingImage *********************
 */

template< typename TFixedImage, typename TMovingImage >
void
ElastixFilter< TFixedImage, TMovingImage >
::SetMovingImage( TMovingImage * movingImage )
{
  this->RemoveInputsOfType( "MovingImage" );
  this->SetInput( "MovingImage", movingImage );
} // end SetMovingImage()


/**
 * ********************* AddMovingImage *********************
 */

template< typename TFixedImage, typename TMovingImage >
void
ElastixFilter< TFixedImage, TMovingImage >
::AddMovingImage( TMovingImage * movingImage )
{
  if( this->GetInput( "MovingImage" ) == ITK_NULLPTR )
  {
    this->SetMovingImage( movingImage );
  }
  else
  {
    this->SetInput( this->MakeUniqueName( "MovingImage" ), movingImage );
  }
}


template< typename TFixedImage, typename TMovingImage >
const typename ElastixFilter< TFixedImage, TMovingImage >::MovingImageType *
ElastixFilter< TFixedImage, TMovingImage >
::GetMovingImage() const
{
  if( this->GetNumberOfInputsOfType( "MovingImage" ) > 1 )
  {
    itkExceptionMacro( "Please provide an index when more than one fixed images are available." );
  }

  return itkDynamicCastInDebugMode< const TMovingImage * >( this->GetInput( "MovingImage" ) );
}


template< typename TFixedImage, typename TMovingImage >
const typename ElastixFilter< TFixedImage, TMovingImage >::MovingImageType *
ElastixFilter< TFixedImage, TMovingImage >
::GetMovingImage( const unsigned int index ) const
{
  unsigned int  n          = 0;
  NameArrayType inputNames = this->GetInputNames();
  for( unsigned int i = 0; i < inputNames.size(); ++i )
  {
    if( this->IsInputOfType( "MovingImage", inputNames[ i ] ) )
    {
      if( index == n )
      {
        return itkDynamicCastInDebugMode< const TMovingImage * >( this->GetInput( inputNames[ i ] ) );
      }

      n++;
    }
  }

  itkExceptionMacro( << "Index exceeds the number of moving images (index: "
                     << index << ", "
                     << "number of moving images: " << n << ")" );
}


template< typename TFixedImage, typename TMovingImage >
unsigned int
ElastixFilter< TFixedImage, TMovingImage >
::GetNumberOfMovingImages() const
{
  return this->GetNumberOfInputsOfType( "MovingImage" );
}


template< typename TFixedImage, typename TMovingImage >
void
ElastixFilter< TFixedImage, TMovingImage >
::SetFixedMask( FixedMaskType * fixedMask )
{
  this->RemoveInputsOfType( "FixedMask" );
  this->SetInput( "FixedMask", fixedMask );
}


template< typename TFixedImage, typename TMovingImage >
void
ElastixFilter< TFixedImage, TMovingImage >
::AddFixedMask( FixedMaskType * fixedMask )
{
  this->SetInput( this->MakeUniqueName( "FixedMask" ), fixedMask );
}


template< typename TFixedImage, typename TMovingImage >
const typename ElastixFilter< TFixedImage, TMovingImage >::FixedMaskType *
ElastixFilter< TFixedImage, TMovingImage >
::GetFixedMask() const
{
  return itkDynamicCastInDebugMode< const FixedMaskType * >( this->GetInput( "FixedMask" ) );
}


template< typename TFixedImage, typename TMovingImage >
const typename ElastixFilter< TFixedImage, TMovingImage >::FixedMaskType *
ElastixFilter< TFixedImage, TMovingImage >
::GetFixedMask( const unsigned int index ) const
{
  unsigned int  n          = 0;
  NameArrayType inputNames = this->GetInputNames();
  for( unsigned int i = 0; i < inputNames.size(); ++i )
  {
    if( this->IsInputOfType( "FixedMask", inputNames[ i ] ) )
    {
      if( index == n )
      {
        return itkDynamicCastInDebugMode< const FixedMaskType * >( this->GetInput( inputNames[ i ] ) );
      }

      n++;
    }
  }

  itkExceptionMacro( << "Index exceeds the number of fixed masks (index: "
                     << index << ", "
                     << "number of fixed masks: " << n << ")" );
}


template< typename TFixedImage, typename TMovingImage >
void
ElastixFilter< TFixedImage, TMovingImage >
::RemoveFixedMask()
{
  this->RemoveInputsOfType( "FixedMask" );
}


template< typename TFixedImage, typename TMovingImage >
unsigned int
ElastixFilter< TFixedImage, TMovingImage >
::GetNumberOfFixedMasks() const
{
  return this->GetNumberOfInputsOfType( "FixedMask" );
}


template< typename TFixedImage, typename TMovingImage >
void
ElastixFilter< TFixedImage, TMovingImage >
::SetMovingMask( MovingMaskType * movingMask )
{
  this->RemoveInputsOfType( "MovingMask" );
  this->AddMovingMask( movingMask );
}


template< typename TFixedImage, typename TMovingImage >
void
ElastixFilter< TFixedImage, TMovingImage >
::AddMovingMask( MovingMaskType * movingMask )
{
  this->SetInput( this->MakeUniqueName( "MovingMask" ), movingMask );
}


template< typename TFixedImage, typename TMovingImage >
const typename ElastixFilter< TFixedImage, TMovingImage >::MovingMaskType *
ElastixFilter< TFixedImage, TMovingImage >
::GetMovingMask() const
{
  return itkDynamicCastInDebugMode< const MovingMaskType * >( this->GetInput( "MovingMask" ) );
}


template< typename TFixedImage, typename TMovingImage >
const typename ElastixFilter< TFixedImage, TMovingImage >::MovingMaskType *
ElastixFilter< TFixedImage, TMovingImage >
::GetMovingMask( const unsigned int index ) const
{
  unsigned int  n          = 0;
  NameArrayType inputNames = this->GetInputNames();
  for( unsigned int i = 0; i < inputNames.size(); ++i )
  {
    if( this->IsInputOfType( "MovingMask", inputNames[ i ] ) )
    {
      if( index == n )
      {
        return itkDynamicCastInDebugMode< const MovingMaskType * >( this->GetInput( inputNames[ i ] ) );
      }

      n++;
    }
  }

  itkExceptionMacro( << "Index exceeds the number of moving masks (index: "
                     << index << ", "
                     << "number of moving masks: " << n << ")" );
}


template< typename TFixedImage, typename TMovingImage >
void
ElastixFilter< TFixedImage, TMovingImage >
::RemoveMovingMask()
{
  this->RemoveInputsOfType( "MovingMask" );
}


template< typename TFixedImage, typename TMovingImage >
unsigned int
ElastixFilter< TFixedImage, TMovingImage >
::GetNumberOfMovingMasks() const
{
  return this->GetNumberOfInputsOfType( "MovingMask" );
}


template< typename TFixedImage, typename TMovingImage >
void
ElastixFilter< TFixedImage, TMovingImage >
::SetLogFileName( const std::string logFileName )
{
  this->m_LogFileName = logFileName;
  this->LogToFileOn();
}


template< typename TFixedImage, typename TMovingImage >
void
ElastixFilter< TFixedImage, TMovingImage >
::RemoveLogFileName()
{
  this->m_LogFileName = "";
  this->LogToFileOff();
}


template< typename TFixedImage, typename TMovingImage >
std::string
ElastixFilter< TFixedImage, TMovingImage >
::MakeUniqueName( const DataObjectIdentifierType & inputName )
{
  return inputName + std::to_string( this->m_InputUID++ );
}


template< typename TFixedImage, typename TMovingImage >
bool
ElastixFilter< TFixedImage, TMovingImage >
::IsInputOfType( const DataObjectIdentifierType & inputType, DataObjectIdentifierType inputName )
{
  return std::strncmp( inputType.c_str(), inputName.c_str(), std::min( inputType.size(), inputName.size() ) ) == 0;
}


template< typename TFixedImage, typename TMovingImage >
unsigned int
ElastixFilter< TFixedImage, TMovingImage >
::GetNumberOfInputsOfType( const DataObjectIdentifierType & inputType )
{
  unsigned int  n          = 0;
  NameArrayType inputNames = this->GetInputNames();
  for( unsigned int i = 0; i < inputNames.size(); ++i )
  {
    if( this->IsInputOfType( inputType, inputNames[ i ] ) )
    {
      n++;
    }
  }

  return n;
}


template< typename TFixedImage, typename TMovingImage >
void
ElastixFilter< TFixedImage, TMovingImage >
::RemoveInputsOfType( const DataObjectIdentifierType & inputType )
{
  NameArrayType inputNames = this->GetInputNames();
  for( unsigned int i = 0; i < inputNames.size(); ++i )
  {
    if( this->IsInputOfType( inputType, inputNames[ i ] ) )
    {
      this->RemoveInput( inputNames[ i ] );
    }
  }
}


} // namespace itk

#endif // itkElastixFilter_hxx
