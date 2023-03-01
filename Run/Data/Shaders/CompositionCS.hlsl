#include "DataStructures.hlsli"
#include "MathFunctions.hlsli"

Texture2D<float4> g_DI			    : register(t0);
Texture2D<float4> g_GI			    : register(t1);
Texture2D<float4> g_Albedo		    : register(t2);
Texture2D<float4> g_Normal          : register(t3);
Texture2D<float4> g_Position        : register(t4);
Texture2D<float2> g_MotionVector    : register(t5);
Texture2D<float>  g_Depth		    : register(t6);
Texture2D<float4> g_TemporalGBuffer : register(t7);
Texture2D<float4> g_DenoiserOutput  : register(t8);
Texture2D<float2> g_MomentsBuffer  : register(t9);
Texture2D<float> g_HistoryLengthBuffer  : register(t10);
Texture2D<float2> g_PartialDerivates  : register(t11);
Texture2D<float> g_varianceEstimation  : register(t12);
Texture2D<float4> g_IndirectAlbedo  : register(t13);

RWTexture2D<float4> g_finalOutput : register(u0);
RWTexture2D<float4> g_compositorOutput : register(u1);
ConstantBuffer<CompositorData> compositorData: register(b0);
SamplerState SimpleSampler : register(s0);

[numthreads(8, 8, 1)]
void main(uint2 DTid : SV_DispatchThreadID)
{
	if (compositorData.renderOutput == 1)
	{
		float depth = g_Depth[DTid] / 120.0f;
		g_finalOutput[DTid] = float4(depth, depth, depth, 1.0f);

	}
	else if (compositorData.renderOutput == 2)
	{
		g_finalOutput[DTid] = g_Albedo[DTid];
		
		//float4 vertexNormal = abs(g_Normal[DTid]);
		//g_finalOutput[DTid] = vertexNormal;
	}
	else if (compositorData.renderOutput == 3)
	{
		g_finalOutput[DTid] = g_DI[DTid];
	}
	else if (compositorData.renderOutput == 4 )
	{
		if ( compositorData.denoiserOn)
		{
			g_finalOutput[DTid] = g_DenoiserOutput[DTid];
		}
		else
		{
			g_finalOutput[DTid] = g_GI[DTid];
		}	
	}
	else if (compositorData.renderOutput == 5)
	{
		g_finalOutput[DTid] = float4(abs(g_MotionVector[DTid]) * 10.0f, 0.0f, 1.0f);
		
	}
	else if (compositorData.renderOutput == 6)
	{
		float history = g_HistoryLengthBuffer[DTid];
		g_finalOutput[DTid] = float4(history / 32.0f, 0.0f, history / 32.0f, 1);
	}
	else if (compositorData.renderOutput == 7)
	{
		g_finalOutput[DTid] = g_TemporalGBuffer[DTid];
		
		//g_finalOutput[DTid] = float4(abs(g_PartialDerivates[DTid]) / 10.0f, 0.0f, 1.0f);
	}
	else if (compositorData.renderOutput == 8)
	{
		float v = g_varianceEstimation[DTid] * 5.0f;
		g_finalOutput[DTid] = float4(v, v, 0.0f, 1.0f);
		
	}
	else if (compositorData.renderOutput == 9)
	{
		float2 v = abs(g_PartialDerivates[DTid]);
		g_finalOutput[DTid] = float4(v.x, v.y, 0.0f, 1.0f);
	}
	//else if (compositorData.renderOutput == 9)
	//{
	//	float3 vertexPosition = GbufferVertexPosition[DTid].xyz;
	//	float4 irradianceGI = float4(0, 0, 0, 1);
	//	bool doesCacheValueExist = RetrieveIrradianceCacheValueIfExists(vertexPosition, irradianceGI);
	//	g_finalOutput[DTid] = irradianceGI;
	//}
	else if (compositorData.renderOutput == 10)
	{
		g_finalOutput[DTid] = g_Normal[DTid];
		//g_finalOutput[DTid] = float4(g_MomentsBuffer[DTid].x,0.0f, g_MomentsBuffer[DTid].x, 1);
	}
	else if (compositorData.renderOutput == 11)
	{
		g_finalOutput[DTid] = g_IndirectAlbedo[DTid];
	}
	
	
	if (compositorData.renderOutput == 0)
	{
		if (compositorData.denoiserOn)
		{
			//g_finalOutput[DTid] = g_Albedo[DTid] * /*saturate*/((g_DI[DTid] + g_DenoiserOutput[DTid]));
			g_finalOutput[DTid] = (g_Albedo[DTid] + (g_IndirectAlbedo[DTid] * 0.4f)) * (g_DI[DTid] + g_DenoiserOutput[DTid]);

		}
		else
		{
			//g_finalOutput[DTid] = g_Albedo[DTid] * /*saturate*/((g_DI[DTid]) + g_GI[DTid]);
			g_finalOutput[DTid] = (g_Albedo[DTid] + (g_IndirectAlbedo[DTid] * 0.4f)) * (g_DI[DTid] + g_GI[DTid]);
		}

	}
	g_compositorOutput[DTid] = g_finalOutput[DTid];
	
}
