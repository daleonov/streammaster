#ifndef __DLPG_PEAKING_BUFFER_H
#define __DLPG_PEAKING_BUFFER_H

namespace DLPG{

class PeakingBuffer{
public:
	PeakingBuffer(int nSize);
	~PeakingBuffer();
	void Clear(double fValue = 0.);
	void Add(double fValue);
	double GetMax();
	double GetMin();
	double GetAverage();
	void Resize(int nNewSize);
	void Resize(double fNewSizeSeconds, double fSampleRateHz = 44100., int nChunkSize = 1);
private:
	double* afBuffer;
	int nSize;
	int nHeadIndex;
};

} // namespace DLPG

#endif // __DLPG_PEAKING_BUFFER_H