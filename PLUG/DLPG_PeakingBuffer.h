#ifndef __DLPG_PEAKING_BUFFER_H
#define __DLPG_PEAKING_BUFFER_H

namespace DLPG{

class PeakingBuffer{
public:
	PeakingBuffer(int nSize, double fClearValue = 0.);
	~PeakingBuffer();

	/*
	@brief Fill the buffer with init values
	@param fClearValue Value to fill the buffer with 
	*/
	void Clear(double fClearValue = 0.);

	/*
	@brief Insert an element into a circular buffer
	@param fValue value of the new element
	*/
	void Add(double fValue);

	/*
	@brief Get the largest value of the buffer (non-rectified)
	@retval Max element of the buffer
	*/
	double GetMax();

	/*
	@brief Get the smallest value of the buffer (non-rectified)
	@retval Min element of the buffer
	*/
	double GetMin();

	/*
	@brief Get the average of the buffer (non-rectified)
	@note All elements are taken into account, even empty ones. 
	@retval Average value of the buffer
	*/
	double GetAverage();

	/*
	@brief Change the size of the buffer
	@note Resets all the elements of the buffer as well!
	@param nNewSize new size of the buffer
	*/
	void Resize(int nNewSize);

	/*
	@brief Change the size of the buffer, for audio applications
	@note Resets all the elements of the buffer as well!
	@param nNewSizeSeconds Time window that the buffer should represent, in seconds (not ms)
	@param fSampleRateHz Sample rate to correctly convert time-based parameters, Hz (not kHz)
	@param nChunkSize For cases, when we do not add data every single sample, but once in every nChunkSize samples
	*/
	void Resize(double fNewSizeSeconds, double fSampleRateHz = 44100., int nChunkSize = 1);
private:
	double* afBuffer;
	int nSize;
	int nHeadIndex;
	static double fInitClearValue;
};

} // namespace DLPG

#endif // __DLPG_PEAKING_BUFFER_H