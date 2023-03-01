#include "DataStructures.hlsli"
#include "MathFunctions.hlsli"
//------------------SRVS----------------------
Texture2D<float4> g_DIInput : register(t0);
Texture2D<float4> g_AlbedoInput : register(t1);
Texture2D<float4> g_NormalInput : register(t2);
Texture2D<float4> g_PreviousGIInput : register(t3);
Texture2D<float4> g_PreviousNormalInput : register(t4);
Texture2D<float2> g_MotionVector : register(t5);
Texture2D<float> g_DepthInput : register(t6);
Texture2D<float> g_PreviousFrameDepthInput : register(t7);
Texture2D<float4> g_PreviousTemporalOutput : register(t8);
Texture2D<float4> g_VertexPosition : register(t9);
Texture2D<float2> g_PreviousMoments : register(t10);
Texture2D<float> g_PreviousHistoryLength : register(t11);
Texture2D<float2> g_PartialDerivates : register(t12);
Texture2D<float4> g_GIInput : register(t13);

//-----------------UAVS----------------------
RWTexture2D<float4> g_temporalOutput : register(u1);
RWTexture2D<float2> g_Moments : register(u2);
RWTexture2D<float> g_HistoryLength : register(u3);

//-----------------CB----------------------
ConstantBuffer<DenoiserData> denoiserData : register(b0);
SamplerState clampSampler : register(s0);

float ReprojectWeight(int2 previousDTid, float currentDepth, float previousDepth, float3 currentNormal, float3 previousNormal, float width)
{
	float weight = 0.0f;
	return weight;
}

bool CheckReprojectionValidity(int2 previousDTid, float currentDepth, float previousDepth, float3 currentNormal, float3 previousNormal , float width)
{
	float depthSigma = 128.0f;
	float normalSigma = 16.0f;
	const float2 imageDimensions = GetTextureDimensions(g_DIInput);
	bool isWithinDim = GetIsWithinDimesions(previousDTid, imageDimensions);
	bool depthDeviation = (abs(previousDepth - currentDepth) / (width + 1e-4)) > depthSigma;
	//bool normalDeviation = (abs(distance(currentNormal, previousNormal)) / (normalSigma + 1e-2)) > 16.0f;
	
	return isWithinDim && !depthDeviation /*&& !normalDeviation*/;

}

bool CheckPreviousSample(uint2 DTid, out float4 previousIndirect, out float2 previousMoment, out float previousHistory)
{
	float2 motionVector = g_MotionVector[DTid];
	const float2 imageDimensions = GetTextureDimensions(g_DIInput);
	const int2 previousDTid = int2(float2(DTid) + motionVector.xy * imageDimensions + float2(0.5, 0.5));
	
	float currentDepth = g_DepthInput[DTid];
	float3 currentNormal = g_NormalInput[DTid].xyz;
	float2 currentPartialDepth = g_PartialDerivates[DTid];
	float maxPDChange = max(currentPartialDepth.x, currentPartialDepth.y)/* * 10.0f*/;
	maxPDChange = length(currentPartialDepth);
	

	const float2 posPrev = floor(DTid.xy) + motionVector.xy * imageDimensions;
	int2 offset[4] = { int2(0, 0), int2(1, 0), int2(0, 1), int2(1, 1) };
    
	bool isProjectionValid = false;
	bool bilateralValids[4];
	int2 offsets[4] = { int2(0, 0), int2(1, 0), int2(0, 1), int2(1, 1) };
	for (int i = 0; i < 4; i++)
	{
		int2 indexOffset = int2(posPrev) + offset[i];
		float depthSample = g_PreviousFrameDepthInput[indexOffset];
		float3 normalSample = g_PreviousNormalInput[indexOffset].xyz;
		bilateralValids[i] = CheckReprojectionValidity(previousDTid, currentDepth, depthSample, currentNormal, normalSample, maxPDChange);
		isProjectionValid = isProjectionValid || bilateralValids[i];
	}
	
	if (isProjectionValid)
	{
		float totalWeights = 0.0f;
		float x = frac(posPrev.x);
		float y = frac(posPrev.y);
		float individualWeights[4] ={(1 - x) * (1 - y),x * (1 - y),(1 - x) * y,x * y};
		for (int i = 0; i < 4; i++)
		{
			int2 indexOffset = int2(posPrev) + offset[i];
			if(bilateralValids[i])
			{
				previousIndirect += individualWeights[i] * g_PreviousTemporalOutput[indexOffset];
				totalWeights += individualWeights[i];
			}
		}
		isProjectionValid = totalWeights >= 0.01f;
		if (isProjectionValid)
		{
			previousIndirect = previousIndirect / totalWeights;
		}
		else
		{
			previousIndirect = float4(0, 0, 0, 0);
		}
		
	}
	if (!isProjectionValid)
	{
		float sampleCounts = 0.0f;
		const int kernelRadius =1;
		for (int x = -kernelRadius; x <= kernelRadius; x++)
		{
			for (int y = -kernelRadius; y <= kernelRadius; y++)
			{
				int2 samplePos = previousDTid + int2(x, y);
				float depthSample = g_PreviousFrameDepthInput[samplePos];
				float3 normalSample = g_PreviousNormalInput[samplePos].xyz;

				bool reprojectionValid = CheckReprojectionValidity(previousDTid, currentDepth, depthSample, currentNormal, normalSample, maxPDChange);
				if (reprojectionValid)
				{
					previousIndirect += g_PreviousTemporalOutput[samplePos];
					sampleCounts += 1.0f;
				}

			}
		}
		if (sampleCounts > 0.0f)
		{
			isProjectionValid = true;
			previousIndirect /= sampleCounts;
		}
	}
	if (isProjectionValid)
	{
		previousHistory = g_PreviousHistoryLength[previousDTid];
	}
	else
	{
		previousIndirect = float4(0, 0, 0, 0);
		previousHistory = 0.0f;
	}
	
	return isProjectionValid;
}
[numthreads(8,8,1)]
void main(uint2 DTid : SV_DispatchThreadID)
{
	float temporalAccumulationFactor = denoiserData.temporalFade;
	float momentsAccumulationFactor = 0.2f;
	
	float4 currentGI = g_GIInput[DTid];
	float4 previousGI = float4(0, 0, 0, 0);
	
	float previousMoment = 0.0f;
	float updateHistory = 0.0f;
	bool success = CheckPreviousSample(DTid, previousGI, previousMoment, updateHistory);
	
	updateHistory = min(32.0f, success ? updateHistory + 1.0f : 1.0f);
	//previousHistory = previousHistory + 1.0f;
	float temporalFade = success ? max(temporalAccumulationFactor, 1 / updateHistory) : 1.0f;
	

	float luminance = Luminance(currentGI.xyz);
	
	float4 temporalOutput = float4(0,0,0,0);

	g_HistoryLength[DTid] = updateHistory;
	float2 motionVector = g_MotionVector[DTid];
	const float2 imageDimensions = GetTextureDimensions(g_DIInput);
	const int2 previousDTid = int2(float2(DTid) + motionVector.xy * imageDimensions + float2(0.5, 0.5));
	float4 previoustemporalcolor;
	previoustemporalcolor = g_PreviousTemporalOutput[previousDTid];
	temporalOutput = lerp(previoustemporalcolor, currentGI,temporalFade);
	
	temporalOutput.a = 0.0f;
	//--CHECKING FOR EMISSIVE SURFACE HIT---
	//if (previoustemporalcolor.a != 0.0f || currentGI.a != 0.0f)
	//{
	//	temporalOutput = float4(1, 0, 0, 1);
	//}
	//g_Moments[DTid].x = temporalFade;
	//if ( /*motionVector.x == 0.0f && motionVector.y == 0.0f ||*/success)
	//{
	//	//temporalOutput = previoustemporalcolor;
	//	g_HistoryLength[DTid] = 32.0f;
	//}
	
	g_temporalOutput[DTid] = temporalOutput;
	
}