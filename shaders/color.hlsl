#pragma once

// sRGB SDR white is meant to be mapped to 80 nits (not 100, even if some game engine (UE) and consoles (PS5) interpret it as such).
static const float WhiteNits_sRGB = 80.f;
static const float ReferenceWhiteNits_BT2408 = 203.f;

// SDR linear mid gray.
// This is based on the commonly used value, though perception space mid gray (0.5) in sRGB or Gamma 2.2 would theoretically be ~0.2155 in linear.
static const float MidGray = 0.18f;

// Start of the highlights shoulder (empirical)
static const float MinHighlightsColor = pow(2.f / 3.f, 2.2f);
static const float MaxShadowsColor = pow(1.f / 3.f, 2.2f);

// SMPTE ST 2084 (Perceptual Quantization) is only defined until this amount of nits.
// This is also the max each color channel can have in HDR10.
static const float PQMaxNits = 10000.0f;
// SMPTE ST 2084 is defined as using BT.2020 white point.
static const float PQMaxWhitePoint = PQMaxNits / WhiteNits_sRGB;

// These have been calculated to be as accurate as possible
static const float3x3 BT709_2_BT2020 = float3x3(
	0.62722527980804443359375f,       0.329476892948150634765625f, 0.04329781234264373779296875f,
	0.0690418779850006103515625f,     0.919605672359466552734375f, 0.011352437548339366912841796875f,
	0.01639117114245891571044921875f, 0.0880887508392333984375f,   0.89552009105682373046875f);

static const float3x3 BT2020_2_BT709 = float3x3(
	 1.6609637737274169921875f,       -0.58811271190643310546875f,    -0.072851054370403289794921875f,
	-0.124477200210094451904296875f,   1.1328194141387939453125f,     -0.00834227167069911956787109375f,
	-0.0181571580469608306884765625f, -0.10066641867160797119140625f,  1.118823528289794921875f);

static const half3x3 BT709_2_BT2020_half = half3x3(
	0.62744140625h,     0.32958984375h,    0.043304443359375h,
	0.06903076171875h,  0.91943359375h,    0.0113525390625h,
	0.016387939453125h, 0.08807373046875h, 0.8955078125h);

static const half3x3 BT2020_2_BT709_half = half3x3(
	 1.6611328125h,      -0.587890625h,      -0.0728759765625h,
	-0.12445068359375h,   1.1328125h,        -0.00833892822265625h,
	-0.018157958984375h, -0.10064697265625h,  1.119140625h);

// this is BT.2020 but a little wider so that we have some headroom for processing
static const float3x3 BT709_2_WBT2020 = float3x3(
	0.610396862030029296875,        0.3320253789424896240234375,  0.0575777851045131683349609375,
	0.07514785230159759521484375,   0.897075176239013671875,      0.02777696959674358367919921875,
	0.0219906084239482879638671875, 0.09680221974849700927734375, 0.881207168102264404296875);

static const float3x3 WBT2020_2_BT709 = float3x3(
	 1.71869885921478271484375,       -0.626136362552642822265625,    -0.09256245195865631103515625,
	-0.14313395321369171142578125,     1.17068326473236083984375,     -0.0275493673980236053466796875,
	-0.02716676704585552215576171875, -0.112976409494876861572265625,  1.1401431560516357421875);

static const float3x3 WBT2020_2_BT2020 = float3x3(
	 1.02967584133148193359375,        -0.01190710626542568206787109375,  -0.017768688499927520751953125,
	-0.013273007236421108245849609375,  1.032054901123046875,             -0.01878183521330356597900390625,
	-0.008765391074120998382568359375, -0.008311719633638858795166015625,  1.01707708835601806640625);

static const float PQ_constant_M1 =  0.1593017578125f;
static const float PQ_constant_M2 = 78.84375f;
static const float PQ_constant_C1 =  0.8359375f;
static const float PQ_constant_C2 = 18.8515625f;
static const float PQ_constant_C3 = 18.6875f;

float3 BT709_To_BT2020(float3 color)
{
	return mul(BT709_2_BT2020, color);
}

float3 BT2020_To_BT709(float3 color)
{
	return mul(BT2020_2_BT709, color);
}

half3 BT709_To_BT2020_half(half3 color)
{
	return mul(BT709_2_BT2020_half, color);
}

half3 BT2020_To_BT709_half(half3 color)
{
	return mul(BT2020_2_BT709_half, color);
}

float3 BT709_To_WBT2020(float3 color)
{
	return mul(BT709_2_WBT2020, color);
}

float3 WBT2020_To_BT709(float3 color)
{
	return mul(WBT2020_2_BT709, color);
}

float3 WBT2020_To_BT2020(float3 color)
{
	return mul(WBT2020_2_BT2020, color);
}

float gamma_linear_to_sRGB(float channel)
{
	if (channel <= 0.0031308f)
	{
		channel = channel * 12.92f;
	}
	else
	{
		channel = 1.055f * pow(channel, 1.f / 2.4f) - 0.055f;
	}
	return channel;
}

float3 gamma_linear_to_sRGB(float3 Color)
{
	return float3(gamma_linear_to_sRGB(Color.r),
	              gamma_linear_to_sRGB(Color.g),
	              gamma_linear_to_sRGB(Color.b));
}

// Mirroring negative colors makes this closer to gamma 2.2 and perception space in general,
// it's sometimes easier to work with these values.
float3 gamma_linear_to_sRGB_mirrored(float3 Color)
{
	return gamma_linear_to_sRGB(abs(Color)) * sign(Color);
}

template<class T>
T linear_to_gamma_mirrored(T Color, float Gamma = 2.2f)
{
	return pow(abs(Color), 1.f / Gamma) * sign(Color);
}

float gamma_sRGB_to_linear(float channel)
{
	if (channel <= 0.04045f)
	{
		channel = channel / 12.92f;
	}
	else
	{
		channel = pow((channel + 0.055f) / 1.055f, 2.4f);
	}
	return channel;
}

float3 gamma_sRGB_to_linear(float3 Color)
{
	return float3(gamma_sRGB_to_linear(Color.r),
	              gamma_sRGB_to_linear(Color.g),
	              gamma_sRGB_to_linear(Color.b));
}

float3 gamma_sRGB_to_linear_mirrored(float3 Color)
{
	return gamma_sRGB_to_linear(abs(Color)) * sign(Color);
}

template<class T>
T gamma_to_linear_mirrored(T Color, float Gamma = 2.2f)
{
	return pow(abs(Color), Gamma) * sign(Color);
}

// PQ (Perceptual Quantizer - ST.2084) encode/decode used for HDR10 BT.2100
template<class T>
T Linear_to_PQ(T LinearColor)
{
	LinearColor = max(LinearColor, 0.f);
	T colorPow = pow(LinearColor, PQ_constant_M1);
	T numerator = PQ_constant_C1 + PQ_constant_C2 * colorPow;
	T denominator = 1.f + PQ_constant_C3 * colorPow;
	T pq = pow(numerator / denominator, PQ_constant_M2);
	return pq;
}

template<class T>
T Linear_to_PQ(T LinearColor, const float PQMaxValue)
{
	LinearColor /= PQMaxValue;
	return Linear_to_PQ(LinearColor);
}

template<class T>
T PQ_to_Linear(T ST2084Color)
{
	ST2084Color = max(ST2084Color, 0.f);
	T colorPow = pow(ST2084Color, 1.f / PQ_constant_M2);
	T numerator = max(colorPow - PQ_constant_C1, 0.f);
	T denominator = PQ_constant_C2 - (PQ_constant_C3 * colorPow);
	T linearColor = pow(numerator / denominator, 1.f / PQ_constant_M1);
	return linearColor;
}

template<class T>
T PQ_to_Linear(T ST2084Color, const float PQMaxValue)
{
	T linearColor = PQ_to_Linear(ST2084Color);
	linearColor *= PQMaxValue;
	return linearColor;
}

float Luminance(float3 color)
{
	// Fixed from "wrong" values: 0.2125 0.7154 0.0721f
	return dot(color, float3(0.2125072777271270751953125f, 0.71535003185272216796875f, 0.07214272022247314453125f));
}

float3 Saturation(float3 color, float saturation)
{
	float luminance = Luminance(color);
	return lerp(luminance, color, saturation);
}

//RGB linear BT.709/sRGB -> OKLab's LMS
static const float3x3 srgb_to_oklms = {
	0.4122214708f, 0.5363325363f, 0.0514459929f,
	0.2119034982f, 0.6806995451f, 0.1073969566f,
	0.0883024619f, 0.2817188376f, 0.6299787005f};

//RGB linear BT.2020 -> OKLab's LMS
static const float3x3 bt2020_to_oklms = {
	0.61648190021514892578125f,    0.3602248132228851318359375f,  0.02302939631044864654541015625f,
	0.2650513947010040283203125f,  0.635972559452056884765625f,   0.0989705026149749755859375f,
	0.10011710226535797119140625f, 0.20404155552387237548828125f, 0.69590473175048828125f};

//OKLab's L'M'S' -> OKLab
static const float3x3 oklms_to_oklab = {
	0.2104542553f,  0.7936177850f, -0.0040720468f,
	1.9779984951f, -2.4285922050f,  0.4505937099f,
	0.0259040371f,  0.7827717662f, -0.8086757660f};

//OKLab -> OKLab's L'M'S'
//the 1s get optimized away by the compiler
static const float3x3 oklab_to_oklms_ = {
	1.f,  0.3963377774f,  0.2158037573f,
	1.f, -0.1055613458f, -0.0638541728f,
	1.f, -0.0894841775f, -1.2914855480f};

//OKLab's LMS -> RGB linear BT.709/sRGB
static const float3x3 oklms_to_srgb = {
	 4.0767416621f, -3.3077115913f,  0.2309699292f,
	-1.2684380046f,  2.6097574011f, -0.3413193965f,
	-0.0041960863f, -0.7034186147f,  1.7076147010f};

//OKLab's LMS -> RGB linear BT.2020
static const float3x3 oklms_to_bt2020 = {
	 2.1408574581146240234375f,       -1.24677360057830810546875f,  0.106467388570308685302734375f,
	-0.8846709728240966796875f,        2.16277790069580078125f,    -0.2783108055591583251953125f,
	-0.0486083738505840301513671875f, -0.45476520061492919921875f,  1.503262996673583984375f};

// (in) linear sRGB/BT.709
// (out) OKLab:
// L - perceived lightness
// a - how green/red the color is
// b - how blue/yellow the color is
float3 linear_srgb_to_oklab(float3 rgb) {
	//LMS
	float3 lms = mul(srgb_to_oklms, rgb);

	//L'M'S'
	float3 lms_ = pow(abs(lms), 1.f/3.f) * sign(lms);

	return mul(oklms_to_oklab, lms_);
}

// (in) linear BT.2020
// (out) OKLab
float3 linear_bt2020_to_oklab(float3 rgb) {
	//LMS
	float3 lms = mul(bt2020_to_oklms, rgb);

	//L'M'S'
	float3 lms_ = pow(abs(lms), 1.f/3.f) * sign(lms);

	return mul(oklms_to_oklab, lms_);
}

// (in) OKLab
// (out) linear sRGB/BT.709
float3 oklab_to_linear_srgb(float3 lab) {
	//L'M'S'
	float3 lms_ = mul(oklab_to_oklms_, lab);

	//LMS
	float3 lms = lms_ * lms_ * lms_;

	return mul(oklms_to_srgb, lms);
}

// (in) OKLab
// (out) linear BT.2020
float3 oklab_to_linear_bt2020(float3 lab) {
	//L'M'S'
	float3 lms_ = mul(oklab_to_oklms_, lab);

	//LMS
	float3 lms = lms_ * lms_ * lms_;

	return mul(oklms_to_bt2020, lms);
}

float3 oklab_to_oklch(float3 lab) {
	float L = lab[0];
	float a = lab[1];
	float b = lab[2];
	return float3(
		L,
		sqrt((a*a) + (b*b)),
		atan2(b, a)
	);
}

float3 oklch_to_oklab(float3 lch) {
	float L = lch[0];
	float C = lch[1];
	float h = lch[2];
	return float3(
		L,
		C * cos(h),
		C * sin(h)
	);
}

// (in) linear sRGB/BT.709
// (out) OKLch:
// L – perceived lightness (identical to OKLAB)
// c – chroma (saturation)
// h – hue
float3 linear_srgb_to_oklch(float3 rgb) {
	return oklab_to_oklch(
		linear_srgb_to_oklab(rgb)
	);
}

// (in) linear BT.2020
// (out) OKLch
float3 linear_bt2020_to_oklch(float3 rgb) {
	return oklab_to_oklch(
		linear_bt2020_to_oklab(rgb)
	);
}

// (in) OKLch
// (out) sRGB/BT.709
float3 oklch_to_linear_srgb(float3 lch) {
	return oklab_to_linear_srgb(
			oklch_to_oklab(lch)
	);
}

// (in) OKLch
// (out) BT.2020
float3 oklch_to_linear_bt2020(float3 lch) {
	return oklab_to_linear_bt2020(
			oklch_to_oklab(lch)
	);
}

// This is almost a perfect approximation of sRGB gamma, but it clips any color below 0.055.
float3 gamma_linear_to_sRGB_Bethesda_Optimized(float3 Color, float InverseGamma = 1.f / 2.4f)
{
	return (pow(Color, InverseGamma) * 1.055f) - 0.055f;
}

	// Original exact inverse formula, this is heavily broken, especially in its inverse form
float3 gamma_sRGB_to_linear_Bethesda_Optimized(float3 Color, float Gamma = 2.4f)
{
	return (pow(Color, Gamma) / 1.055f) + 0.055f;
}

//L'M'S'->ICtCp
static const float3x3 PQ_LMS_2_ICtCp = {
	0.5f,             0.5f,             0.f,
	1.61376953125f,  -3.323486328125f,  1.709716796875f,
	4.378173828125f, -4.24560546875f,  -0.132568359375f};

//ICtCp->L'M'S'
//the 1s get optimized away by the compiler
static const float3x3 ICtCp_2_PQ_LMS = {
	1.f,  0.008609036915004253387451171875f,  0.11102962493896484375f,
	1.f, -0.008609036915004253387451171875f, -0.11102962493896484375f,
	1.f,  0.560031354427337646484375f,       -0.3206271827220916748046875f};

//RGB BT.709->LMS
static const float3x3 BT709_2_LMS = {
	0.295654296875f, 0.623291015625f, 0.0810546875f,
	0.156005859375f, 0.7275390625f,   0.116455078125f,
	0.03515625f,     0.15673828125f,  0.807861328125f};

//LMS->RGB BT.709
static const float3x3 LMS_2_BT709 = {
	 6.171343326568603515625f,          -5.318845272064208984375f,      0.14753799140453338623046875f,
	-1.3213660717010498046875f,          2.5573856830596923828125f,    -0.23607718944549560546875f,
	-0.012195955030620098114013671875f, -0.2647107541561126708984375f,  1.27721846103668212890625f};

//ICtCp->L'M'S'
float3 ICtCp_to_PQ_LMS(float3 Colour)
{
	return mul(ICtCp_2_PQ_LMS, Colour);
}

//L'M'S'->ICtCp
float3 PQ_LMS_to_ICtCp(float3 Colour)
{
	return mul(PQ_LMS_2_ICtCp, Colour);
}

//RGB BT.709->LMS
float3 BT709_to_LMS(float3 Colour)
{
	return mul(BT709_2_LMS, Colour);
}

//LMS->RGB BT.709
float3 LMS_to_BT709(float3 Colour)
{
	return mul(LMS_2_BT709, Colour);
}

float3 BT709_to_ICtCp(float3 Colour)
{
	// Keep division by "PQMaxWhitePoint" here (BT709_to_LMS()) instead of in "Linear_to_PQ()" for floating point accuracy
	float3 LMS = BT709_to_LMS(Colour / PQMaxWhitePoint);
	float3 PQ_LMS = Linear_to_PQ(LMS);
	return PQ_LMS_to_ICtCp(PQ_LMS);
}

float3 ICtCp_to_BT709(float3 Colour)
{
	float3 PQ_LMS = ICtCp_to_PQ_LMS(Colour);
	// max should be save as LMS should be only positive
	float3 LMS = max(PQ_to_Linear(PQ_LMS), 0.f);
	float3 RGB = LMS_to_BT709(LMS);
	return RGB * PQMaxWhitePoint;
}


static const float3x3 BT709_2_AP1D65 = {
	0.61685097217559814453125,     0.33406293392181396484375,     0.049086071550846099853515625,
	0.069866396486759185791015625, 0.91741669178009033203125,     0.012716926634311676025390625,
	0.020549066364765167236328125, 0.107642211019992828369140625, 0.871808707714080810546875};

static const float3x3 AP1D65_2_BT709 = {
	 1.69267940521240234375,          -0.606218039989471435546875,   -0.086461342871189117431640625,
	-0.1285739839076995849609375,      1.13793361186981201171875,    -0.009359653107821941375732421875,
	-0.02402246557176113128662109375, -0.12621171772480010986328125,  1.150234222412109375};

static const float3x3 AP1D65_2_XYZ = {
	 0.647292673587799072265625,        0.13440339267253875732421875, 0.1684710681438446044921875,
	 0.26599824428558349609375,         0.676089823246002197265625,   0.0579119287431240081787109375,
	-0.0054470631293952465057373046875, 0.00407283008098602294921875, 1.0897972583770751953125};

static const float3x3 WIDE_2_AP1D65 = {
	0.83451688289642333984375,        0.16025958955287933349609375,    0.005223505198955535888671875,
	0.02554519288241863250732421875,  0.973101556301116943359375,      0.0013532745651900768280029296875,
	0.001925828866660594940185546875, 0.03037279658019542694091796875, 0.967701375484466552734375};

// Expand bright saturated BT.709 colors onto BT.2020 to achieve a fake HDR look.
// Input (and output) needs to be in sRGB linear space.
// Calling this with a value of 0 still results in changes (avoid doing so, it might produce invalid colors).
// Calling this with values above 1 yields diminishing returns.
float3 ExtendGamut(float3 Color, float ExtendGamutAmount = 1.f)
{
	float3 ColorAP1    = mul(BT709_2_AP1D65, Color);
	float3 ColorExpand = mul(WIDE_2_AP1D65,  Color);

	float  LumaAP1   = dot(ColorAP1, AP1D65_2_XYZ[1]);
	float3 ChromaAP1 = ColorAP1 / LumaAP1;
	if (LumaAP1 <= 0.f) // Skip invalid colors
		return Color;

	float3 ChromaAP1Minus1  = ChromaAP1 - 1.f;
	float  ChromaDistSqr    = dot(ChromaAP1Minus1, ChromaAP1Minus1);
	float  ExtendGamutAlpha = (1.f - exp2(-4.f * ChromaDistSqr)) * (1.f - exp2(-4.f * ExtendGamutAmount * LumaAP1 * LumaAP1));

	ColorAP1 = lerp(ColorAP1, ColorExpand, ExtendGamutAlpha);

	Color = mul(AP1D65_2_BT709, ColorAP1);
	return Color;
}

float3 bt2446_hdr_to_sdr(float3 linRGB, float lHDR, float lSDR) {
	const float3 rgb = pow(linRGB, 1.f/2.4f);

	const float pHDR = 1.f + (32.f * pow(lHDR / 10000.0f, 1.f / 2.4f));
	const float pSDR = 1.f + (32.f * pow(lSDR / 10000.0f, 1.f / 2.4f));

	const float Y = Luminance(rgb); // Luma

	const float Yp = log(1.f + (pHDR - 1.0) * Y) / log(pHDR);

	const float Yc = (Yp < 0.7399f) ? 1.0770f * max(0, Yp)
		: (Yp < 0.9909f) ? (-1.1510f * Yp * Yp) + (2.7811f * Yp) - 0.6302f
		: 0.5000f * min(Yp, 1.f) + 0.5000f;

	const float Ysdr = (pow(pSDR, Yc) - 1.f) / (pSDR - 1.f);

	const float fYsdr = Ysdr / (1.1f * Y);
	const float Cbtmo = fYsdr * ((rgb.b - Y) / 1.8556);
	const float Crtmo = fYsdr * ((rgb.r - Y) / 1.5748);
	const float Ytmo = Ysdr - max(0.1f * Crtmo, 0);

	float3 outRGB = float3(
			Ytmo + (1.5748f * Crtmo),
			Ytmo - (0.2126f * 1.5748f / 0.7152f) * Crtmo - (0.0722f * 1.8556f / 1.5748f) * Cbtmo,
			Ytmo + (1.8556 * Cbtmo)
	);
	float3 outLinRgb = pow(max(0, outRGB), 2.4f);
	return outLinRgb;
}

float toneMapGainBT2446a(float Y_HDR, float L_HDR, float L_SDR) {
	float rho_hdr = 1.0 + 32.0 * pow(L_HDR / 10000.0, 1.0/2.4);
	float rho_sdr = 1.0 + 32.0 * pow(L_SDR / 10000.0, 1.0/2.4);

	// Y_HDR is "A normalized full-range linear display-light HDR signal"
	float Yprime = pow(Y_HDR / L_HDR, 1.0/2.4);

	float Yp_prime = log(1.0 + (rho_hdr - 1.0) * Yprime) / log(rho_hdr);
	float Yc_prime = 0.0;
	if (Yp_prime < 0.0) {
		Yc_prime = 0.0;
	} else if (Yp_prime < 0.7399) {
		Yc_prime = 1.0770 * Yp_prime;
	} else if (Yp_prime < 0.9909) {
		Yc_prime = -1.1510 * pow(Yp_prime, 2.0) + 2.7811*Yp_prime - 0.6302;
	} else if (Yp_prime <= 1.0) {
		Yc_prime = 0.5 * Yp_prime + 0.5;
	} else {
		Yc_prime = 1.0;
	}
	float Ysdr_prime = (pow(rho_sdr, Yc_prime) - 1.0) / (rho_sdr - 1.0);

	float Ysdr = pow(Ysdr_prime, 2.4);
	return (Ysdr * L_SDR) / Y_HDR;
}

// From Chromium dev
float3 bt2446_bt709(float3 linRGB, float lHDR, float lSDR) {
	float3 tonemap_input = BT709_To_BT2020(linRGB);
	float3 gain = float3(toneMapGainBT2446a(tonemap_input.r, lHDR, lSDR),
                      toneMapGainBT2446a(tonemap_input.g, lHDR, lSDR),
                      toneMapGainBT2446a(tonemap_input.b, lHDR, lSDR));
	tonemap_input *= gain;
	return BT2020_To_BT709(tonemap_input);
}
