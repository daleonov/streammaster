#include "PLUG_IControlExtras.h"


namespace Plug{

ILevelMeteringBar::ILevelMeteringBar(IPlugBase *pPlug, int x, int y, IRECT pR, int paramIdx)
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
}

bool ILevelMeteringBar::Draw(IGraphics* pGraphics){

	// Background
	const int nBgBarTopLeftX     = mBarRect.L + this->x;
	const int nBgBarTopLeftY     = mBarRect.T + this->y;
	const int nBgBarBottomRightX = mBarRect.R + this->x;
	const int nBgBarBottomRightY = mBarRect.B + this->y;
	IRECT tBgRect(nBgBarTopLeftX, nBgBarTopLeftY, nBgBarBottomRightX, nBgBarBottomRightY);
	pGraphics->FillIRect(&METERING_BAR_DEFAULT_BG_ICOLOR, &tBgRect);

	// Foreground
	const int nLevelBarTopLeftX     = nBgBarTopLeftX;
	const int nLevelBarTopLeftY     = nBgBarBottomRightY - _CalculateRectHeight(this->fCurrentValue);
	const int nLevelBarBottomRightX = nBgBarBottomRightX;
	const int nLevelBarBottomRightY = nBgBarBottomRightY;
	IRECT tLevelRect(nLevelBarTopLeftX, nLevelBarTopLeftY, nLevelBarBottomRightX, nLevelBarBottomRightY);
	pGraphics->FillIRect(&METERING_BAR_DEFAULT_FG_ICOLOR, &tLevelRect);

	// Notch
	// Y's are calculated the way the height of the notch is preserved
	// both when METERING_BAR_DEFAULT_NOTCH_HEIGHT is even and odd.
	const int nNotchTopLeftX     = nBgBarTopLeftX;
	const int nNotchTopLeftY     = nBgBarBottomRightY - _CalculateRectHeight(this->fNotchValue) - METERING_BAR_DEFAULT_NOTCH_HEIGHT/2;
	const int nNotchBottomRightX = nBgBarBottomRightX;
	const int nNotchBottomRightY = nNotchTopLeftY + METERING_BAR_DEFAULT_NOTCH_HEIGHT;
	IRECT tNotchRect(nNotchTopLeftX, nNotchTopLeftY, nNotchBottomRightX, nNotchBottomRightY);
	pGraphics->FillIRect(&METERING_BAR_DEFAULT_NOTCH_ICOLOR, &tNotchRect);
	return true;
}

void ILevelMeteringBar::SetValue(double fValue){
	this->fCurrentValue = fValue;
}

void ILevelMeteringBar::SetNotchValue(double fValue){
	this->fNotchValue = fValue;
}

inline int ILevelMeteringBar::_CalculateRectHeight(double fValue){
  const double fMax = mPlug->GetParam(this->mParamIdx)->GetMax();
  const double fMin = mPlug->GetParam(this->mParamIdx)->GetMin();
	const double fBarRange = fabs(fMax - fMin);
	const int nBarBgHeight = fabs(mBarRect.B - mBarRect.T);
  const double fRelativeValue = (fValue > fMax) ? fMax : fValue - fMin;
	int nLevelBarHeight = floor((fRelativeValue / fBarRange) * nBarBgHeight);
	return nLevelBarHeight;
}
} //namespace Plug