#include "DataStructures.hlsli"
#include "MathFunctions.hlsli"

Texture2D<float4> g_DIInput : register(t0);
Texture2D<float4> g_AlbedoInput : register(t1);
Texture2D<float4> g_NormalInput : register(t2);
Texture2D<float4> g_VertexPositionInput : register(t3);
Texture2D<float> g_DepthInput : register(t4);
Texture2D<float> g_varianceInput : register(t5);
Texture2D<float2> g_PartialDepth : register(t6);
Texture2D<float4> g_DenoiserInput : register(t7);

RWTexture2D<float4> g_output : register(u0);

ConstantBuffer<DenoiserData> denoiserData : register(b0);

SamplerState SimpleSampler : register(s0);
const static float AtrousFilterKernel[5] =
{
	0.0625, 0.25, 0.375, 0.25, 0.00625
};

const static float kernelWeights[3] = { 1.0, 2.0 / 3.0, 1.0 / 6.0 };
	

float CalculateColorWeight(uint2 basePixel, uint2 offsetPixel)
{
	float weight;
	float weightSigma = 4;
	float4 currentColor = Luminance(g_DenoiserInput[basePixel].xyz);
	float4 offsetColor = Luminance(g_DenoiserInput[offsetPixel].xyz);
	float4 t = currentColor - offsetColor;
	float exponentialComponent = dot(t, t);
	weight = min(exp(-(exponentialComponent) / weightSigma), 1.0);
	return weight;
}

float CalculatePositionWeight(uint2 basePixel , uint2 offsetPixel)
{
	float weight;
	float weightSigma = 1; 
	float4 currentPosition = g_VertexPositionInput[basePixel];
	float4 offsetPosition = g_VertexPositionInput[offsetPixel];
	float4 t = currentPosition - offsetPosition;
	float exponentialComponent = dot(t, t);
	weight = min(exp(-(exponentialComponent) / weightSigma), 1.0);
	return weight;
}

float CalculateNormalWeight(uint2 basePixel, uint2 offsetPixel)
{
	//float weight;
	//float weightsigma = 128;
	//float4 currentnormal = g_NormalInput[basePixel];
	//float4 offsetnormal = g_NormalInput[offsetPixel];
	//float4 t = currentnormal - offsetnormal;
	//float exponentialcomponent = max(dot(t, t), 0.0);
	//weight = min(exp(-(exponentialcomponent) / weightsigma), 1.0);
	//return weight;
	
	float weight;
	float weightSigma = 64.0f;
	float4 currentNormal = g_NormalInput[basePixel];
	float4 offsetNormal = g_NormalInput[offsetPixel];
	float dotNormal = dot(currentNormal, offsetNormal);
	weight = max(0.0f, dotNormal);
	weight = pow(weight, weightSigma);
	
	return weight;
}


float CalculateDepthWeight(uint2 DTid, uint2 offsetPixel)
{
	//float weight;
	//float weightSigma = pow(1.1, 2);
	//float currentDepth = g_DepthInput[DTid];
	//float offsetDepth = g_DepthInput[offsetPixel];
	//float t = currentDepth - offsetDepth;
	//float exponentialComponent = max(t, 0.0);
	//weight = min(exp(-(exponentialComponent) / weightSigma), 1.0);
	//return weight;
	
	float weight;
	float weightSigma = 1.0f;
	float2 partialDepth = g_PartialDepth[DTid];
	uint2 screenSpaceOffset = abs(offsetPixel - DTid);
	float epsilon = 0.005f;
	float currentDepth = g_DepthInput[DTid];
	float offsetDepth = g_DepthInput[offsetPixel];
	float t = -abs(currentDepth - offsetDepth);
	float gradient = abs((weightSigma * dot(screenSpaceOffset, partialDepth))) + epsilon;
	t = t / gradient;
	weight = exp(t);
	return weight;

}
float CalculateVarianceWeight(uint2 DTid, uint2 offsetPixel)
{
	//float weight;
	//float weightSigma = pow(1.1, 2);
	//float currentVariance = g_varianceInput[DTid];
	//float offsetVariance = g_varianceInput[offsetPixel];
	//float t = currentVariance - offsetVariance;
	//float exponentialComponent = max(t, 0.0);
	//weight = min(exp(-(exponentialComponent) / weightSigma), 1.0);
	//return weight;
	float weight;
	float weightSigma = 4.0f;
	float epsilon = 0.01f;
	float currentLuminance = Luminance(g_DenoiserInput[DTid].rgb);
	float offsetLuminance = Luminance(g_DenoiserInput[offsetPixel].rgb);
	float variance = g_varianceInput[DTid];
	float t = -abs(currentLuminance - offsetLuminance);
	float gradient = (weightSigma * variance) + epsilon;
	weight = exp(t / gradient);
	return weight;

}

[numthreads(8, 8, 1)]
void main(uint2 DTid : SV_DispatchThreadID)
{
	float4 denoisedOutput = float4(0,0,0,0);
	int ikernel = -1, jkernel = -1;
	int pixelOffset =denoiserData.atrousStepSize; //  0 , 1 , 2
	//pixelOffset = 0;  //  0 , 1 , 2
	pixelOffset = pow(2, pixelOffset);              //  1 , 2 , 4
	int kernelOffsetSize = pixelOffset * 2;			//  2 , 4 , 8
	
	uint width, height;
	g_output.GetDimensions(width, height);
	uint2 textureDimensions = uint2(width, height);
	float cumulativeKernelWeight = 0.0f;
	float varianceWeight = 0.0f;
	float totalWeight = 0.0f;
	
	float partialDepth = max(g_PartialDepth[DTid].x, g_PartialDepth[DTid].y);
	float depth = g_DepthInput[DTid];
	//float variance = ComputeVarianceGaussian(DTid);
	const float epsVariance = 1e-10;
	const float indirectCenterLuminance = Luminance(g_DenoiserInput.Load(int3(DTid, 0)).rgb);
	const float depthSigma = max(partialDepth, 1e-8) * denoiserData.atrousStepSize;
	//const float indirectSigma = 10.0f * sqrt(max(0.0, epsVariance + variance));
	float4 indirectSums = float4(0, 0, 0, 0);
	
	for (int i = -kernelOffsetSize; i <= kernelOffsetSize; i += pixelOffset)
	{
		ikernel++;
		for (int j = -kernelOffsetSize; j <= kernelOffsetSize; j += pixelOffset)
		{
			jkernel++;
			uint2 indexOffset = uint2(i, j);
			const float kernel = kernelWeights[abs(i)] * kernelWeights[abs(j)];
			bool isWithinDimensions = GetIsWithinDimesions(DTid + indexOffset, textureDimensions);
			if (isWithinDimensions && (i != 0 || j != 0))
			{
				float normalWeight = CalculateNormalWeight(DTid, DTid + indexOffset);
				float positionWeight = CalculatePositionWeight(DTid, DTid + indexOffset);
				float colorWeight = CalculateColorWeight(DTid, DTid + indexOffset);
				float depthWeight = CalculateDepthWeight(DTid, DTid + indexOffset);
				varianceWeight = CalculateVarianceWeight(DTid, DTid + indexOffset);
				float kernelWeight = (AtrousFilterKernel[ikernel] + AtrousFilterKernel[jkernel]) / 2.0f;
				float totalWeight = 0.0f;
				if (denoiserData.temporalFadeVarianceEstimation.y == 1.0f)
				{
					totalWeight = normalWeight * depthWeight * varianceWeight;
				}
				else
				{
					totalWeight = normalWeight * depthWeight;
				}
				cumulativeKernelWeight += totalWeight;
				denoisedOutput += (g_DenoiserInput[DTid + indexOffset] * totalWeight);

			}
		}
	}
	denoisedOutput /= cumulativeKernelWeight;
	g_output[DTid] = denoisedOutput;
	
	
	//denoisedOutput = float4(indirectSums / totalWeight);
	//g_output[DTid] = denoisedOutput;
	
	
}
