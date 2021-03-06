#ifndef _PLUG_LOUDNESSMETER_H
#define _PLUG_LOUDNESSMETER_H

#include "ebur128.h"
#include <math.h>

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
	@brief Integrated LUFS reading
	@retval LUFS value (in dB)
	*/
	double GetLufs();

	/*
	@brief Momentary LUFS reading
	@retval LUFS value (in dB)
	*/
	double GetMomentaryLufs();

	/*
	@brief Short term LUFS reading
	@retval LUFS value (in dB)
	*/
	double GetShortTermLufs();

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

	/*
	@brief True peaking. 4x oversampling for fs < 96000 Hz, 2x for fs < 192000 Hz and 1x for 192000 Hz.
	@retval Highest peak from all current samples and frames (linear, 1.0 = 0.0dB)
	*/
	double GetTruePeaking();

	/*
	@brief True peaking, but moving value rather than integrated. 4x oversampling for fs < 96000 Hz, 2x for fs < 192000 Hz and 1x for 192000 Hz.
	@retval Highest peak from last AddSamples() batch (linear, 1.0 = 0.0dB)
	*/
	double GetTruePeakingShortTerm();

private:
	double _fSampleRate = 44100.;
	int _nChannels = 2;
	double _fLufs = 0;
	double _fLra = 0;
	ebur128_state *_tLoudnessMeterEbur128 = NULL;
}; //class LoudnessMeter

} //namespace Plug

#endif //_PLUG_LOUDNESSMETER_H
