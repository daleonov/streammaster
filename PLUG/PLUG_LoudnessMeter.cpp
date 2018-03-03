#include "PLUG_LoudnessMeter.h"

#define _PLUG_PLANAR_MODE
#define _PLUG_INTERLEAVED_MODE

using namespace Plug;

void LoudnessMeter::AddSamples(double* afBuffer, size_t nSamples){

	bs1770_ctx_add_samples_p_f64(
		this->_tCtx,
		0,
		this->_fSampleRate,
		this->_nChannels,
    (bs1770_f64_t**)&afBuffer,
		nSamples
		);
}

LoudnessMeter::LoudnessMeter(){
	this->_tCtx = bs1770_ctx_open_default(1);
}

LoudnessMeter::~LoudnessMeter(){
}

double LoudnessMeter::GetLufs(){
	return bs1770_ctx_track_lufs_r128(_tCtx,0);
}

double LoudnessMeter::GetLra(){
	return bs1770_ctx_track_lra_default(_tCtx,0);
}
