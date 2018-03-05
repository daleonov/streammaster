
#ifndef _PLUG_ICONTROLEXTRAS_H
#define _PLUG_ICONTROLEXTRAS_H

#include "IControl.h"

#ifndef PLUG_DEFAULT_BG_ICOLOR
#define PLUG_DEFAULT_BG_ICOLOR IColor(255, 40, 40, 40)
#endif
#define METERING_BAR_DEFAULT_SIZE_IRECT IRECT(0, 0, 74, 492)
#define METERING_BAR_DEFAULT_BG_ICOLOR IColor(255, 61, 61, 61)
#define METERING_BAR_DEFAULT_FG_ICOLOR IColor(255, 100, 255, 100)
#define METERING_BAR_MIN_FG_HEIGHT 2
#define METERING_BAR_DEFAULT_NOTCH_ICOLOR IColor(255, 100, 100, 255)
#define METERING_BAR_DEFAULT_NOTCH_HEIGHT 3
#define METERING_BAR_DEFAULT_NOTCH_VALUE -0.
#define METERING_BAR_MAX_NAME_SIZE 64
#define METERING_BAR_MAX_LABEL_SIZE 32

namespace Plug{
/*
@brief Vertical metering bar. Can be used for VU, loudness, gain reduction metering etc.
@param pPlug Plugin class pointer
@param x Bar's top left corner x
@param y Bar's top left corner y
@param pR IRECT representing the bar's size. Relative coordinates. 
@param paramIdx Unique ID of the control
*/
class ILevelMeteringBar : public IPanelControl
{
private:
	int mParamIdx;
	double fCurrentValue;
	double fNotchValue;
	IRECT mBarRect;
	int x;
	int y;
  bool bIsReversed;
  IColor *ptLevelBarColor;
  IColor *ptNotchColor;

	/*
	@param Converts value in meter's units to respective vertical coordinate on the bar. 
	@retval Converted vertical coordinate in pixels relative to bottom y of the meter.
	*/
	inline int ILevelMeteringBar::_CalculateRectHeight(double fValue);
public:
	ILevelMeteringBar(IPlugBase *pPlug, int x, int y, IRECT pR, int paramIdx);

	/*
	@param Graphic part. Sizes and colours are based on macros defined in the header. 
	*/
	bool Draw(IGraphics* pGraphics);

	/*
	@brief Store current value of the meter.
	@note That method doesn't redraw the meter - call Draw() to update graphics. 
	*/
	void SetValue(double fValue);

	/*
	@brief Value of the notch. Notch can be used to mark target, peak, zero level etc. 
	*/
	void SetNotchValue(double fValue);
};

} //namespace Plug

#endif //_PLUG_ICONTROLEXTRAS_H
