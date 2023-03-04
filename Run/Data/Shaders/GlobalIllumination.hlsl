
#include "Utilities.h"
#define PI 3.141
#define RAYMIN  0.001
#define RAYMAX  10000
#define POINTLIGHTMAXDISTANCE  2
#include "RandomNumberGenerator.hlsli"
#include "DataStructures.hlsli"
#include "MathFunctions.hlsli"

RWTexture2D<float4>             GIOutput					:   register(u0);
RWTexture2D<float4>				GBufferIndirectAlbedo	    : register(u1);

RaytracingAccelerationStructure     Scene			    : register(t0, space0);
StructuredBuffer<uint>              Indices			    : register(t1, space0);
StructuredBuffer<Vertex_PNCUTB>      Vertices			: register(t2, space0);

ConstantBuffer<SceneConstantBuffer>	g_sceneCB			: register(b0);
ConstantBuffer<RaytracedPointLights> g_lightCB			: register(b1);

Texture2D							DiffuseTexture			  : register(t4, space0);
Texture2D							NormalMapTexture		  : register(t5, space0);
Texture2D							SpecularTexture			  : register(t6, space0);
Texture2D<float4>					GbufferVertexPosition	  : register(t7);
Texture2D<float4>					GBufferVertexNormal		  : register(t8);
Texture2D<float4>					GBufferDirectIllumination : register(t9);
Texture2D<float4>					GBufferAlbedo			  : register(t10);
SamplerState						SimpleSampler		      : register(s0);
//SamplerState						NormalSampler		: register(s1);


uint InitializeRandomForHemisphereSampling(uint val0, uint val1, uint backoff = 16)
{
	uint v0 = val0, v1 = val1, s0 = 0;

	//[unroll]
	for (uint n = 0; n < 16; n++)
	{
		s0 += 0x9e3779b9;
		v0 += ((v1 << 4) + 0xa341316c) ^ (v1 + s0) ^ ((v1 >> 5) + 0xc8013ea4);
		v1 += ((v0 << 4) + 0xad90777d) ^ (v0 + s0) ^ ((v0 >> 5) + 0x7e95761e);

	}
	return v0;
}
float3 CosineHemisphereSample(inout uint randSeed, float3 hitNorm)
{
	float2 randVal = float2(nextRand(randSeed), nextRand(randSeed));
	bool isCosineWeighted = g_sceneCB.lightfallOff_AmbientIntensity_CosineSampling_DayNight.z;
	if (isCosineWeighted)
	{
		
		float3 bitangent = GetBitangent(hitNorm);
		float3 tangent = cross(bitangent, hitNorm);
		float r = sqrt(randVal.x);
		float phi = 2.0f * 3.14159265f * randVal.y;

		float3 cosineDirection = tangent * (r * cos(phi).x) + bitangent * (r * sin(phi)) + hitNorm.xyz * sqrt(max(0.0, 1.0f - randVal.x));
		return cosineDirection;

	}
	
	float theta = 2.0f * 3.14159265f * randVal.x;
	float phi = acos(2 * randVal.y - 1);
	float x = sin(phi) * cos(theta);
	float y = sin(phi) * sin(theta);
	float z = cos(phi);
	
	float3 randomDirection = float3(x, y, z);
	if (dot(hitNorm, randomDirection) < 0)
	{
		randomDirection *= -1;
	}
	return randomDirection;
}

float3 SpecularHemisphereSample(inout uint randSeed, float3 hitNorm, float3 hitPosition)
{
	float3 viewDir = normalize(g_sceneCB.cameraPosition.xyz - hitPosition);
	float3 refDir = reflect(-viewDir, hitNorm);
	return refDir;
}

void GetHitData(out HitData data, BuiltInTriangleIntersectionAttributes attr)
{
	float2 dimensions = DispatchRaysIndex().xy;
	data.hitPosition = HitWorldPosition();

	// Get the base index of the triangle's first 16 bit index.
	uint indicesPerTriangle = 3;
	uint startIndex = PrimitiveIndex() * indicesPerTriangle;
	const uint3 indices32Bytes = { Indices[startIndex], Indices[startIndex + 1], Indices[startIndex + 2] };

	float2 vertexUvs[3] =
	{
		Vertices[indices32Bytes[0]].m_uvTexCoords,
		Vertices[indices32Bytes[1]].m_uvTexCoords,
		Vertices[indices32Bytes[2]].m_uvTexCoords
	};

	float3 vertexNormals[3] =
	{
		Vertices[indices32Bytes[0]].m_normal.xyz,
		Vertices[indices32Bytes[1]].m_normal.xyz,
		Vertices[indices32Bytes[2]].m_normal.xyz
	};
	
	float4 color[3] =
	{
		Vertices[indices32Bytes[0]].m_color,
		Vertices[indices32Bytes[1]].m_color,
		Vertices[indices32Bytes[2]].m_color
	};
	
	float3 tangents[3] =
	{
		Vertices[indices32Bytes[0]].m_tangent,
		Vertices[indices32Bytes[1]].m_tangent,
		Vertices[indices32Bytes[2]].m_tangent
	};
	
	float3 normalMap = float3(0, 0, 0);
	float3 inputFaceNormal = HitAttribute(vertexNormals, attr);
	float3 finalNormal = float3(0, 0, 0);
	data.traingleUV = HitAttribute(vertexUvs, attr);
	data.colors = HitAttributeColor(color, attr);
	if (data.colors.w == 0.0f && data.colors.y != 0.0f)
	{
		data.diffuseAlbedo = float4(0.0f, 0.45f, 0.46f, 1.0f);
	}
	else
	{
		data.diffuseAlbedo = float4(DiffuseTexture.SampleLevel(SimpleSampler, data.traingleUV, 0).xyz, 1.0f);
	}
	float3 tangentAttr = HitAttribute(tangents, attr);
	data.triangleNormal = inputFaceNormal;

	if (!g_sceneCB.textureMappings.x)
	{
		return;
	}
	
	normalMap = float3(NormalMapTexture.SampleLevel(SimpleSampler, data.traingleUV, 0).xyz);
	normalMap.x = 2 * normalMap.x - 1;
	normalMap.y = 2 * normalMap.y - 1;
	normalMap.z = normalMap.z;
	//normal = HitAttribute(vertexNormals, attr);
	float3 biTangent = float3(0, 0, 0);
	float3 tangent = normalize(tangentAttr - dot(tangentAttr, inputFaceNormal) * inputFaceNormal);
	if (dot(inputFaceNormal, tangent) > 0.0f)
	{
		biTangent = cross(inputFaceNormal, tangent);
	}
	else
	{
		biTangent = -cross(inputFaceNormal, tangent); //Create the biTangent

	}							
	float3x3 texSpace = float3x3(tangent, biTangent, inputFaceNormal); //Create the "Texture Space"
	finalNormal = normalize(mul(normalMap, texSpace)); //Convert normal from normal map to texture space and store in input.normal
	data.triangleNormal = finalNormal;

	
}
void AppendIrradianceValueToCache(in float3 hitVertexPosition,in float3 vertexNormal, in float4 GIValue)
{
	//uint numStructs;
	//uint stride;
	//g_irradianceCache.GetDimensions(numStructs, stride);
	//IrradianceCache newCacheValue;
	//newCacheValue.Initialize();
	//newCacheValue.vertexPosition = hitVertexPosition;
	//newCacheValue.vertexNormal = vertexNormal;
	//newCacheValue.GI = GIValue;
	//int counter = g_irradianceCache.IncrementCounter() - 1;
	//g_irradianceCache[counter] = newCacheValue;
}

bool CheckIfISInShadow(float3 hitPosition)
{
	if (g_sceneCB.lightBools.x == 0)
	{
		return false;
	}
	GIRayPayload shadowPayload;
	shadowPayload.GlobalIllumination = float4(0, 0, 0, 0);
	shadowPayload.ReflectionIndex = 0.0f;
	shadowPayload.Raytype = 2.0f;
	shadowPayload.tHit = -1.0f;
	
	RayDesc shadowRay;
	float3 pixelToLight = normalize(g_sceneCB.lightPosition.xyz - hitPosition);
	float tLight = distance(g_sceneCB.lightPosition.xyz, hitPosition);
	shadowRay.Origin = hitPosition + (pixelToLight * 0.001);
	shadowRay.Direction = (pixelToLight);
	shadowRay.TMin = RAYMIN;
	shadowRay.TMax = RAYMAX;

	TraceRay(Scene, RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH, ~0, 0, 1, 0, shadowRay, shadowPayload);
	return shadowPayload.tHit != -1.0f && shadowPayload.tHit < tLight;
	//return false;
}

float4 CalculateDiffuseLighting(float3 hitPosition, float3 normal)
{
	float4 sunlightColor = g_sceneCB.GIColor;
	//float4 sunlightColor = float4(1,1, 1,0);
	float3 pixelToLight = normalize(g_sceneCB.lightPosition.xyz - hitPosition);
	
    // Diffuse contribution.
	float fNDotL = max(0.0f, dot(pixelToLight, normal));
	float4 color = saturate(sunlightColor * fNDotL);
	color.a = 0.0f;
	return color;
}

float4 CalculateSpecularLighting(float3 cameraPosition, float3 hitPosition, float3 normal)
{
	float4 sunlightColor = g_sceneCB.GIColor;
	float3 pixelToLight = normalize(g_sceneCB.lightPosition.xyz - hitPosition);
	
	float3 viewDir = normalize(cameraPosition - hitPosition);
	//float3 refDir = reflect(-pixelToLight, normal);
	float3 halfDir = normalize(pixelToLight + viewDir);
	
	float dotValue = dot(viewDir, halfDir);
	
	float specularCoeff = pow(saturate(dotValue), 64);
	float3 specularLight =  specularCoeff * sunlightColor.rgb;
	
	return float4(specularLight.xyz, 1.0f);
}

float3 DecideClosestPlane(float3 hitPosition, float3 glowstoneCenter)
{
	float3 glowstonePlanes[6];
	glowstonePlanes[0] = glowstoneCenter + float3(0.5f, 0.0f, 0.0f);
	glowstonePlanes[1] = glowstoneCenter + float3(-0.5f, 0.0f, 0.0f);
	glowstonePlanes[2] = glowstoneCenter + float3(0.0f, 0.5f, 0.0f);
	glowstonePlanes[3] = glowstoneCenter + float3(0.0f, -0.5f, 0.0f);
	glowstonePlanes[4] = glowstoneCenter + float3(0.0f, 0.0f, 0.5f);
	glowstonePlanes[5] = glowstoneCenter + float3(0.0f, 0.0f, -0.5f);
	float closestDistance = POINTLIGHTMAXDISTANCE;
	int closestPlaneIndex = 0;
	for (int i = 0; i < 6; i++)
	{
		if (distance(glowstonePlanes[i], hitPosition) < closestDistance)
		{
			closestPlaneIndex = i;
		}
	}
	return glowstonePlanes[closestPlaneIndex];
}

void PointLightContribution(float3 hitPosition, float3 normal, uint rand, inout float4 globalIllumination)
{
	int totalLights = g_lightCB.Counter + 1;
	const uint lightsToCheckWith = 8;
	
	//----------------CHOOSE BEST SET OF POINT LIGHTS------------
	int lightsIndices = 0;
	int lightArray[lightsToCheckWith];

	//int frameNumber = g_sceneCB.samplingData.w;
	//uint randSeed = SeedThread(asuint(frameNumber));

	//for (int j = 0; j < lightsToCheckWith; j++)
	//{
	//	int randIndex = RandomUpperLower(randSeed, 0, totalLights);
	//	lightArray[j] = randIndex;
	//}
	
	//float totalWeight = 0.00001f;
	//float lightWeights[lightsToCheckWith];
	//for (int k = 0; k < lightsToCheckWith; k++)
	//{
	//	float3 randomLight = g_lightCB.PointLightPosition[lightArray[k]].xyz;
	//	float dotProductValue = abs(dot(hitPosition, randomLight));
	//	float disToLight = max(0.0001f, distance(hitPosition, randomLight));
	//	float lightWeight = dotProductValue / disToLight;
	//	lightWeights[k] = lightWeight;
	//	totalWeight += lightWeight;
	//}
	//float random01 = Random01(rand);
	//float currentValue = 0.0f;
	//int FinalLightIndex = -1;
	//for (int l = 0; l < lightsToCheckWith; l++)
	//{
	//	float weight = lightWeights[l] / totalWeight;
	//	if (IsGreaterThanAndCloser(weight, currentValue, random01))
	//	{
	//		FinalLightIndex = lightArray[l];
	//	}

	//}
	//if (FinalLightIndex == -1) // No good Lights found. So Return;
	//{
	//	return;
	//}
	
	for (int i = 0; i < totalLights; i++)
	{
		float3 checkingLight = g_lightCB.PointLightPosition[i].xyz;
		float dist = length(checkingLight - hitPosition);
		if (dist < POINTLIGHTMAXDISTANCE && lightsIndices < lightsToCheckWith)
		{
			lightArray[lightsIndices] = i;
			lightsIndices = lightsIndices + 1;
		}
	}
	if (lightsIndices == 0) // No good Lights found. So Return;
	{
		return;
	}

	//int pointLightToSample = min(nextRand(rand), lightsIndices-1);
	int rndSeed = SeedThread(asuint(g_sceneCB.samplingData.w));
	int pointLightToSample = RandomUpperLower(rndSeed, 0, lightsIndices - 1);
	//int pointLightIndexInMainArray = lightArray[FinalLightIndex];
	//float3 SamplingPointLightPosition = g_lightCB.PointLightPosition[FinalLightIndex].xyz;
	int pointLightIndexInMainArray = lightArray[pointLightToSample];
	float3 SamplingPointLightPosition = g_lightCB.PointLightPosition[pointLightIndexInMainArray].xyz;
	float distToLight = length(SamplingPointLightPosition - hitPosition);
	float3 lightDirection = normalize(SamplingPointLightPosition - hitPosition);
			
	RayDesc pointLightRay;
	pointLightRay.Origin = hitPosition + (lightDirection * 0.01f);
	pointLightRay.Direction = lightDirection;
	pointLightRay.TMin = RAYMIN;
	pointLightRay.TMax = distToLight;
			
	GIRayPayload pointLightPayload;
	pointLightPayload.Raytype = 5.0f;
	pointLightPayload.GlobalIllumination = float4(0, 0, 0, 0);
	pointLightPayload.ReflectionIndex = 0.0f;
	TraceRay(Scene, RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH, ~0, 0, 1, 0, pointLightRay, pointLightPayload);
			
	//float fnPointLightDotL = 0.0f;
	//fnPointLightDotL = max(0.01f, dot(lightDirection, data.triangleNormal));
	float fnDotL = dot(lightDirection, normal);
	if (pointLightPayload.DidHitEmissiveSurface)
	{
		globalIllumination += (pointLightPayload.GlobalIllumination * fnDotL / distToLight)/* * g_sceneCB.samplingData.z*/;
	}
}


[shader("raygeneration")]
void GIRayGenShader()
{
	float4 globalIllumination = float4(0, 0, 0, 0);
	float4 indirectAlbedo = float4(0, 0, 0, 0);
	float2 dispatchRayIndex = DispatchRaysIndex().xy;
	float3 rayDir;
	float3 hitPosition, hitNormal;
	hitPosition = GbufferVertexPosition[dispatchRayIndex].xyz;
	hitNormal = GBufferVertexNormal[dispatchRayIndex].xyz;
	uint2 texDimensions;
	GIOutput.GetDimensions(texDimensions.x, texDimensions.y);
	//--------------------Clear All GBuffers before starting Ray generation---------------------
	GIOutput[dispatchRayIndex] = float4(0, 0, 0, 0);
	uint rand = InitializeRandomForHemisphereSampling(dispatchRayIndex.x + dispatchRayIndex.y * texDimensions.x, g_sceneCB.samplingData.w, 16);
	//---------------------POINT LIGHT ILLUMINATION--------------------
	PointLightContribution(hitPosition, hitNormal, rand, globalIllumination);
	//bool isInShadow = CheckIfISInShadow(hitPosition);
	//if (!isInShadow)
	//{
	//	globalIllumination += CalculateDiffuseLighting(hitPosition, hitNormal);

	//}
	
	//----------------------GLOBAL ILLUMINATION----------------------
	bool didHitSkyOrEmissiveSurface = false;
	bool isSurfaceSpecular = GBufferAlbedo[dispatchRayIndex].a != 0.0f;
	if (!isFloat3Zero(hitNormal) && g_sceneCB.lightBools.y)
	{
		for (uint sampleIndex = 0; sampleIndex < g_sceneCB.samplingData.y; sampleIndex++)
		{
			float3 randomRaySamplingDirection = float3(0,0,0);
			//randomRaySamplingDirection = GetRandomRayDirection(dispatchRayIndex, hitPosition, hitNormal, texDimensions, g_sceneCB.samplingData.w);
			if (isSurfaceSpecular)
			{
				randomRaySamplingDirection = SpecularHemisphereSample(rand, hitNormal, hitPosition);
			}
			else
			{
				randomRaySamplingDirection = CosineHemisphereSample(rand, hitNormal);
			}
			float fNDotL = max(0.01f, dot(randomRaySamplingDirection, hitNormal));
				
			
			RayDesc ray;
			ray.Origin = hitPosition + (randomRaySamplingDirection * 0.001f);
			ray.Direction = randomRaySamplingDirection;
			ray.TMin = RAYMIN;
			ray.TMax = RAYMAX;
			
			GIRayPayload payloadIndirect;
			payloadIndirect.Raytype = 1.0f;
			payloadIndirect.GlobalIllumination = float4(0, 0, 0, 0);
			payloadIndirect.ReflectionIndex = 0.0f;

			RAY_FLAG flags = RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH;
			TraceRay(Scene, flags, ~0, 0, 1, 0, ray, payloadIndirect);
			globalIllumination += payloadIndirect.GlobalIllumination / (fNDotL / PI);
			indirectAlbedo += payloadIndirect.IndirectAlbedo;
		}
	}
	else
	{
		globalIllumination = float4(0, 0, 0, 0);
	}
	
	if (isSurfaceSpecular)
	{
		GBufferIndirectAlbedo[dispatchRayIndex] = indirectAlbedo;
	}
	else
	{
		GBufferIndirectAlbedo[dispatchRayIndex] = float4(0,0,0,0);
	}
	globalIllumination /= (g_sceneCB.samplingData.y * g_sceneCB.samplingData.y);
	GIOutput[dispatchRayIndex] = globalIllumination ;
	
}

[shader("closesthit")]
void GIClosestHitShader(inout GIRayPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{
	//--------------SHADOW RAY EARLY OUT----------------------
	if (payload.Raytype == 2)
	{
		payload.tHit = RayTCurrent();
		return;
	}
	
	float4 globalIllumination = float4(0, 0, 0, 0);
	float4 indirectAlbedo = float4(0, 0, 0, 0);
	float2 dispatchRayIndex = DispatchRaysIndex().xy;
	uint2 texDimensions;
	GIOutput.GetDimensions(texDimensions.x, texDimensions.y);
	uint rand = InitializeRandomForHemisphereSampling(dispatchRayIndex.x + dispatchRayIndex.y * texDimensions.x, g_sceneCB.samplingData.w, 16);
	
	HitData data;
	data.Initialize();
	GetHitData(data, attr);
	indirectAlbedo += data.diffuseAlbedo;
	//------------EARLY OUT FOR EMISSIVE SURFACE----------------
	if (payload.Raytype == 5)
	{
		if (data.colors.w != 0.0f && data.colors.y != 0.0f)
		{
			payload.DidHitEmissiveSurface = true;
			payload.GlobalIllumination = float4(data.colors.rgb, 1);
			payload.IndirectAlbedo = data.colors;
			return;
		}
		else
		{
			payload.DidHitEmissiveSurface = false;
			return;
		}
	}

	if (data.colors.w != 0.0f)
	{
		payload.DidHitEmissiveSurface = true;
		payload.GlobalIllumination = float4(data.colors.rgb, 1);
		payload.IndirectAlbedo = data.colors;
		payload.tHit = RayTCurrent();
		return;
	}

	//--------------------THE ILLUMINATION FROM SUN AT THIS POINT---------------------
	bool isHitInShadow = CheckIfISInShadow(data.hitPosition);
	float4 specularLight = float4(0, 0, 0, 0);
	float4 diffuseLight = float4(0, 0, 0, 0);
	
	diffuseLight = CalculateDiffuseLighting(data.hitPosition, data.triangleNormal);
	if (g_sceneCB.textureMappings.y && data.colors.z != 0.0f && data.colors.w == 0.0f)
	{
		specularLight = CalculateSpecularLighting(g_sceneCB.cameraPosition.xyz, data.hitPosition, data.triangleNormal);	
	}
	
	//--------------COLOR BLEEDING-----------------
	if (g_sceneCB.samplingData.z && payload.ReflectionIndex == 0.0f)
	{
		diffuseLight *= data.diffuseAlbedo;
		//specularLight *= data.diffuseAlbedo;
	}
	
	globalIllumination += specularLight + diffuseLight;
	globalIllumination *= !isHitInShadow;
	
	int totalLights = g_lightCB.Counter;

	if (payload.ReflectionIndex < g_sceneCB.samplingData.x )
	{
		PointLightContribution(data.hitPosition, data.triangleNormal, rand, globalIllumination);
		float3 randomRaySamplingDirection;
		//randomRaySamplingDirection = GetRandomRayDirection(dispatchRayIndex, data.hitPosition, data.triangleNormal, texDimensions, payload.ReflectionIndex);
		//randomRaySamplingDirection = GetReflectedPlane(-WorldRayDirection(), data.triangleNormal);
		bool isSurfaceSpecular = (data.colors.w == 0.0f && data.colors.z != 0.0f);
		if (isSurfaceSpecular)
		{
			randomRaySamplingDirection = SpecularHemisphereSample(rand, data.triangleNormal, data.hitPosition);
		}
		else
		{
			randomRaySamplingDirection = CosineHemisphereSample(rand, data.triangleNormal);
		}
		//randomRaySamplingDirection = CosineHemisphereSample(rand, data.triangleNormal);
		float fNDotL = max(0.01f, dot(randomRaySamplingDirection, data.triangleNormal));
		RayDesc ray;
		ray.Origin = data.hitPosition + (randomRaySamplingDirection * 0.001f);
		ray.Direction = randomRaySamplingDirection;
		ray.TMin = RAYMIN;
		ray.TMax = RAYMAX;

		GIRayPayload payloadIndirect;
		payloadIndirect.Raytype = 1.0f;
		payloadIndirect.GlobalIllumination = float4(0, 0, 0, 0);
		payloadIndirect.ReflectionIndex = payload.ReflectionIndex + 1;
		TraceRay(Scene, 0, ~0, 0, 1, 0, ray, payloadIndirect);

		float reflectionFalloff = pow((1 - g_sceneCB.lightfallOff_AmbientIntensity_CosineSampling_DayNight.x), payload.ReflectionIndex);
		globalIllumination += (payloadIndirect.GlobalIllumination /*/ (fNDotL / PI)*/) * fNDotL * reflectionFalloff;

	}

	//CAMERA HITS THE FIRST PIXEL AND APPENDS THE GI VALUE TO CACHE Note : Not doing it outside of this if check coz we don't want 
	//AppendIrradianceValueToCache(payload.lastHitPosition, payload.lastHitNormal, globalIllumination);
	payload.GlobalIllumination = globalIllumination;
	payload.IndirectAlbedo = indirectAlbedo;
	payload.DidHitEmissiveSurface = false;
	payload.tHit = RayTCurrent();
	//AcceptHitAndEndSearch();
}

[shader("miss")]
void GIMissShader(inout GIRayPayload payload)
{
	if (payload.Raytype == 5)
	{
		return;
	}
	float4 background = g_sceneCB.GIColor * g_sceneCB.lightfallOff_AmbientIntensity_CosineSampling_DayNight.y;
	background.a = 0.0f;
	payload.tHit = -1.0f;
	//payload.GlobalIllumination = g_sceneCB.GIColor * 0.5f;
	if(g_sceneCB.lightBools.z)														// Is Sky light on
	{
		payload.GlobalIllumination = background;
	}
	payload.DidHitEmissiveSurface = false;
	payload.IndirectAlbedo = background;
}
