#ifndef __itkBModeImageFilter_txx
#define __itkBModeImageFilter_txx
#include "itkBModeImageFilter.h"

#include "itkMetaDataDictionary.h"

#include <algorithm>
#include <sstream>
#include <string>

namespace itk
{
template < class TInputImage, class TOutputImage >
BModeImageFilter< TInputImage, TOutputImage >
::BModeImageFilter()
{
  m_AnalyticFilter = AnalyticType::New();
  m_ComplexToModulusFilter = ComplexToModulusType::New();
  m_PadFilter = PadType::New();
  m_AddConstantFilter = AddConstantType::New();
  m_LogFilter = LogType::New();
  m_ROIFilter = ROIType::New();

  // Avoid taking the log of zero.  Assuming that the original input is coming
  // from a digitizer that outputs integer types, so 1 is small.
  m_AddConstantFilter->SetConstant( 1 );
  m_PadFilter->SetConstant( 0. );

  m_ComplexToModulusFilter->SetInput( m_AnalyticFilter->GetOutput() );
  m_ROIFilter->SetInput( m_ComplexToModulusFilter->GetOutput() );
  m_LogFilter->SetInput( m_AddConstantFilter->GetOutput() );
}

template < class TInputImage, class TOutputImage >
void
BModeImageFilter< TInputImage, TOutputImage >
::PrintSelf( std::ostream& os, Indent indent ) const
{
  Superclass::PrintSelf( os, indent );
}

template < class TInputImage, class TOutputImage >
void
BModeImageFilter< TInputImage, TOutputImage >
::GenerateInputRequestedRegion()
{
  // call the superclass' implementation of this method
  Superclass::GenerateInputRequestedRegion();

  // get pointers to the inputs
  typename InputImageType::Pointer inputPtr  =
    const_cast<InputImageType *> (this->GetInput());
  typename OutputImageType::Pointer outputPtr = this->GetOutput();

  if ( !inputPtr || !outputPtr )
    {
    return;
    }

  // we need to compute the input requested region (size and start index)
  typedef const typename OutputImageType::SizeType& OutputSizeType;
  OutputSizeType outputRequestedRegionSize =
    outputPtr->GetRequestedRegion().GetSize();
  typedef const typename OutputImageType::IndexType& OutputIndexType;
  OutputIndexType outputRequestedRegionStartIndex =
    outputPtr->GetRequestedRegion().GetIndex();

  //// the regions other than the fft direction are fine
  typename InputImageType::SizeType  inputRequestedRegionSize = outputRequestedRegionSize;
  typename InputImageType::IndexType inputRequestedRegionStartIndex = outputRequestedRegionStartIndex;

  // we but need all of the input in the fft direction
  const unsigned int direction = this->GetDirection();
  const typename InputImageType::SizeType& inputLargeSize =
    inputPtr->GetLargestPossibleRegion().GetSize();
  inputRequestedRegionSize[direction] = inputLargeSize[direction];
  const typename InputImageType::IndexType& inputLargeIndex =
    inputPtr->GetLargestPossibleRegion().GetIndex();
  inputRequestedRegionStartIndex[direction] = inputLargeIndex[direction];

  typename InputImageType::RegionType inputRequestedRegion;
  inputRequestedRegion.SetSize( inputRequestedRegionSize );
  inputRequestedRegion.SetIndex( inputRequestedRegionStartIndex );

  inputPtr->SetRequestedRegion( inputRequestedRegion );
}


template < class TInputImage, class TOutputImage >
void
BModeImageFilter< TInputImage, TOutputImage >
::EnlargeOutputRequestedRegion(DataObject *output)
{
  OutputImageType* outputPtr = dynamic_cast< OutputImageType* >( output );

  // we need to enlarge the region in the fft direction to the
  // largest possible in that direction
  typedef const typename OutputImageType::SizeType& ConstOutputSizeType;
  ConstOutputSizeType requestedSize =
    outputPtr->GetRequestedRegion().GetSize();
  ConstOutputSizeType outputLargeSize =
    outputPtr->GetLargestPossibleRegion().GetSize();
  typedef const typename OutputImageType::IndexType& ConstOutputIndexType;
  ConstOutputIndexType requestedIndex =
    outputPtr->GetRequestedRegion().GetIndex();
  ConstOutputIndexType outputLargeIndex =
    outputPtr->GetLargestPossibleRegion().GetIndex();

  typename OutputImageType::SizeType enlargedSize   = requestedSize;
  typename OutputImageType::IndexType enlargedIndex = requestedIndex;
  const unsigned int direction = this->GetDirection ();
  enlargedSize[direction]  = outputLargeSize[direction];
  enlargedIndex[direction] = outputLargeIndex[direction];

  typename OutputImageType::RegionType enlargedRegion;
  enlargedRegion.SetSize( enlargedSize );
  enlargedRegion.SetIndex( enlargedIndex );
  outputPtr->SetRequestedRegion( enlargedRegion );
}

template < class TInputImage, class TOutputImage >
void
BModeImageFilter< TInputImage, TOutputImage >
::GenerateData()
{
  typename InputImageType::ConstPointer  inputPtr  = this->GetInput();
  typename OutputImageType::Pointer      outputPtr = this->GetOutput();

  if ( !inputPtr || !outputPtr )
    {
    return;
    }

  const unsigned int direction = m_AnalyticFilter->GetDirection();
  typename InputImageType::SizeType size = inputPtr->GetLargestPossibleRegion().GetSize();

  // Zero padding.  FFT direction should be factorable by 2 for all FFT
  // implementations to work.
  unsigned int n = size[direction];
  while( n % 2 == 0 )
    n /= 2;
  bool padSize;
  if( n == 1 )
    padSize = false;
  else
    padSize = true;
  if( padSize )
    {
    n = size[direction];
    unsigned int newSizeDirection = 1;
    while( newSizeDirection < n )
      newSizeDirection *= 2;
    typename InputImageType::SizeType padSize;
    padSize.Fill( 0 );
    padSize[direction] = newSizeDirection - size[direction];
    size[direction] = newSizeDirection;
    m_PadFilter->SetPadUpperBound( padSize );
    m_PadFilter->SetInput( inputPtr );
    m_AnalyticFilter->SetInput( m_PadFilter->GetOutput() );
    m_ROIFilter->SetReferenceImage( inputPtr );
    m_ROIFilter->SetInput( m_ComplexToModulusFilter->GetOutput() );
    m_AddConstantFilter->SetInput( m_ROIFilter->GetOutput() );
    }
  else // padding is not required
    {
    m_AnalyticFilter->SetInput( inputPtr );
    m_AddConstantFilter->SetInput( m_ComplexToModulusFilter->GetOutput() );
    }
  m_LogFilter->GraftOutput( outputPtr );
  m_LogFilter->Update();
  this->GraftOutput( m_LogFilter->GetOutput() );
}

} // end namespace itk

#endif