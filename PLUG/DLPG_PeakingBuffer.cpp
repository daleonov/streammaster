#include "DLPG_PeakingBuffer.h"

namespace DLPG{

PeakingBuffer::PeakingBuffer(int nSize):
nSize(nSize),
nHeadIndex(0)
{
	afBuffer = new double [nSize];
}

PeakingBuffer::~PeakingBuffer(){
	delete [] afBuffer;
}

void PeakingBuffer::Clear(double fValue){
	for(int i = 0; i < nSize; i++)
		afBuffer[i] = fValue;
}

void PeakingBuffer::Add(double fValue){
	if(nHeadIndex++ == nSize) nHeadIndex = 0;
	afBuffer[nHeadIndex] = fValue;
}

double PeakingBuffer::GetMax(){
	return 0.;
}

double PeakingBuffer::GetMin(){
	return 0.;
}

double PeakingBuffer::GetAverage(){
	return 0.;
}

void PeakingBuffer::Resize(int nNewSize){

}

void PeakingBuffer::Resize(double fNewSizeSeconds, double fSampleRateHz, int nChunkSize){

}

} // namespace DLPG