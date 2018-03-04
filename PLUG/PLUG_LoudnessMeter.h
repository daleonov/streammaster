#ifndef _PLUG_LOUDNESSMETER_H
#define _PLUG_LOUDNESSMETER_H


namespace Plug{

class LoudnessMeter{
/*
@brief Class for measuring LUFS and LRA. Lib1770 C++ wrapper. 
*/
public:
	LoudnessMeter();
	~LoudnessMeter();
	/*
	@brief Send samples to the meter
	@param afBuffer Buffer with samples in 64bit float format (linear values)
	@note Samples in the buffer can be interleaved or planar. Define _PLUG_INTERLEAVED_MODE or _PLUG_PLANAR_MODE respectively
	@param nSamples Size of the buffer (in samples)
	*/
	void AddSamples(double* afBuffer, size_t nSamples);

	/*
	@brief LUFS reading
	@note Each call resets the meter
	@retval LUFS value (in dB)
	*/
	double GetLufs();

	/*
	@brief LRA reading
	@note Each call resets the meter
	@retval LRA value
	*/
	double GetLra();

	/*
	@brief Sets sample rate for the meter
	@param fRate sample rate. E.g. "44100.0" for 44.1kHz.
	*/
	void SetSampleRate(double fRate);

	/*
	@brief Sets nubmer of audio channels
	@paran nChannels Nubmer of channels. E.g. "2" for stereo.
	*/
	void SetNumberOfChannels(int nChannels);

private:
	double _fSampleRate = 44100.;
	int _nChannels = 2;
	double _fLufs = 0;
	double _fLra = 0;
	//bs1770_ctx_t *_tCtx = NULL;
}; //class LoudnessMeter

} //namespace Plug

#endif //_PLUG_LOUDNESSMETER_H
