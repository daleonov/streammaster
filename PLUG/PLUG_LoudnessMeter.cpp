#include "PLUG_LoudnessMeter.h"

using namespace Plug;

void LoudnessMeter::AddSamples(double* afBuffer, size_t nSamples){
	ebur128_add_frames_double(
		this->_tLoudnessMeterEbur128,
		afBuffer,
		(size_t) nSamples
		);
}

LoudnessMeter::LoudnessMeter(){
	this->_tLoudnessMeterEbur128 = ebur128_init(
		(unsigned) this->_nChannels,
		(unsigned) this->_fSampleRate,
		EBUR128_MODE_I
		);

	// Mono
	if (this->_nChannels == 1)
		ebur128_set_channel(this->_tLoudnessMeterEbur128, 0, EBUR128_DUAL_MONO);
	// Stereo
	if (this->_nChannels == 2)
		ebur128_set_channel(this->_tLoudnessMeterEbur128, 0, EBUR128_LEFT);
		ebur128_set_channel(this->_tLoudnessMeterEbur128, 1, EBUR128_RIGHT);
	// Surround
	if (this->_nChannels == 5){
		ebur128_set_channel(this->_tLoudnessMeterEbur128, 0, EBUR128_LEFT);
		ebur128_set_channel(this->_tLoudnessMeterEbur128, 1, EBUR128_RIGHT);
		ebur128_set_channel(this->_tLoudnessMeterEbur128, 2, EBUR128_CENTER);
		ebur128_set_channel(this->_tLoudnessMeterEbur128, 3, EBUR128_LEFT_SURROUND);
		ebur128_set_channel(this->_tLoudnessMeterEbur128, 4, EBUR128_RIGHT_SURROUND);
	}
}

LoudnessMeter::~LoudnessMeter(){	
	ebur128_destroy(&this->_tLoudnessMeterEbur128);
}

double LoudnessMeter::GetLufs(){
	double fLoudness;
  // Options
  // Monemtary (2sec): ebur128_loudness_momentary()
  // LUFS short term: ebur128_loudness_shortterm()
  // LIFS integrated: ebur128_loudness_global()
	ebur128_loudness_global(this->_tLoudnessMeterEbur128, &fLoudness);
	return fLoudness;
}

double LoudnessMeter::GetLra(){
  double fLra;
  ebur128_loudness_range(this->_tLoudnessMeterEbur128, &fLra);
	return fLra;
}

void LoudnessMeter::SetSampleRate(double fRate){
	this->_fSampleRate = fRate;
}

void LoudnessMeter::SetNumberOfChannels(int nChannels){
	this->_nChannels = nChannels;
}

