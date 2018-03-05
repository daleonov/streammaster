#include "PLUG_IControlExtras.h"


namespace Plug{

ILevelMeteringBar::ILevelMeteringBar(
		IPlugBase *pPlug,
		int x,
		int y,
		IRECT pR,
		int paramIdx,
		bool bIsReversed,
		const IColor *ptLevelBarColor,
		const IColor *ptNotchColor,
		const IColor *ptAboveNotchColor
		)
: IPanelControl(pPlug, pR, &PLUG_DEFAULT_BG_ICOLOR)
{
	this->x = x;
	this->y = y;
	this->mPlug = pPlug;
	this->mParamIdx = paramIdx;
	this->fNotchValue = METERING_BAR_DEFAULT_NOTCH_VALUE;
	this->fCurrentValue = mPlug->GetParam(paramIdx)->GetDefault();
	// There is no operator "=" for IRECT, so copying it explicitely
	memcpy(&this->mBarRect, &pR, sizeof(pR));
	this->ptLevelBarColor = new IColor(*ptLevelBarColor);
	this->ptNotchColor = new IColor(*ptNotchColor);
	this->bIsReversed = bIsReversed;
}
ILevelMeteringBar::~ILevelMeteringBar(){
	delete (this->ptLevelBarColor);
	delete (this->ptNotchColor);
}
bool ILevelMeteringBar::Draw(IGraphics* pGraphics){

	int nRectHeight  = _CalculateRectHeight(this->fCurrentValue);
	int nNotchHeight = _CalculateRectHeight(this->fNotchValue);

	// Background
	const int nBgBarTopLeftX     = mBarRect.L + this->x;
	const int nBgBarTopLeftY     = mBarRect.T + this->y;
	const int nBgBarBottomRightX = mBarRect.R + this->x;
	const int nBgBarBottomRightY = mBarRect.B + this->y;
	IRECT tBgRect(nBgBarTopLeftX, nBgBarTopLeftY, nBgBarBottomRightX, nBgBarBottomRightY);
	pGraphics->FillIRect(&METERING_BAR_DEFAULT_BG_ICOLOR, &tBgRect);

	// Notch
	const int nNotchTopLeftX     = nBgBarTopLeftX;
	const int nNotchBottomRightX = nBgBarBottomRightX;
	int nNotchTopLeftY;
	int nNotchBottomRightY;

	#ifdef METERING_BAR_NOTCH_1
	// This code fills the background behind a level bar with solid color 
	if (!this->bIsReversed){
		nNotchTopLeftY = nBgBarBottomRightY - nNotchHeight - METERING_BAR_DEFAULT_NOTCH_HEIGHT;
		nNotchBottomRightY = nBgBarBottomRightY;
	}
	else{
		nNotchTopLeftY  = nBgBarTopLeftY;
		nNotchBottomRightY = nBgBarBottomRightY - nNotchHeight - METERING_BAR_DEFAULT_NOTCH_HEIGHT;
	}
	IRECT tNotchRectOption1(nNotchTopLeftX, nNotchTopLeftY, nNotchBottomRightX, nNotchBottomRightY);
	pGraphics->FillIRect(ptNotchColor, &tNotchRectOption1);
	// End notch option 1
	#endif

	// Foreground
	const int nLevelBarTopLeftX     = nBgBarTopLeftX;
	const int nLevelBarBottomRightX = nBgBarBottomRightX;
	int nLevelBarTopLeftY;
	int nLevelBarBottomRightY;
	// Normal display (bar starts at the bottom)
	if (!this->bIsReversed){
		nLevelBarTopLeftY     = nBgBarBottomRightY - nRectHeight;
		nLevelBarBottomRightY = nBgBarBottomRightY;
	}
	// Reversed display (bar starts at the top)
	else{
		nLevelBarTopLeftY     = nBgBarTopLeftY;
		nLevelBarBottomRightY = nBgBarBottomRightY - nRectHeight;
	}
	IRECT tLevelRect(nLevelBarTopLeftX, nLevelBarTopLeftY, nLevelBarBottomRightX, nLevelBarBottomRightY);
	pGraphics->FillIRect(ptLevelBarColor, &tLevelRect);

	// Notch option #2 coordinates
	if (!this->bIsReversed){
		nNotchTopLeftY  = nBgBarBottomRightY - _CalculateRectHeight(this->fNotchValue) - \
			METERING_BAR_DEFAULT_NOTCH_HEIGHT/2;
		nNotchBottomRightY = nNotchTopLeftY + METERING_BAR_DEFAULT_NOTCH_HEIGHT;
	}
	else{
		nNotchTopLeftY  = nBgBarBottomRightY - _CalculateRectHeight(this->fNotchValue) - \
			METERING_BAR_DEFAULT_NOTCH_HEIGHT/2;
		nNotchBottomRightY = nNotchTopLeftY + METERING_BAR_DEFAULT_NOTCH_HEIGHT;
	}

	#ifdef METERING_BAR_NOTCH_2
	// This code draws a small line at notch level.
	// A piece of code above also belongs to that botch, but its coordinates are needed in both cases. 
	IRECT tNotchRectOption2(nNotchTopLeftX, nNotchTopLeftY, nNotchBottomRightX, nNotchBottomRightY);
	pGraphics->FillIRect(ptNotchColor, &tNotchRectOption2);
	// End notch option 2
	#endif
	
    const IColor tIcolorRed(255, 255, 0, 0);
    IRECT tIrectA(nNotchTopLeftX, nNotchTopLeftY, nLevelBarBottomRightX, nLevelBarBottomRightY);
    IRECT tIrectB(nLevelBarTopLeftX, nLevelBarTopLeftY, nNotchBottomRightX, nNotchBottomRightY);
	// If the value is above the notch level	
	if ((this->bIsReversed) && (this->fCurrentValue < this->fNotchValue))
		pGraphics->FillIRect(&tIcolorRed, &tIrectA);
	if ((!this->bIsReversed) && (this->fCurrentValue > this->fNotchValue))
		pGraphics->FillIRect(&tIcolorRed, &tIrectB);

	// If the value completely overloards the meter - just fill the whole box with solid color. 	
	const double fMax = mPlug->GetParam(this->mParamIdx)->GetMax();
	const double fMin = mPlug->GetParam(this->mParamIdx)->GetMin();
	if ((this->bIsReversed) && (this->fCurrentValue < fMin))
		pGraphics->FillIRect(&tIcolorRed, &tBgRect);
	if ((!this->bIsReversed) && (this->fCurrentValue > fMax))
		pGraphics->FillIRect(&tIcolorRed, &tBgRect);

	#ifdef METERING_BAR_NOTCH_2
	// Always draw notch option 2 on top of everything!
	pGraphics->FillIRect(ptNotchColor, &tNotchRectOption2);
	#endif

	return true;
}

void ILevelMeteringBar::SetValue(double fValue){
	this->fCurrentValue = fValue;
	SetDirty(false);
	Redraw();
}

void ILevelMeteringBar::SetNotchValue(double fValue){
	this->fNotchValue = fValue;
	SetDirty(false);
	Redraw();
}

int ILevelMeteringBar::_CalculateRectHeight(double fValue){
	const double fMax = mPlug->GetParam(this->mParamIdx)->GetMax();
	const double fMin = mPlug->GetParam(this->mParamIdx)->GetMin();
	const double fBarRange = fabs(fMax - fMin);
	const int nBarBgHeight = fabs(mBarRect.B - mBarRect.T);
	double fRelativeValue;

	if(fValue < fMin)
    return METERING_BAR_MIN_FG_HEIGHT;
	else
		fRelativeValue = (fValue > fMax) ? fMax : fValue - fMin;

	int nLevelBarHeight = floor((fRelativeValue / fBarRange) * nBarBgHeight);
	return nLevelBarHeight;
}
} //namespace Plug
