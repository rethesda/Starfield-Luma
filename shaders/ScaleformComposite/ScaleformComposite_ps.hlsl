#include "../shared.hlsl"
#include "../color.hlsl"
#include "RootSignature.hlsl"

// Hack: change the alpha value at which the UI blends in in HDR, to increase readability. Range is 0 to 1, with 1 having no effect.
#define HDR_UI_BLEND_POW 0.775f

struct PushConstantWrapper_ScaleformCompositeLayout
{
	int Unknown1;
	int Unknown2;
};

cbuffer stub_PushConstantWrapper_ScaleformCompositeLayout : register(b0)
{
	PushConstantWrapper_ScaleformCompositeLayout Layout : packoffset(c0);
};

Texture2D<float4> inputTexture : register(t0, space8);
SamplerState inputSampler : register(s0, space8);

struct PSInputs
{
	float4 uv : TEXCOORD0;
	float4 pos : SV_Position;
};

[RootSignature(ShaderRootSignature)]
float4 PS(PSInputs inputs) : SV_Target
{
	float4 UIColor = inputTexture.Sample(inputSampler, float2(inputs.uv.x, inputs.uv.y));
	float UIIntensity = asfloat(Layout.Unknown1);

	// Theoretically all UI is in sRGB (though it might have been designed on gamma 2.2 screens, we can't know that, but it was definately targeting gamma 2.2 as output anyway).
	UIColor.xyz = gamma_sRGB_to_linear(UIColor.xyz);
	UIColor.xyz = UIColor.xyz * UIIntensity;
#if ENABLE_HDR
#if SDR_USE_GAMMA_2_2
	UIColor.xyz = pow(gamma_linear_to_sRGB(UIColor.xyz), 2.2f);
#endif // HDR_GAMMA_CORRECTION
	UIColor.xyz *= HdrDllPluginConstants.HDRUIPaperWhiteNits / WhiteNits_BT709;
	// Scale alpha to emulate sRGB gamma blending (we blend in linear space in HDR),
	// this won't ever be perfect but it's close enough for most cases.
	// We do a saturate to avoid pow of -0, which might lead to unexpected results.
	UIColor.a = pow(saturate(UIColor.a), HDR_UI_BLEND_POW); //TODO: base the percentage of application of "HDR_UI_BLEND_POW" based on how black/dark the UI color is.
#else
	UIColor.xyz = gamma_linear_to_sRGB(UIColor.xyz);
#endif // ENABLE_HDR

	return UIColor;
}