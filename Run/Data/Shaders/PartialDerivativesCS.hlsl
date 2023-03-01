#include "DataStructures.hlsli"
#include "MathFunctions.hlsli"

//------------------SRVS----------------------
Texture2D<float> g_DepthInput : register(t0);

//-----------------UAVS----------------------
RWTexture2D<float2> g_OutputDepthDerivatives : register(u0);

//-----------------CB----------------------
ConstantBuffer<DenoiserData> denoiserData : register(b0);

[numthreads(8,8,1)]
void main(uint2 DTid : SV_DispatchThreadID)
{
	uint width, height;
	g_DepthInput.GetDimensions(width, height);
	uint2 textureDimensions = uint2(width, height);
	uint2 top = clamp(DTid.xy + uint2(0, -1), 0, textureDimensions - 1);
	uint2 bottom = clamp(DTid.xy + uint2(0, 1), 0, textureDimensions - 1);
	uint2 left = clamp(DTid.xy + uint2(-1, 0), 0, textureDimensions - 1);
	uint2 right = clamp(DTid.xy + uint2(1, 0), 0, textureDimensions - 1);

	float center = g_DepthInput[DTid.xy];
	float2 backwardDiff = center - float2(g_DepthInput[left], g_DepthInput[top]);
	float2 forwardDiff = float2(g_DepthInput[right], g_DepthInput[bottom]) - center;
	
	float2 ddx = float2(backwardDiff.x, forwardDiff.x);
	float2 ddy = float2(backwardDiff.y, forwardDiff.y);
	
	
	uint2 minIndex =
	{
		GetIndexClosest(0, ddx),
        GetIndexClosest(0, ddy)
	};
	float2 ddxy = float2(ddx[minIndex.x], ddy[minIndex.y]);
	float max = 1;
	float2 signValue = sign(ddxy);
	ddxy = signValue * min(abs(ddxy), max);
	
	g_OutputDepthDerivatives[DTid] = ddxy;
	
	
}

