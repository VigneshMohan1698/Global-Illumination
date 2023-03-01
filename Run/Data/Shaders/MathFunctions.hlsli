
// Retrieve hit world position.
float3 HitWorldPosition()
{
	return WorldRayOrigin() + RayTCurrent() * WorldRayDirection();
}


bool GetIsEqual(float3 input1, float3 input2)
{
	if (input1.x == input2.x && input1.y == input2.y && input1.z == input2.z)
	{
		return true;
	}
	return false;
}
bool GetIsWithinDimesions(uint2 Dtid, uint2 textureDimenions)
{
	if (Dtid.x >= 1 && Dtid.x <= textureDimenions.x - 1 && Dtid.y >= 1 && Dtid.y <= textureDimenions.y - 1)
	{
		return true;
	}
	return false;
}

float3 GetReflectedPlane(float3 IncomingRay, float3 SurfaceNormal)
{
	float3 reflectedVector = IncomingRay - 2 * dot(IncomingRay, SurfaceNormal) * SurfaceNormal;
	return reflectedVector;
}

float Interpolate(float pointA, float pointB, float fraction)
{
	float returnValue = pointA + ((pointB - pointA) * fraction);
	return returnValue;
}
float GetFractionWithin(float inputValue, float inputStart, float inputEnd)
{
	float range = inputEnd - inputStart;

	return (inputValue - inputStart) / range;
}
float RangeMap(float inputValue, float inputStart, float inputEnd, float outputStart, float outputEnd)
{
	float fraction = GetFractionWithin(inputValue, inputStart, inputEnd);
	return Interpolate(outputStart, outputEnd, fraction);
}

float4 RangeMapfloat4(float4 inputValue, float4 inputStart, float4 inputEnd, float4 outputStart, float4 outputEnd)
{
	float4 returnColor = { 0, 0, 0, 1 };
	float rfraction, gfraction, bfraction, afraction;
	rfraction = GetFractionWithin(inputValue.r, inputStart.r, inputEnd.r);
	gfraction = GetFractionWithin(inputValue.g, inputStart.g, inputEnd.g);
	bfraction = GetFractionWithin(inputValue.b, inputStart.b, inputEnd.b);
	returnColor.r = Interpolate(outputStart.r, outputEnd.r, rfraction);
	returnColor.g = Interpolate(outputStart.g, outputEnd.g, gfraction);
	returnColor.b = Interpolate(outputStart.b, outputEnd.b, bfraction);

	return returnColor;
}
float Luminance(in float3 color)
{
	return dot(color, float3(0.299, 0.587, 0.114));
}

float2 Chromaticity(in float3 color)
{
	float2 chromaticity = float2(0.0f, 0.0f);
	chromaticity.r = color.r / (color.r + color.g + color.b);
	chromaticity.g = color.g / (color.r + color.g + color.b);
	return chromaticity;
}
// Retrieve attribute at a hit position interpolated from vertex attributes using the hit's barycentrics.
float3 HitAttribute(float3 vertexAttribute[3], BuiltInTriangleIntersectionAttributes attr)
{
	return vertexAttribute[0] +
        attr.barycentrics.x * (vertexAttribute[1] - vertexAttribute[0]) +
        attr.barycentrics.y * (vertexAttribute[2] - vertexAttribute[0]);
}

float2 HitAttribute(float2 vertexAttribute[3], BuiltInTriangleIntersectionAttributes attr)
{
	return vertexAttribute[0] +
        attr.barycentrics.x * (vertexAttribute[1] - vertexAttribute[0]) +
        attr.barycentrics.y * (vertexAttribute[2] - vertexAttribute[0]);
}
float HitAttributeColor(float colorAttribute[3], BuiltInTriangleIntersectionAttributes attr)
{
	return colorAttribute[0] +
        attr.barycentrics.x * (colorAttribute[1] - colorAttribute[0]) +
        attr.barycentrics.y * (colorAttribute[2] - colorAttribute[0]);
}
float4 HitAttributeColor(float4 colorAttribute[3], BuiltInTriangleIntersectionAttributes attr)
{
	return colorAttribute[0] +
        attr.barycentrics.x * (colorAttribute[1] - colorAttribute[0]) +
        attr.barycentrics.y * (colorAttribute[2] - colorAttribute[0]);
}
float3 CosineWeightedSampling(in float2 randomUV, out float weight)
{
	float3 outputDirection;
	float radius = sqrt(1 - randomUV.x * randomUV.x);
	float angle = 2.0f * 3.1417f * randomUV.y;
	outputDirection.x = cos(angle) * radius;
	outputDirection.y = sin(angle) * radius;
	outputDirection.z = sqrt(randomUV.x);
	float distrubtion = outputDirection.z / 3.1417f;
	weight = 1 / (2 * 3.1417f * distrubtion);
	return outputDirection;
}


float2 ClipSpaceToTextureSpace(in float4 clipSpacePosition)
{
	float3 NDCposition = clipSpacePosition.xyz / clipSpacePosition.w; // Perspective divide to get Normal Device Coordinates: {[-1,1], [-1,1], (0, 1]}
	NDCposition.y = -NDCposition.y; // Invert Y for DirectX-style coordinates.
	float2 texturePosition = (NDCposition.xy + 1) * 0.5f; // [-1,1] -> [0, 1]
	return texturePosition;
}
float2 ClipSpaceToScreenSpace(in float4 clipSpacePosition)
{
	float3 NDCposition = clipSpacePosition.xyz / clipSpacePosition.w; // Perspective divide to get Normal Device Coordinates: {[-1,1], [-1,1], (0, 1]}
	return NDCposition.xy;
}


float3 RandomSampling(in float2 uv)
{
	float r = (1 - uv.x * uv.x);
	float phi = 2.0f * 3.1417f * uv.y;
	float3 dir;
	dir.x = cos(phi) * r;
	dir.y = sin(phi) * r;
	dir.z = uv.x;
	return dir;
}


bool isFloat3Zero(float3 value)
{
	return (value.x == 0 && value.y == 0 && value.z == 0);
}

bool isFloat4Zero(float4 value)
{
	return (value.x == 0 && value.y == 0 && value.z == 0 && value.w ==0);
}

float3 GetBitangent(float3 u)
{
	float3 a = abs(u);
	uint xm = ((a.x - a.y) < 0 && (a.x - a.z) < 0) ? 1 : 0;
	uint ym = (a.y - a.z) < 0 ? (1 ^ xm) : 0;
	uint zm = 1 ^ (xm | ym);
	return cross(u, float3(xm, ym, zm));
}

int2 LessThan(int2 check, int2 bounds)
{
	return check.x < bounds.x || check.y < bounds.y;

}

uint GetIndexClosest(in float ref, in float2 value)
{
	float2 changeValue = abs(ref - value);
	uint outIndex = changeValue[1] < changeValue[0] ? 1 : 0;
	return outIndex;
}


float2 GetTextureDimensions(Texture2D textures)
{
	uint width, height;
	textures.GetDimensions(width, height);
	return float2(width, height);
}

bool IsGreaterThanAndCloser(float value,float currentValue, float destinationValue)
{
	if (value > currentValue && value < destinationValue)
	{
		return true;
	}
	if (abs(destinationValue - value) < abs(destinationValue - currentValue))
	{
		
		return true;
	}
	return true;
}