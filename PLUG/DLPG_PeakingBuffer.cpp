#include "DLPG_PeakingBuffer.h"

namespace DLPG{

double PeakingBuffer::fInitClearValue = 0.;

PeakingBuffer::PeakingBuffer(int nSize, double fClearValue):
nSize(nSize),
nHeadIndex(0)
{
	afBuffer = new double [nSize];
	fInitClearValue = fClearValue;
	Clear(fInitClearValue);
}

PeakingBuffer::~PeakingBuffer(){
	delete [] afBuffer;
}

void PeakingBuffer::Clear(double fClearValue){
	for(int i = 0; i < nSize; i++)
		afBuffer[i] = fClearValue;
	fInitClearValue = fClearValue;
}

void PeakingBuffer::Add(double fValue){
	if(nHeadIndex++ == nSize) nHeadIndex = 0;
	afBuffer[nHeadIndex] = fValue;
}

double PeakingBuffer::GetMax(){
	double fMax;
	if (nSize == 0) return 0.;
	fMax = afBuffer[0];
	for (int i = 1; i < nSize; i++)
		if(afBuffer[i] > fMax) fMax = afBuffer[i];
	return fMax;
}

double PeakingBuffer::GetMin(){
	double fMin;
	if (nSize == 0) return 0.;
	fMin = afBuffer[0];
	for (int i = 1; i < nSize; i++)
		if(afBuffer[i] < fMin) fMin = afBuffer[i];
	return fMin;
}

double PeakingBuffer::GetAverage(){
	double fSum = 0.;
	if(nSize == 0) return 0.;
	for (int i = 0; i < nSize; i++) fSum += afBuffer[i];
	return fSum / nSize;
}

void PeakingBuffer::Resize(int nNewSize){
	delete [] afBuffer;
	afBuffer = new double [nNewSize];
	nSize = nNewSize;
	nHeadIndex = 0; 
}

void PeakingBuffer::Resize(double fNewSizeSeconds, double fSampleRateHz, int nChunkSize){
	nSize = (int)(fNewSizeSeconds * fSampleRateHz / nChunkSize);
	Resize(nSize);
}

} // namespace DLPG