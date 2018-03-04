#include "PLUG_LoudnessMeter.h"

using namespace Plug;

void LoudnessMeter::AddSamples(double* afBuffer, size_t nSamples){
	/*for (int i = 0; i < nSamples; i++)
	{
    bs1770_sample_f64_t tSample = { afBuffer[i], afBuffer[i] };
		bs1770_ctx_add_sample_f64(
			this->_tCtx,
			0,
			this->_fSampleRate,
			this->_nChannels,
      		tSample
			);
	}*/

}

LoudnessMeter::LoudnessMeter(){
	//this->_tCtx = bs1770_ctx_open_default(1);
}

LoudnessMeter::~LoudnessMeter(){	
    //bs1770_ctx_close(this->_tCtx);
}

double LoudnessMeter::GetLufs(){
  return 0.;
	//bs1770_ctx_track_lufs_r128(_tCtx,0);
}

double LoudnessMeter::GetLra(){
  return 0.;
	//bs1770_ctx_track_lra_default(_tCtx,0);
}

void LoudnessMeter::SetSampleRate(double fRate){
	this->_fSampleRate = fRate;
}

void LoudnessMeter::SetNumberOfChannels(int nChannels){
	this->_nChannels = nChannels;
}

