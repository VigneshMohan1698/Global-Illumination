
#include "Utilities.h"

#define PI 3.141
#define RAYMIN  0.001;
#define RAYMAX  10000;
#include "RandomNumberGenerator.hlsli"
#include "DataStructures.hlsli"
#include "MathFunctions.hlsli"

RWTexture2D<float4>             GbufferVertexPosition :   register(u0);
RWTexture2D<float4>             GBufferVertexNormal   :   register(u1);
RWTexture2D<float4>             GBufferVertexAlbedo   :   register(u2);
RWTexture2D<float2>             GBufferMotionVector   :   register(u3);
RWTexture2D<float4>             GBufferDirectLight    :	  register(u4);
RWTexture2D<float>              GBufferDepth		  :	  register(u5);
RWTexture2D<float4>             GBufferOcclusion	  :	  register(u6);
RWTexture2D<float>              GBufferVariance		  :	  register(u7);

RaytracingAccelerationStructure Scene				  :	  register(t0, space0);
StructuredBuffer<uint>          Indices               :   register(t1, space0);
StructuredBuffer<Vertex_PNCUTB>  Vertices			  :    register(t2, space0);
Texture2D                       DiffuseTexture        :   register(t3, space0);
Texture2D                       NormalMapTexture      :   register(t5, space0);
Texture2D                       SkyboxTexture         :   register(t6, space0);
Texture2D                       SkyboxNightTexture    :   register(t7, space0);
Texture2D                       SpecularTexture       :   register(t8, space0);
SamplerState                    SimpleSampler         :   register(s0);
//SamplerState                    NormalSampler         :  register(s1);

ConstantBuffer<SceneConstantBuffer> g_sceneCB              : register(b0);

////--------------------LOCAL ROOT SIGNATURE VALUES-------------------------
//StructuredBuffer<uint> chunkIndices : register(t15, space1);
//StructuredBuffer<Vertex_PNCUTB> chunkVertices : register(t16, space1);

//AppendStructuredBuffer<IrradianceCache> g_irradianceCache : register(u9);

float4 EnvironmentTexture(float3 direction)
{
	uint width = 2048, height = 1536;
	//SkyboxTexture.GetDimensions(width, height);
	float xDimensions = width / 4.0f;
	float yDimensions = height / 3.0f;
	
	float4 color = float4(0, 0, 0, 0);
	float mainAxis;
	float2 uv = float2(0, 0);
	float3 absDirections = float3(abs(direction.x), abs(direction.y), abs(direction.z));
	
	bool xPositive = direction.x > 0 ? 1 : 0;
	bool yPositive = direction.y > 0 ? 1 : 0;
	bool zPositive = direction.z > 0 ? 1 : 0;
	int xindex = 1, yindex = 1;
	
	//------------X------------
	if (xPositive && absDirections.x >= absDirections.y && absDirections.x >= absDirections.z)
	{
		mainAxis = absDirections.x;
		uv.x = -direction.y;
		uv.y = -direction.z;
		xindex = 2;
		yindex = 2;
	}
	
	if (!xPositive && absDirections.x >= absDirections.y && absDirections.x >= absDirections.z)
	{
		mainAxis = absDirections.x;
		uv.x = direction.y;
		uv.y = -direction.z;
		xindex = 4;
		yindex = 2;
	}
	
	//------Y---------
	if (yPositive && absDirections.y >= absDirections.x && absDirections.y >= absDirections.z)
	{
		mainAxis = absDirections.y;
		uv.x = direction.x;
		uv.y = -direction.z;
		xindex = 1;
		yindex = 2;
	}
	
	if (!yPositive && absDirections.y >= absDirections.x && absDirections.y >= absDirections.z)
	{
		mainAxis = absDirections.y;
		uv.x = -direction.x;
		uv.y = -direction.z;
		xindex =3;
		yindex = 2;
	}
	
	//------Z---------
	if (zPositive && absDirections.z >= absDirections.x && absDirections.z >= absDirections.y)
	{
		mainAxis = absDirections.z;
		uv.x = -direction.y;
		uv.y = direction.x;
		xindex = 2;
		yindex = 1;
	}
	else if (!zPositive && absDirections.z >= absDirections.x && absDirections.z >= absDirections.y)
	{
		mainAxis = absDirections.z;
		uv.x = direction.x;
		uv.y = -direction.y;
		xindex = 2;
		yindex = 3;
	}

	//-------------------GETTING UV PER CUBE FACE--------------
	uv.x = 0.5f * (uv.x / mainAxis + 1.0f);
	uv.y = 0.5f * (uv.y / mainAxis + 1.0f);
	
	//-----GETTING UV PER CUBE TEXTURE POINT--------------
	float outputXStart = ((xindex - 1) * xDimensions) / width;
	float outputXEnd = (xindex * xDimensions) / width;
	
	float outputYStart = ((yindex - 1) * yDimensions) / height;
	float outputYEnd = (yindex * yDimensions) / height;
	uv.x = RangeMap(uv.x, 0.0f, 1.0f, outputXStart, outputXEnd);
	uv.y = RangeMap(uv.y, 0.0f, 1.0f, outputYStart, outputYEnd);
	
	
	color = SkyboxTexture.SampleLevel(SimpleSampler, uv, 0.0f);
	return color;
}

float3 GetCosineRandom(float3 hitPosition, float2 DTid)
{
	float2 uv = DTid * hash(hitPosition) ;
	float weight = 0.0f;
	
	float3 outRandom = CosineWeightedSampling(uv, weight);
	return normalize(outRandom * weight);
}

void createCoordinateSystem(float3 N, out float3 Nt, out float3 Nb)
{
	if (abs(N.x) > abs(N.y)) 
		Nt = float3(N.z, 0, -N.x) / sqrt(N.x * N.x + N.z * N.z);
	else
		Nt = float3(0, -N.z, N.y) / sqrt(N.y * N.y + N.z * N.z);
	Nb = cross(N,Nt);
}

// Generate a ray in world space for a camera pixel corresponding to an index from the dispatched 2D grid.
inline void GenerateCameraRay(uint2 currentPixel, uint2 totalPixels, out float3 origin, out float3 direction)
{
	float4 world;
    ////-------------INVERSING THE MATRICES APPROACH---------------------
	float2 pixelCenter = (currentPixel + float2(0.5, 0.5)) / totalPixels;
	float2 ndc = float2(2, -2) * pixelCenter + float2(-1, 1);
	float4 screenPosition = float4(ndc, 0, 1);
	float4 clipPosition = mul(screenPosition, g_sceneCB.inversedProjectionMatrix);
	float4 viewPosition = mul(clipPosition, g_sceneCB.inversedViewMatrix);
	world = viewPosition;

	world.xyz /= world.w;

	origin = g_sceneCB.cameraPosition.xyz;
	direction = normalize(world.xyz - origin);
}
inline void GenerateCameraRayAtOrigin(out float3 direction)
{
	float4 world;

    ////-------------INVERSING THE MATRICES APPROACH---------------------
	float4 screenPosition = float4(0,0,0,1);
	float4 clipPosition = mul(screenPosition, g_sceneCB.inversedProjectionMatrix);
	float4 viewPosition = mul(clipPosition, g_sceneCB.inversedViewMatrixOrigin);
	world = viewPosition;

	world.xyz /= world.w;

	direction = normalize(world.xyz);
}

 //Diffuse lighting calculation.
bool CheckIfISInShadow(float3 hitPosition)
{
	if (g_sceneCB.lightBools.x == 0)
	{
		return false;
	}
	RayPayload shadowPayload;
	shadowPayload.color = float4(0, 0, 0, 0);
	shadowPayload.reflectionIndex = 0;
	shadowPayload.raytype = 2;
	shadowPayload.didHitGeometry = false;
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
}
float4 CalculateDiffuseLighting(float3 hitPosition, float3 normal)
{
	float4 sunlightColor = g_sceneCB.GIColor;
	//float4 sunlightColor = float4(1,1, 1,0);
	float3 pixelToLight = normalize(g_sceneCB.lightPosition.xyz - hitPosition);
    // Diffuse contribution.
	float fNDotL = max(0.0f, dot(pixelToLight, normal));
	return sunlightColor * fNDotL;
}


float4 CalculateSpecular(float3 hitPosition, float3 normal, float2 uv )
{
	float4 sunlightColor = g_sceneCB.GIColor;
	float3 cameraPosition = g_sceneCB.cameraPosition.xyz;
	
	//float4 sunlightColor = float4(1,1, 1,0);
	float3 lightDirection = normalize(g_sceneCB.lightPosition.xyz - hitPosition);
	float3 viewDirection = normalize(cameraPosition - hitPosition);
	
	float3 reflectionDirection = GetReflectedPlane(-lightDirection, normal);
	float3 halfwayRef = normalize(viewDirection + viewDirection);
	
	float4 specularMapColor = float4(SpecularTexture.SampleLevel(SimpleSampler, uv, 0).xyz, 1.0f); // Shininess
	float specularShininess = dot(1.0f, specularMapColor);
	float specularity = pow(saturate(dot(viewDirection, halfwayRef)), specularShininess);
	float3 specularColor = 0.2f * specularity * sunlightColor;
	return float4(specularColor, 0.0f);
}

bool CheckPointLightContribution(float3 hitPosition)
{
	if (g_sceneCB.lightBools.x == 0)
	{
		return false;
	}
	RayPayload pointLightPayload;
	pointLightPayload.color = float4(0, 0, 0, 0);
	pointLightPayload.reflectionIndex = 0;
	pointLightPayload.raytype = 3;
	pointLightPayload.didHitGeometry = false;
	
	RayDesc pointLightRay;
	float3 pixelToLight = normalize(float3(6.8f,0.5f, 44.0f) - hitPosition);
	float tLight = distance(float3(6.8f, 0.5f, 44.0f), hitPosition);
	pointLightRay.Origin = hitPosition + (pixelToLight * 0.1);
	pointLightRay.Direction = (pixelToLight);
	pointLightRay.TMin = RAYMIN;
	pointLightRay.TMax = RAYMAX;

	TraceRay(Scene, 0, ~0, 0, 1, 0, pointLightRay, pointLightPayload);
	return pointLightPayload.didHitGeometry && pointLightPayload.tHit < tLight;
}
bool DidHitSun(float3 hitPosition)
{
	float distanceToSun = distance(hitPosition, g_sceneCB.lightPosition.xyz);
	if (distanceToSun < 3.0f)
	{
		return true;
	}
	return false;
}
bool CanSeeSun(float3 rayDirection, inout float weight)
{
	float3 lightDirection = normalize(g_sceneCB.lightPosition.xyz - g_sceneCB.cameraPosition.xyz);
	float dotWithSun = dot(lightDirection, rayDirection);
	
	if (dotWithSun > 0.995f && dotWithSun < 1.0f)
	{
		weight = RangeMap(dotWithSun, 0.995f, 1.0f, 0.0f, 1.0f);
		weight = 0.1f;
		return true;
	}
	return false;
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
	
	float3 bitangents[3] =
	{
		Vertices[indices32Bytes[0]].m_bitangent,
		Vertices[indices32Bytes[1]].m_bitangent,
		Vertices[indices32Bytes[2]].m_bitangent
	};
	float3 normalMap = float3(0, 0, 0);
	float3 inputFaceNormal = HitAttribute(vertexNormals, attr);
	float3 finalNormal = float3(0, 0, 0);
	data.traingleUV = HitAttribute(vertexUvs, attr);
	data.colors = HitAttributeColor(color, attr);
	float4 waterColor = float4(14, 135, 204, 255) /  255;
	bool isWater = false;
	data.triangleNormal = inputFaceNormal;
	if (data.colors.y != 0.0f && data.colors.w == 0.0f)
	{
		data.diffuseAlbedo = waterColor;
		isWater = true;
	}

	else
	{
		data.diffuseAlbedo = float4(DiffuseTexture.SampleLevel(SimpleSampler, data.traingleUV, 0).xyz, 1.0f);
	}
	data.diffuseAlbedo.a = 0.0f;
	//--------------STORING IS SPECULAR IN ALPHA CHANNEL---------------------
	if (data.colors.w == 0.0f && data.colors.z != 0.0f)
	{
		data.diffuseAlbedo.a = 1.0f;
	}
	if (!g_sceneCB.textureMappings.x || isWater)
	{
		return;
	}
	float3 tangentAttr = HitAttribute(tangents, attr);
	float3 bitangentAttr = HitAttribute(bitangents, attr);
	normalMap = float3(NormalMapTexture.SampleLevel(SimpleSampler, data.traingleUV, 0).xyz);
	normalMap.x = 2 * normalMap.x - 1;
	normalMap.y = 2 * normalMap.y - 1;
	normalMap.z = normalMap.z;
	//normal = HitAttribute(vertexNormals, attr);
	float3 biTangent = float3(0, 0, 0);
	float3 tangent = normalize(tangentAttr - dot(tangentAttr, inputFaceNormal) * inputFaceNormal);
	biTangent = normalize(bitangentAttr - dot(bitangentAttr, inputFaceNormal) * inputFaceNormal);
	//if (dot(inputFaceNormal, tangent) > 0.0f)
	//{
	//	biTangent = cross(inputFaceNormal, tangent);
	//}
	//else
	//{
	//	biTangent = -cross(inputFaceNormal, tangent); //Create the biTangent

	//}
										
	float3x3 texSpace = float3x3(tangentAttr, bitangentAttr, inputFaceNormal); //Create the "Texture Space"
	finalNormal = normalize(mul(normalMap, texSpace)); 
	data.triangleNormal = finalNormal;
	
}

//TEXTURE SPACE
float2 CalculateMotionVector(float3 _hitPosition)
{
    // Calculate screen space position of the hit in the previous frame.
	float4 previousViewPosition = mul(float4(_hitPosition, 1.0f), g_sceneCB._viewMatrix);
	float4 previousClipPosition = mul(previousViewPosition,g_sceneCB.projectionMatrix);
	float2 _texturePosition = ClipSpaceToTextureSpace(previousClipPosition);

	float2 xy = DispatchRaysIndex().xy; // Center in the middle of the pixel.
	float2 texturePosition = xy / DispatchRaysDimensions().xy;

	return texturePosition - _texturePosition;
}
//NDC SPACE
float2 CalculateMotionVectorNDC(float3 hitPosition)
{
	float2 outMotionVector;
	float4 previousViewPosition = mul(float4(hitPosition, 1.0f),g_sceneCB._viewMatrix);
	float4 previousClipPosition = mul(previousViewPosition, g_sceneCB.projectionMatrix); // Projection Matrix remains same for all frames
	float3 previousNDCposition = previousClipPosition.xyz / previousClipPosition.w; // Perspective divide to get Normal Device Coordinates: {[-1,1], [1,1], (0, 1]}
	previousNDCposition.y = -previousNDCposition.y;
	float2 previousTexturePosition = (previousNDCposition.xy + 1) * 0.5f; // Converting to texture space [-1,-1] [1,1] to  [0,1] 
	
	float4 currentViewPosition = mul(float4(hitPosition, 1.0f), g_sceneCB.viewMatrix);
	float4 currentClipPosition = mul(currentViewPosition, g_sceneCB.projectionMatrix);
	float3 currentNDCposition = currentClipPosition.xyz / currentClipPosition.w; // Perspective divide to get Normal Device Coordinates: {[-1,1], [1,1], (0, 1]}
	currentNDCposition.y = -currentNDCposition.y;
	float2 currentTexturePosition = (currentNDCposition.xy + 1) * 0.5f; // Converting to texture space [-1,-1] [1,1] to  [0,1] 

	outMotionVector = previousTexturePosition.xy - currentTexturePosition.xy;
		
	const float epsilon = 1e-5f;
	outMotionVector = (previousClipPosition.w < epsilon) ? float2(0, 0) : outMotionVector;
	//const float epsilon = 1e-5f;
	//outMotionVector = (previousClipPosition.w < epsilon) ? float2(0, 0) : outMotionVector;
	return outMotionVector;
}


float4 CalculateSkyBoxColor(float3 unitVector)
{
	float4 color;
	float u, v;
	u = 0.5 + (atan2(unitVector.y, unitVector.z) / PI);
	v = 0.5 + (asin(unitVector.x) / PI);
	color = float4(SkyboxTexture.SampleLevel(SimpleSampler, float2(u,v), 0).xyz, 1.0f);
	return color;
}

bool RetrieveIrradianceCacheValueIfExists(in float3 hitVertexPosition, out float4 GIValue)
{
	//uint numStructs;
	//uint stride;
	//g_irradianceCache.GetDimensions(numStructs, stride);
	//for (int i = 0; i < numStructs; i++)
	//{
	//	float3 vertexPosition = g_irradianceCache[i].vertexPosition;
	//	if (vertexPosition.x == hitVertexPosition.x && vertexPosition.y == hitVertexPosition.y && vertexPosition.z == hitVertexPosition.z)
	//	{
	//		GIValue = g_irradianceCache[i].GI;
	//		return true;
	//	}

	//}
	return false;
}

void GetWaterRefractedAndReflectedRay(in float3 rayDirection,in float3 impactSurfaceNormal, out float3 refractedRay,out float3  reflectedRay)
{
	float3 N = impactSurfaceNormal;
	float3 I = rayDirection; // CAMERA TO WORLD POINT OR LIGHT TO WORLD POINT?
	float cosTheta = dot(rayDirection, N);
	float cosThetaSquared = cosTheta * cosTheta;
	float refrationCoeff = 1 / 1.3;
	float refrationCoeffSquare = refrationCoeff * refrationCoeff;
	float c1 = sqrt(1 - (refrationCoeffSquare) * (1 - cosThetaSquared));

	refractedRay = I + N * ((cosTheta * refrationCoeff) - c1);
	reflectedRay = GetReflectedPlane(rayDirection, impactSurfaceNormal);
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


[shader("raygeneration")]
void MyRaygenShader()
{
	float3 rayDir;
	float3 origin;

	// Generate a ray for a camera pixel corresponding to an index from the dispatched 2D grid.
	GenerateCameraRay(DispatchRaysIndex().xy, DispatchRaysDimensions().xy, origin, rayDir);

	// Trace the ray.
	// Set the ray's extents.
	RayDesc ray;
	ray.Origin = origin;
	ray.Direction = rayDir;
	//Set TMin to a non-zero small value to avoid aliasing issues due to floating - point errors.
	//TMin should be kept small to prevent missing geometry at close contact areas.
	ray.TMin = RAYMIN;
	ray.TMax = RAYMAX;
	RayPayload payload;
	payload.color = float4(0, 0, 0, 0);
	payload.reflectionIndex = 0.0f;
	payload.raytype = 0;
	payload.startDirection = rayDir;
	payload.tHit = -1.0f;
	
	//--------------------Clear All GBuffers before starting Ray generation

	GbufferVertexPosition[DispatchRaysIndex().xy] = float4(0, 0, 0, 1);
	GBufferVertexNormal[DispatchRaysIndex().xy] = float4(0, 0, 0, 1);
	GBufferVertexAlbedo[DispatchRaysIndex().xy] = float4(0, 0, 0, 1);
	GBufferOcclusion[DispatchRaysIndex().xy] = float4(0, 0, 0, 0);
	GBufferMotionVector[DispatchRaysIndex().xy] = float2(0, 0);
	GBufferDepth[DispatchRaysIndex().xy] = 0.0f;
	GBufferDirectLight[DispatchRaysIndex().xy] = 0.0f;
	GBufferVariance[DispatchRaysIndex().xy] = 0.0f;
	
	//--------------------Main Trace Ray call-------------------------
	TraceRay(Scene, 0, ~0, 0, 1, 0, ray, payload);
	
	float3 currentFrameVertexPosition = GbufferVertexPosition[DispatchRaysIndex().xy].xyz;
	//GBufferMotionVector[DispatchRaysIndex().xy] = CalculateMotionVectorNDC(previousFrameVertexPosition, currentFrameVertexPosition);
	if (payload.tHit != -1.0f)
	{
		GBufferMotionVector[DispatchRaysIndex().xy] = CalculateMotionVectorNDC(currentFrameVertexPosition);
	}
	else
	{
		GBufferMotionVector[DispatchRaysIndex().xy] = float2(0, 0);
	}
}

[shader("closesthit")]
void MyClosestHitShader(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{	
	//--------------SHADOW RAY EARLY OUT----------------------
	if (payload.raytype == 2)
	{
		payload.tHit = RayTCurrent();
		return;
	}
	
	float2 dimensions = DispatchRaysIndex().xy;
	HitData data;
	data.Initialize();
	GetHitData(data, attr);
	
	//--------------WATER RAY EARLY OUT-----------------
	if (payload.raytype == 3)
	{
		if (data.colors.y != 0.0f && data.colors.w == 0.0f)
		{
			//payload.color += data.diffuseAlbedo / 2.0f;
			
			RayPayload waterPayload;
			waterPayload.color = float4(0, 0, 0, 0);
			waterPayload.reflectionIndex = payload.reflectionIndex + 1.0f;
			waterPayload.raytype = 3;
			waterPayload.startDirection = payload.startDirection;
			waterPayload.tHit = -1.0f;
			
			RayDesc waterRefractedRay;
			waterRefractedRay.Origin = data.hitPosition + payload.startDirection * 0.001f;
			waterRefractedRay.Direction = payload.startDirection;
			waterRefractedRay.TMin = RAYMIN;
			waterRefractedRay.TMax = RAYMAX;
			TraceRay(Scene, 0, ~0, 0, 1, 0, waterRefractedRay, waterPayload);
			payload.color += waterPayload.color;
		}
		else
		{
			payload.color = data.diffuseAlbedo ;
		}
		return;
	}
	bool isInShadow = CheckIfISInShadow(data.hitPosition.xyz);
	float4 DirectLight = float4(0, 0, 0, 0);
	if (!isInShadow)
	{
		float4 diffuse = float4(0, 0, 0, 0);
		float4 specular = float4(0, 0, 0, 0);
		//----------------Blinn phong-------------------------
		diffuse = CalculateDiffuseLighting(data.hitPosition, data.triangleNormal);
		//if (g_sceneCB.textureMappings.y) // If Specular maps are enabled
		//{
		//	specular = CalculateSpecular(data.hitPosition, data.triangleNormal, data.traingleUV);
		//}
		DirectLight = diffuse + specular;
	}

	//-----------------------------------WRITING DATA TO G BUFFER-------------------------------------
	if (g_sceneCB.lightBools.w != 0.0f)
	{
		GBufferDirectLight[dimensions] = DirectLight;
	}

	if (data.colors.w != 0.0f)
	{
		GBufferDirectLight[dimensions] = float4(1, 1, 1, 1);
	}
	
	GBufferVertexAlbedo[dimensions] = data.diffuseAlbedo;
	float4 waterColor = data.diffuseAlbedo;
	if (data.colors.y != 0.0f && data.colors.w == 0.0f && (g_sceneCB.textureMappings.z || g_sceneCB.textureMappings.w))
	{
		float3 refractedRay = float3(0, 0, 0);
		float3 reflectedRay = float3(0, 0, 0);
		GetWaterRefractedAndReflectedRay(payload.startDirection, data.triangleNormal, refractedRay, reflectedRay);
		RayPayload waterPayload;
		waterPayload.color = float4(0, 0, 0, 0);
		waterPayload.reflectionIndex = 0.0f;
		waterPayload.raytype = 3;
		waterPayload.startDirection = refractedRay;
		waterPayload.tHit = -1.0f;
		//--------------------Water Trace Ray call-------------------------
		if (g_sceneCB.textureMappings.w)  // Water refractions On
		{
			RayDesc waterRefractedRay;
			waterRefractedRay.Origin = data.hitPosition + refractedRay * 0.001f;
			waterRefractedRay.Direction = refractedRay;
			waterRefractedRay.TMin = RAYMIN;
			waterRefractedRay.TMax = RAYMAX;
			TraceRay(Scene, 0, ~0, 0, 1, 0, waterRefractedRay, waterPayload);
			waterColor += waterPayload.color * 0.3f;
		}
		if (g_sceneCB.textureMappings.z)  // Water reflections On
		{
			RayDesc waterReflectedRay;
			waterReflectedRay.Origin = data.hitPosition + reflectedRay * 0.001f;
			waterReflectedRay.Direction = reflectedRay;
			waterReflectedRay.TMin = RAYMIN;
			waterReflectedRay.TMax = RAYMAX;
			
			waterPayload.color = float4(0, 0, 0, 0);
			waterPayload.reflectionIndex = 0.0f;
			waterPayload.raytype = 3;
			waterPayload.startDirection = reflectedRay;
			waterPayload.tHit = -1.0f;
			
			TraceRay(Scene, 0, ~0, 0, 1, 0, waterReflectedRay, waterPayload);
			waterColor += waterPayload.color * 0.8f;
		}
	}
	GBufferVertexAlbedo[dimensions] = saturate(waterColor);
	GbufferVertexPosition[dimensions] = float4(data.hitPosition, 1.0f);
	GBufferVertexNormal[dimensions] = float4(data.triangleNormal, 1.0f);
	
	float3 cameraDirection;
	GenerateCameraRayAtOrigin(cameraDirection);
	float depth = RayTCurrent() * dot(payload.startDirection, cameraDirection);
	//depth = RayTCurrent();
	GBufferDepth[dimensions] = depth /*/120.0f*/; // Far plane
	payload.tHit = RayTCurrent();
	//if (data.diffuseAlbedo.a != 0.0f)
	//{
	//	ReportHit();
	//}
}

[shader("miss")]
void MyMissShader(inout RayPayload payload)
{
	if(payload.raytype == 3)
	{
		return;
	}
	float2 dimensions = DispatchRaysIndex().xy;
	float4 background = float4(0, 0.3, 0.5, 0.0f);
	background = EnvironmentTexture(payload.startDirection);
	GBufferVertexAlbedo[dimensions] = background;
	GBufferVertexNormal[dimensions] = float4(0,0,0,0);
	GBufferDirectLight[dimensions] = float4(1,1,1,1);
	float weight = 0.0f;
	bool sun = CanSeeSun(payload.startDirection, weight);
	if(sun)
	{
		GBufferOcclusion[dimensions] = float4(1, 1, 1, 1);
		GBufferVertexAlbedo[dimensions] = float4(1, 1, 1, 1);
	}
	
	payload.tHit = -1.0f;
}


[shader("anyhit")]
void MyAnyHitShader(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{
	HitData data;
	data.Initialize();
	GetHitData(data, attr);
	float2 dimensions = DispatchRaysIndex().xy;
	payload.color += float4(1, 1, 1, 1);
	GBufferVertexAlbedo[dimensions] = payload.color;

	GbufferVertexPosition[dimensions] = float4(data.hitPosition, 1.0f);
	GBufferVertexNormal[dimensions] = float4(data.triangleNormal, 1.0f);
}