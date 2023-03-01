static float fSQRT_3_OVER_3 = 0.57735026918962576450914878050196f;
float1 RangeMap(float1 inputValue, float1 inputStart, float1 inputEnd, float1 outputStart, float1 outputEnd)
{
	float1 range = inputEnd - inputStart;
	float1 fraction = (inputValue - inputStart) / range;
	float1 returnValue = (outputStart + ((outputEnd - outputStart) * fraction));
	return returnValue;
}

struct VertexInputData
{
	float3 localPosition : POSITION;
	float4 color : COLOR;
	float2 uv : TEXCOORD;
};
struct VertexToPixelData
{
	float4 v_clipPos	: SV_Position;		
	float4 v_color		: VertexColor;		
	float2 v_uv			: TextureCoords;	
	float4 v_worldPos	: WorldPos;	
	float3 v_rgbaChannels: RgbaChannels;
	float3 v_surfaceNormal : SurfaceNormal;
};
cbuffer CameraConstants : register(b2)
{
	float4x4 ProjectionMatrix;
	float4x4 ViewMatrix;
};
//------------------------------------------------------------------------------------------------
cbuffer ModelConstants : register(b3)
{
	float4x4 ModelMatrix;
	float4 ModelColor;
};
cbuffer MinecraftGameConstants : register(b8)
{
	float4		b_camWorldPos;		
	float4		b_skyColor;			
	float4		b_outdoorLightColor;
	float4		b_indoorLightColor;	
	float		b_fogStartDist;		
	float		b_fogEndDist;		
	float		b_fogMaxAlpha;		
	float		b_time;				
	
};

Texture2D diffuseTexture : register(t0);
SamplerState diffuseSampler : register(s0);


float rand(float3 uv)
{
	return frac(sin(dot(uv, float3(12.9898, 78.233, 58.9321))) * 43758.5453);
}
float Interpolate(float pointA, float pointB, float fraction)
{
	float returnValue = pointA + ((pointB - pointA) * fraction);
	return returnValue;
}

float ComputeCubicBezier1D(float A, float B, float C, float D, float t)
{
	float AB = Interpolate(A, B, t);
	float BC = Interpolate(B, C, t);
	float CD = Interpolate(C, D, t);
	float ABC = Interpolate(AB, BC, t);
	float BCD = Interpolate(BC, CD, t);
	float ABCD = Interpolate(ABC, BCD, t);
	return ABCD;
}
const unsigned int SquirrelNoise5(int positionX, unsigned int seed)
{
	const unsigned int SQ5_BIT_NOISE1 = 0xd2a80a3f; // 11010010101010000000101000111111
	const unsigned int SQ5_BIT_NOISE2 = 0xa884f197; // 10101000100001001111000110010111
	const unsigned int SQ5_BIT_NOISE3 = 0x6C736F4B; // 01101100011100110110111101001011
	const unsigned int SQ5_BIT_NOISE4 = 0xB79F3ABB; // 10110111100111110011101010111011
	const unsigned int SQ5_BIT_NOISE5 = 0x1b56c4f5; // 00011011010101101100010011110101

	unsigned int mangledBits = (unsigned int) positionX;
	mangledBits *= SQ5_BIT_NOISE1;
	mangledBits += seed;
	mangledBits ^= (mangledBits >> 9);
	mangledBits += SQ5_BIT_NOISE2;
	mangledBits ^= (mangledBits >> 11);
	mangledBits *= SQ5_BIT_NOISE3;
	mangledBits ^= (mangledBits >> 13);
	mangledBits += SQ5_BIT_NOISE4;
	mangledBits ^= (mangledBits >> 15);
	mangledBits *= SQ5_BIT_NOISE5;
	mangledBits ^= (mangledBits >> 17);
	return mangledBits;
}

const unsigned int Get3dNoiseUint(int indexX, int indexY, int indexZ, unsigned int seed)
{
	const int PRIME1 = 198491317; // Large prime number with non-boring bits
	const int PRIME2 = 6542989; // Large prime number with distinct and non-boring bits
	return SquirrelNoise5(indexX + (PRIME1 * indexY) + (PRIME2 * indexZ), seed);
}
float SmoothStep3(float t)
{
	float returnvalue = ComputeCubicBezier1D(0, 0, 1, 1, t);
	return returnvalue;
}

float Compute3dPerlinNoise(float posX, float posY, float posZ, float scale, unsigned int numOctaves, float octavePersistence, float octaveScale, bool renormalize, unsigned int seed)
{
	const float OCTAVE_OFFSET = 0.636764989593174f; // Translation/bias to add to each octave

	static const float3 gradients[8] = // Traditional "12 edges" requires modulus and isn't any better.
	{
		float3(+fSQRT_3_OVER_3, +fSQRT_3_OVER_3, +fSQRT_3_OVER_3), // Normalized unit 3D vectors
		float3(-fSQRT_3_OVER_3, +fSQRT_3_OVER_3, +fSQRT_3_OVER_3), //  pointing toward cube
		float3(+fSQRT_3_OVER_3, -fSQRT_3_OVER_3, +fSQRT_3_OVER_3), //  corners, so components
		float3(-fSQRT_3_OVER_3, -fSQRT_3_OVER_3, +fSQRT_3_OVER_3), //  are all sqrt(3)/3, i.e.
		float3(+fSQRT_3_OVER_3, +fSQRT_3_OVER_3, -fSQRT_3_OVER_3), // 0.5773502691896257645091f.
		float3(-fSQRT_3_OVER_3, +fSQRT_3_OVER_3, -fSQRT_3_OVER_3), // These are slightly better
		float3(+fSQRT_3_OVER_3, -fSQRT_3_OVER_3, -fSQRT_3_OVER_3), // than axes (1,0,0) and much
		float3(-fSQRT_3_OVER_3, -fSQRT_3_OVER_3, -fSQRT_3_OVER_3) // faster than edges (1,1,0).
	};

	float totalNoise = 0.f;
	float totalAmplitude = 0.f;
	float currentAmplitude = 1.f;
	float invScale = (1.f / scale);
	float3 currentPos;
	currentPos = float3(posX * invScale, posY * invScale, posZ * invScale);

	for (unsigned int octaveNum = 0; octaveNum < numOctaves; ++octaveNum)
	{
		// Determine random unit "gradient vectors" for surrounding corners
		float3 cellMins;
		cellMins = float3(floor(currentPos.x), floor(currentPos.y), floor(currentPos.z));
		float3 cellMaxs;
		cellMaxs = float3(cellMins.x + 1.f, cellMins.y + 1.f, cellMins.z + 1.f);
		int indexWestX = (int) cellMins.x;
		int indexSouthY = (int) cellMins.y;
		int indexBelowZ = (int) cellMins.z;
		int indexEastX = indexWestX + 1;
		int indexNorthY = indexSouthY + 1;
		int indexAboveZ = indexBelowZ + 1;

		unsigned int noiseBelowSW = Get3dNoiseUint(indexWestX, indexSouthY, indexBelowZ, seed);
		unsigned int noiseBelowSE = Get3dNoiseUint(indexEastX, indexSouthY, indexBelowZ, seed);
		unsigned int noiseBelowNW = Get3dNoiseUint(indexWestX, indexNorthY, indexBelowZ, seed);
		unsigned int noiseBelowNE = Get3dNoiseUint(indexEastX, indexNorthY, indexBelowZ, seed);
		unsigned int noiseAboveSW = Get3dNoiseUint(indexWestX, indexSouthY, indexAboveZ, seed);
		unsigned int noiseAboveSE = Get3dNoiseUint(indexEastX, indexSouthY, indexAboveZ, seed);
		unsigned int noiseAboveNW = Get3dNoiseUint(indexWestX, indexNorthY, indexAboveZ, seed);
		unsigned int noiseAboveNE = Get3dNoiseUint(indexEastX, indexNorthY, indexAboveZ, seed);

		float3 gradientBelowSW = gradients[noiseBelowSW & 0x00000007];
		float3 gradientBelowSE = gradients[noiseBelowSE & 0x00000007];
		float3 gradientBelowNW = gradients[noiseBelowNW & 0x00000007];
		float3 gradientBelowNE = gradients[noiseBelowNE & 0x00000007];
		float3 gradientAboveSW = gradients[noiseAboveSW & 0x00000007];
		float3 gradientAboveSE = gradients[noiseAboveSE & 0x00000007];
		float3 gradientAboveNW = gradients[noiseAboveNW & 0x00000007];
		float3 gradientAboveNE = gradients[noiseAboveNE & 0x00000007];

		// Dot each corner's gradient with displacement from corner to position
		float3 displacementFromBelowSW= float3(currentPos.x - cellMins.x, currentPos.y - cellMins.y, currentPos.z - cellMins.z);
		float3 displacementFromBelowSE= float3(currentPos.x - cellMaxs.x, currentPos.y - cellMins.y, currentPos.z - cellMins.z);
		float3 displacementFromBelowNW= float3(currentPos.x - cellMins.x, currentPos.y - cellMaxs.y, currentPos.z - cellMins.z);
		float3 displacementFromBelowNE= float3(currentPos.x - cellMaxs.x, currentPos.y - cellMaxs.y, currentPos.z - cellMins.z);
		float3 displacementFromAboveSW= float3(currentPos.x - cellMins.x, currentPos.y - cellMins.y, currentPos.z - cellMaxs.z);
		float3 displacementFromAboveSE= float3(currentPos.x - cellMaxs.x, currentPos.y - cellMins.y, currentPos.z - cellMaxs.z);
		float3 displacementFromAboveNW= float3(currentPos.x - cellMins.x, currentPos.y - cellMaxs.y, currentPos.z - cellMaxs.z);
		float3 displacementFromAboveNE= float3(currentPos.x - cellMaxs.x, currentPos.y - cellMaxs.y, currentPos.z - cellMaxs.z);

		float dotBelowSW = dot(gradientBelowSW, displacementFromBelowSW);
		float dotBelowSE = dot(gradientBelowSE, displacementFromBelowSE);
		float dotBelowNW = dot(gradientBelowNW, displacementFromBelowNW);
		float dotBelowNE = dot(gradientBelowNE, displacementFromBelowNE);
		float dotAboveSW = dot(gradientAboveSW, displacementFromAboveSW);
		float dotAboveSE = dot(gradientAboveSE, displacementFromAboveSE);
		float dotAboveNW = dot(gradientAboveNW, displacementFromAboveNW);
		float dotAboveNE = dot(gradientAboveNE, displacementFromAboveNE);

		// Do a smoothed (nonlinear) weighted average of dot results
		float weightEast = SmoothStep3(displacementFromBelowSW.x);
		float weightNorth = SmoothStep3(displacementFromBelowSW.y);
		float weightAbove = SmoothStep3(displacementFromBelowSW.z);
		float weightWest = 1.f - weightEast;
		float weightSouth = 1.f - weightNorth;
		float weightBelow = 1.f - weightAbove;

		// 8-way blend (8 -> 4 -> 2 -> 1)
		float blendBelowSouth = (weightEast * dotBelowSE) + (weightWest * dotBelowSW);
		float blendBelowNorth = (weightEast * dotBelowNE) + (weightWest * dotBelowNW);
		float blendAboveSouth = (weightEast * dotAboveSE) + (weightWest * dotAboveSW);
		float blendAboveNorth = (weightEast * dotAboveNE) + (weightWest * dotAboveNW);
		float blendBelow = (weightSouth * blendBelowSouth) + (weightNorth * blendBelowNorth);
		float blendAbove = (weightSouth * blendAboveSouth) + (weightNorth * blendAboveNorth);
		float blendTotal = (weightBelow * blendBelow) + (weightAbove * blendAbove);
		float noiseThisOctave = blendTotal * (1.f / 0.793856621f); // 3D Perlin is in [-.793856621,.793856621]; map to ~[-1,1]

		// Accumulate results and prepare for next octave (if any)
		totalNoise += noiseThisOctave * currentAmplitude;
		totalAmplitude += currentAmplitude;
		currentAmplitude *= octavePersistence;
		currentPos *= octaveScale;
		currentPos.x += OCTAVE_OFFSET; // Add "irrational" offset to de-align octave grids
		currentPos.y += OCTAVE_OFFSET; // Add "irrational" offset to de-align octave grids
		currentPos.z += OCTAVE_OFFSET; // Add "irrational" offset to de-align octave grids
		++seed; // Eliminates octaves "echoing" each other (since each octave is uniquely seeded)
	}

	// Re-normalize total noise to within [-1,1] and fix octaves pulling us far away from limits
	if (renormalize && totalAmplitude > 0.f)
	{
		totalNoise /= totalAmplitude; // Amplitude exceeds 1.0 if octaves are used
		totalNoise = (totalNoise * 0.5f) + 0.5f; // Map to [0,1]
		totalNoise = SmoothStep3(totalNoise); // Push towards extents (octaves pull us away)
		totalNoise = (totalNoise * 2.0f) - 1.f; // Map back to [-1,1]
	}

	return totalNoise;
}

//------------------------------------------------------------------------------------------------
float3 DiminishingAddComponents( float3 a, float3 b )
{
	   return 1.0f - (1.0f - a) * (1.0f - b);
}
VertexToPixelData VertexMain(VertexInputData vertex)
{
	VertexToPixelData outputToPS;
	float3 newPosition;
	newPosition = vertex.localPosition;
	float waveDuration = b_time * 100.0f % 1.0f;
	if (vertex.color.b == 1.0)
	{
		if (waveDuration > 0.5f)
		{
			waveDuration = RangeMap(waveDuration, 0.5f, 1.0f, -0.2f, 0.2f);
		}
		else
		{
			waveDuration = RangeMap(waveDuration, 0.0f, 0.5f, 0.2f, -0.2f);
		}
		float offset = sin(vertex.localPosition.x + vertex.localPosition.y) * waveDuration;

		newPosition = vertex.localPosition + offset;
	}

	float4 vertexLocalPos = float4(newPosition, 1.0);
	float4 vertexWorldPos = mul(ModelMatrix, vertexLocalPos);
	float4 vertexCameraPos = mul(ViewMatrix, vertexWorldPos);
	float4 vertexClipPos = mul(ProjectionMatrix, vertexCameraPos);
	float3 surfaceNormal = normalize(newPosition - vertex.localPosition);

	outputToPS.v_clipPos = vertexClipPos;
	outputToPS.v_worldPos = vertexWorldPos;
	outputToPS.v_color = vertex.color * ModelColor;
	outputToPS.v_uv = vertex.uv;
	outputToPS.v_rgbaChannels = vertex.color;
	outputToPS.v_surfaceNormal = surfaceNormal;

	return outputToPS;
}

float4 PixelMain( VertexToPixelData input ) : SV_Target0 
{
	float4 waterlight = float4(0.0f, 0.0, 0.0f, 1.0f);
	if (input.v_rgbaChannels.b == 1.0f)
	{
		float3 sunPosition = float3(0.0f, 0.0f, 200.0f);
		float3 worldPosition = float3(input.v_worldPos.x, input.v_worldPos.y, input.v_worldPos.z);
		float3 sunDirection = sunPosition - worldPosition;
		
		float3 surfaceNormal = float3(0.0f, 0.0f, 1.0f);
		float lightOffset = sin(input.v_worldPos.x * 5.0f) + cos(input.v_worldPos.y) + cos(input.v_worldPos.z * 100.0f);
		surfaceNormal += lightOffset;
		surfaceNormal = normalize(surfaceNormal);

		float lightIntensity = saturate(dot(surfaceNormal, normalize(sunDirection)));
		float dotValue = dot(surfaceNormal, sunDirection);
		if (b_skyColor.r > 0.35f)
		{
		/*	lightIntensity = lightIntensity + rand(input.v_worldPos.xyz);*/
			waterlight = float4(lightIntensity, lightIntensity, lightIntensity, 1);
		}
	}

	float2 uvCoords = input.v_uv;
	float4 diffuseTexel = diffuseTexture.Sample(diffuseSampler, uvCoords);
	if (diffuseTexel.a < 0.01)
	{
		discard;
	}

	// Compute lit pixel color
	float outdoorLightExposure = input.v_color.r;
	float indoorLightExposure = input.v_color.g;
	float3 outdoorLight = outdoorLightExposure * b_outdoorLightColor.rgb;
	float3 indoorLight = indoorLightExposure * b_indoorLightColor.rgb;
	float3 diffuseLight = DiminishingAddComponents(outdoorLight, indoorLight);
	float3 diffuseRGB = diffuseLight * diffuseTexel.rgb;
	
	// Compute the fog
	float3 dispCamToPixel = input.v_worldPos.xyz - b_camWorldPos.xyz;
	float distCamToPixel = length(dispCamToPixel);
	float fogDensity = b_fogMaxAlpha * saturate((distCamToPixel - b_fogStartDist) / (b_fogEndDist - b_fogStartDist));
	float3 finalRGB = lerp(diffuseRGB, b_skyColor.rgb, fogDensity);
	float finalAlpha = saturate(diffuseTexel.a + fogDensity);
	float4 finalColor = float4(finalRGB, finalAlpha) + waterlight;
	return finalColor;
}

