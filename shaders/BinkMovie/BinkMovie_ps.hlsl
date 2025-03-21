#include "../shared.hlsl"
#include "../color.hlsl"
#include "RootSignature.hlsl"

Texture2D<float> TexY  : register(t0, space8);
Texture2D<float> TexCb : register(t2, space8);
Texture2D<float> TexCr : register(t1, space8);

SamplerState Sampler0 : register(s0, space8);

struct PSInputs
{
	float4 pos : SV_Position;
	float2 TEXCOORD : TEXCOORD0;
};

#if SDR_USE_GAMMA_2_2
	#define GAMMA_TO_LINEAR(x) gamma_to_linear(x)
	#define LINEAR_TO_GAMMA(x) linear_to_gamma(x)
#else
	#define GAMMA_TO_LINEAR(x) gamma_sRGB_to_linear(x)
	#define LINEAR_TO_GAMMA(x) gamma_linear_to_sRGB(x)
#endif

//TODO: maybe expose the shoulder pow and max nits to user? it's probably better to simply pick a default that looks nice on most movies.
static const float BinkVideosAutoHDRMaxOutputNits = 750.f;
// The higher it is, the "later" highlights start
static const float BinkVideosAutoHDRShoulderPow = 2.75f; // A somewhat conservative value

// AutoHDR pass to generate some HDR brightess out of an SDR signal (it has no effect if HDR is not engaged).
// This is hue conserving and only really affects highlights.
// https://github.com/Filoppi/PumboAutoHDR
float3 PumboAutoHDR(float3 SDRColor, float MaxOutputNits, float PaperWhite)
{
	const float SDRRatio = max(Luminance(SDRColor), 0.f);
	// Limit AutoHDR brightness, it won't look good beyond a certain level.
	// The paper white multiplier is applied later so we account for that.
	const float AutoHDRMaxWhite = max(min(MaxOutputNits, BinkVideosAutoHDRMaxOutputNits) / PaperWhite, WhiteNits_sRGB) / WhiteNits_sRGB;
	const float AutoHDRShoulderRatio = 1.f - max(1.f - SDRRatio, 0.f);
	const float AutoHDRExtraRatio = pow(max(AutoHDRShoulderRatio, 0.f), BinkVideosAutoHDRShoulderPow) * (AutoHDRMaxWhite - 1.f);
	const float AutoHDRTotalRatio = SDRRatio + AutoHDRExtraRatio;
	return SDRColor * safeDivision(AutoHDRTotalRatio, SDRRatio);
}

[RootSignature(ShaderRootSignature)]
float4 PS(PSInputs inputs) : SV_Target
{
	float Y = TexY.Sample(Sampler0, inputs.TEXCOORD.xy).x;
	float Cb = TexCb.Sample(Sampler0, inputs.TEXCOORD.xy).x;
	float Cr = TexCr.Sample(Sampler0, inputs.TEXCOORD.xy).x;

	float3 color;
	// usually in YCbCr the ranges are (in float):
	// Y:   0.0-1.0
	// Cb: -0.5-0.5
	// Cr: -0.5-0.5
	// but since this is a digital signal (in unsinged 8bit: 0-255) it's now:
	// Y:  0.0-1.0
	// Cb: 0.0-1.0
	// Cr: 0.0-1.0
	// the formula adjusts for that but was for BT.601 limited range while the video is definitely BT.709 full range (proven by bink meta data)
	// so the matrix paramters have been adjusted for BT.709 full range (both the brightness and hue of all colors would have previously been wrong)
#if 1 // Rec.709 full range
	color.r = (Y - 0.790487825870513916015625f) + (Cr * 1.5748f);
	color.g = (Y + 0.329009473323822021484375f) - (Cr * 0.46812427043914794921875f) - (Cb * 0.18732427060604095458984375f);
	color.b = (Y - 0.931438446044921875f)       + (Cb * 1.8556f);
#elif 0 // Rec.709 limited range
    Y *= 1.16438353f;
    color.r = (Y - 0.972945094f) + (Cr * 1.79274106f);
    color.g = (Y + 0.301482677f) - (Cr * 0.532909333f) - (Cb * 0.213248610f);
    color.b = (Y - 1.13340222f)  + (Cb * 2.11240172f);
#else // Rec.601 limited range (vanilla)
    Y *= 1.16412353515625f;
    color.r = (Y - 0.870655059814453125f) + (Cr * 1.595794677734375f);
    color.g = (Y + 0.529705047607421875f) - (Cr * 0.8134765625f) - (Cb * 0.391448974609375f);
    color.b = (Y - 1.081668853759765625f) + (Cb * 2.017822265625f);
#endif

	// Clamp for safety as YCbCr<->RGB is not 100% accurate in float and can produce negative/invalid colors,
	// this breaks the UI pass if we are using R16G16B16A16F textures,
	// as UI blending produces invalid pixels if it's blended with an invalid color.
	// Also this is need for the gamma pow.
	color = max(color, 0.f);

#if !SDR_LINEAR_INTERMEDIARY
	if (HdrDllPluginConstants.DisplayMode > 0)
#endif // SDR_LINEAR_INTERMEDIARY
	{
		// Note: we ignore "HdrDllPluginConstants.GammaCorrection" here, as we fully rely on "SDR_USE_GAMMA_2_2"
		// Note: "ApplyGammaBelowZeroDefault" doesn't matter here as the UI should only have positive scRGB values (negative values would have been accidental)
		color = GAMMA_TO_LINEAR(color);

		if (HdrDllPluginConstants.DisplayMode > 0)
		{
			const float paperWhite = HdrDllPluginConstants.HDRGamePaperWhiteNits / WhiteNits_sRGB;
			if (HdrDllPluginConstants.AutoHDRVideos)
			{
				color = PumboAutoHDR(color, HdrDllPluginConstants.HDRPeakBrightnessNits, paperWhite);
			}

			color *= paperWhite; // Use the game brightness, not the UI one, as these are usually videos that are seamless with gameplay
		}
	}

	return float4(color, 1.0f);
}