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
	ebur128_set_channel(this->_tLoudnessMeterEbur128, 0, EBUR128_LEFT);
	ebur128_set_channel(this->_tLoudnessMeterEbur128, 1, EBUR128_RIGHT);
}

LoudnessMeter::~LoudnessMeter(){	
	ebur128_destroy(&this->_tLoudnessMeterEbur128);
}

double LoudnessMeter::GetLufs(){
	double fLoudness;
	ebur128_loudness_global(this->_tLoudnessMeterEbur128, &fLoudness);
	// See also: ebur128_loudness_global_multiple() for multiple files
	return fLoudness;
}

double LoudnessMeter::GetLra(){
	return 0.;
}

void LoudnessMeter::SetSampleRate(double fRate){
	this->_fSampleRate = fRate;
}

void LoudnessMeter::SetNumberOfChannels(int nChannels){
	this->_nChannels = nChannels;
}

