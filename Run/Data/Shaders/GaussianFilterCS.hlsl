#include "DataStructures.hlsli"
#include "MathFunctions.hlsli"

Texture2D<float4> g_DIInput : register(t0);
Texture2D<float4> g_AlbedoInput : register(t1);
Texture2D<float4> g_NormalInput : register(t2);
Texture2D<float4> g_GIInput : register(t3);

RWTexture2D<float4> g_output : register(u0);
ConstantBuffer<DenoiserData> denoiserData : register(b0);

SamplerState SimpleSampler : register(s0);


static const float weights7X7[7][7] =
{
	{ 1, 2, 4, 8, 4, 2, 1 },
	{ 2, 4, 8, 16, 8, 4, 2 },
	{ 4, 8, 16, 32, 16, 8, 4 },
	{ 8, 16, 32, 64, 32, 16, 8 },
	{ 4, 8, 16, 32, 16, 8, 4 },
	{ 2, 4, 8, 16, 8, 4, 2 },
	{ 1, 2, 4, 8, 4, 2, 1 },
};

static const float weights9X9[9][9] =
{
	{ 0.00625, 0.125,  0.25,   0.5,   1,   0.5,    0.25,  0.125,   0.00625},
	{ 0.125,    0.25,   0.5,     1,   4,     1,     0.5,   0.25,   0.125 },
	{ 0.25,      0.5,     1,     4,  16,     4,       1,   0.5,    0.25 },
	{ 0.5,         1,     4,    16,  64,    16,       4,     1,     0.5 },
	{ 1,           4,    16,    64, 256,    64,      16,     4,      1 },
	{ 0.5,         1,     4,    16,  64,    16,       4,     1,    0.5 },
	{ 0.25,      0.5,     1,     4,  16,     4,       1,   0.5,    0.25 },
	{ 0.125,    0.25,   0.5,     1,  4,      1,     0.5,  0.25,   0.125 },
	{ 0.00625, 0.125,  0.25,   0.5,   1,   0.5,     0.25, 0.125, 0.00625 },
};

static const float weights3X3[3][3] =
{
	{ 1, 2, 1 },
	{ 2, 4, 2 },
	{ 1, 2, 1 },
};

[numthreads(8, 8, 1)]
void main(uint2 DTid : SV_DispatchThreadID)
{
	float4 blurredGI = g_GIInput[DTid];
	float4 blurredValues = float4(0, 0, 0, 0);
	float averageSum = 1.0f;
	int kernelOffsetSize = floor(denoiserData.kernelSize / 2.0f);
	int kernelSize = denoiserData.kernelSize;
	//CHECKING FOR EDGES
	for (int i = -kernelOffsetSize; i <= kernelOffsetSize; i++)
	{
		for (int j = -kernelOffsetSize; j <= kernelOffsetSize; j++)
		{
			uint2 indexOffset = uint2(i, j);
			bool isWithinDimensions = GetIsWithinDimesions(DTid + indexOffset, denoiserData.textureDim);
			bool isSameNormal = GetIsEqual(g_NormalInput[DTid].xyz, g_NormalInput[DTid + indexOffset].xyz);
			if (isSameNormal && isWithinDimensions)
			{
				if (kernelSize == 3)
				{
					blurredValues = blurredValues + (weights3X3[i + kernelOffsetSize][j + kernelOffsetSize] * g_GIInput[DTid + indexOffset]);
					averageSum += weights3X3[i + kernelOffsetSize][j + kernelOffsetSize];
				}
				else if (kernelSize == 7)
				{
					blurredValues = blurredValues + (weights7X7[i + kernelOffsetSize][j + kernelOffsetSize] * g_GIInput[DTid + indexOffset]);
					averageSum += weights7X7[i + kernelOffsetSize][j + kernelOffsetSize];
				}
				else if (kernelSize == 9)
				{
					blurredValues = blurredValues + (weights9X9[i + kernelOffsetSize][j + kernelOffsetSize] * g_GIInput[DTid + indexOffset]);
					averageSum += weights9X9[i + kernelOffsetSize][j + kernelOffsetSize];
				}
			}
		}
	}
	blurredValues /= averageSum;
	
	blurredValues.a = 1.0f;

	//blurredValues.a = 1.0f;
	//float4 samples = float4(
 //       g_GIInput.SampleLevel(MirroredLinearSampler, (DTid + offsets[0]) * textureDimensionsBuffer.invTextureDim, 0).r,
 //       g_GIInput.SampleLevel(MirroredLinearSampler, (DTid + offsets[1]) * textureDimensionsBuffer.invTextureDim, 0).g,
 //       g_GIInput.SampleLevel(MirroredLinearSampler, (DTid + offsets[2]) * textureDimensionsBuffer.invTextureDim, 0).b,
 //       g_GIInput[DTid + 1].a);

	//g_output[DTid] = dot(samples, weights);
	g_output[DTid] = blurredValues;
	g_output[DTid].a = 1.0f;
}
