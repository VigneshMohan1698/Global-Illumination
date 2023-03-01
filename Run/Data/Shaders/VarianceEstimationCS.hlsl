#include "DataStructures.hlsli"
#include "MathFunctions.hlsli"

//-----------------UAVS----------------------
RWTexture2D<float> g_varianceOutput : register(u0);

//------------------SRVS----------------------
Texture2D<float4> g_GIInput : register(t0);
Texture2D<float2> g_MotionVectorInput : register(t1);
Texture2D<float> g_History : register(t2);
Texture2D<float> g_DepthInput : register(t3);
Texture2D<float2> g_PartialDerivativeDepth : register(t4);
Texture2D<float4> g_DIInput : register(t5);

//-----------------CB----------------------
ConstantBuffer<DenoiserData> denoiserData : register(b0);
SamplerState clampSampler : register(s0);

[numthreads(8, 8, 1)]
void main(uint2 DTid : SV_DispatchThreadID)
{
	const float kernelRadius = denoiserData.kernelSize;
	float2 numberOfSamples = 0.0f;
	float2 variancePair = float2(0.0f, 0.0f);
	
	for (int i = -3; i <= 3; i++)
	{
		for (int j = -3; j <= 3; j++)
		{
			if (i == 0 && j == 0)
			{
				continue;
			}
			int2 offset = DTid + int2(i, j);
			float3 colorCoordinate = g_GIInput[offset].xyz;
			float sampleLuminance = Luminance(colorCoordinate);
			float sampSquared = sampleLuminance * sampleLuminance;
			variancePair += float2(sampleLuminance, sampSquared);
			
			numberOfSamples += 1.0f;
		}
	}
	variancePair /= numberOfSamples;
	g_varianceOutput[DTid] = sqrt(max(0.0, variancePair.y - variancePair.x * variancePair.x));
	
	
}


