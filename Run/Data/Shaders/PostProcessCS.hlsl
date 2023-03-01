#include "DataStructures.hlsli"
#include "RandomNumberGenerator.hlsli"
#include "MathFunctions.hlsli"

//------------------SRVS----------------------
Texture2D<float4> g_CompositorData : register(t0);
Texture2D<float> g_DepthBuffer : register(t1);
Texture2D<float4> g_OcclusionTexture : register(t2);
Texture2D<float4> g_NormalBuffer : register(t3);
Texture2D<float4> g_DIBuffer : register(t4);

//-----------------CB----------------------
RWTexture2D<float4> g_output : register(u0);

ConstantBuffer<PostProcessData> processData : register(b0);

SamplerState SimpleSampler : register(s0);

inline bool CanSeeSun(float2 pixelCenter)
{
	//float4 world;
	//float3 origin;
	//float3 direction;
 //   ////-------------INVERSING THE MATRICES APPROACH---------------------
	////pixelCenter = (currentPixel + float2(0.5, 0.5)) / totalPixels;
	//float2 ndc = float2(2, -2) * pixelCenter + float2(-1, 1);
	//float4 screenPosition = float4(pixelCenter, 0, 1);
	//float4 clipPosition = mul(screenPosition, processData.inversedProjectionMatrix);
	//float4 viewPosition = mul(clipPosition, processData.inversedViewMatrix);
	//world = viewPosition;

	//world.xyz /= world.w;

	//origin = processData.cameraPosition.xyz;
	//direction = world.xyz - origin;
	//float3 sunDirection = processData.lightPosition.xyz - origin.xyz;
	//float dotValue = dot(direction, sunDirection);

	//if (dotValue > 0.98f && dotValue <= 1.0f)
	//{
	//	return true;
	//}
	
	//direction = normalize(direction);
	return false;
}


float2 GetLightPositionScreenSpace()
{
	//float4 lightPosition = float4(100,0,70.0f,1.0f);
	float4 lightPosition = processData.lightPosition;
	float4 ViewPosition = mul(lightPosition, processData.viewMatrix);
	float4 ClipPosition = mul(ViewPosition, processData.projectionMatrix);
	float2 texturePosition = ClipSpaceToTextureSpace(ClipPosition);
	return texturePosition;
}

[numthreads(8,8,1)]
void main(uint2 DTid : SV_DispatchThreadID)
{
	float godRaySingle = 0.0f;
	float4 godRayValue = float4(0, 0, 0, 0);
	float decay = 1.0f;
	float exposure = 1.0f;
	float density = 1.0f;
	float weight = 0.01f;	
	int NUM_SAMPLES = 100;
	float2 lightPosition2D = GetLightPositionScreenSpace();
	//lightPosition2D = float2(0.5f,0.5f);
	float2 uv = ((DTid.xy + 0.5f) * processData.invTextureDim);
	float2 lightDeltaCoord = uv - lightPosition2D;
	lightDeltaCoord *= 1.0f / NUM_SAMPLES * density;
	float illuminationDecay = 1.0;
	float2 texCoord = uv.xy;
	for (int i = 0; i < NUM_SAMPLES; i++)
	{
		texCoord -= lightDeltaCoord;
		float4 occlusion = g_OcclusionTexture.SampleLevel(SimpleSampler, texCoord, 1.0f);
		float4 light = g_DIBuffer.SampleLevel(SimpleSampler, texCoord, 1.0f);
		float4 normal = g_NormalBuffer.SampleLevel(SimpleSampler, texCoord, 1.0f);
		//float distanceToLight = distance(float2(texCoord.x, texCoord.y / 2.0f),
		//float2(lightPosition2D.x, lightPosition2D.y / 2.0f));
		float depth = g_DepthBuffer.SampleLevel(SimpleSampler, texCoord, 1.0f);
		//bool sunVisible = CanSeeSun(texCoord);
		//if (!isFloat4Zero(light) && !isFloat4Zero(normal))
		//{
		//	light = float4(1, 1, 1, 1);
		//}
		//else
		//{
		//	light = float4(0, 0, 0, 0);
		//}
		occlusion *= illuminationDecay * weight;
		godRayValue += occlusion;
		illuminationDecay *= decay;
		//--------RAYMARCH-----------------
		//float2 o = uv - lightPosition2D;
		//float2 c = lerp(uv, lightPosition2D, (float(i) + hashUint(uv)) / float(NUM_SAMPLES - 1));
		//float sampledDepth = g_DepthBuffer.SampleLevel(SimpleSampler, c, 1.0f);
		//float depthValue = g_DepthBuffer.SampleLevel(SimpleSampler, uv, 1.0f);
		
		//if (sampledDepth == 0.0f)
		//{
		//	godRaySingle += 1.0f / (NUM_SAMPLES);
		//}

	}
	//godRayValue = godRaySingle * exposure * float4(1, 1, 1, 1);
	godRayValue *= exposure  ;
		
	float alpha = godRayValue.a;
	godRayValue *= processData.GIColor;
	//g_output[DTid] = g_CompositorData[DTid] + g_OcclusionTexture[DTid];
	//g_output[DTid] = g_CompositorData[DTid] + godRayValue ;
	//g_output[DTid] = g_DIBuffer[DTid];
	g_output[DTid] = (1 - alpha) * g_CompositorData[DTid] + godRayValue * alpha;
}	