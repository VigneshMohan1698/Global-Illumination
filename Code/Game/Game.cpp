#include "Game.hpp"
#include "stdlib.h"
#include "Game/GameCommon.hpp"
#include "Game/App.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Game/App.hpp"
#include <Engine/Math/VertexUtils.hpp>
#include "Engine/Input/InputSystem.hpp"
#include "Game/BlockTemplateDefinition.hpp"
#include <vector>
#include <Engine/Renderer/SimpleTriangleFont.hpp>
#include "Engine/Renderer/RendererD12.cpp"
#include "Engine/Core/SpriteAnimDefinition.hpp"
#include <random>
extern App* g_theApp;
//TIME SPENT IN THESIS - AS of 09-09-2022 - 42 hours -- Not including summer time (Close to 25 hours on the pipeline)

extern App* g_theApp;
extern RendererD12* g_theRenderer;
extern InputSystem* g_theInputSystem;
extern AudioSystem* g_theAudioSystem; 
extern Window* g_theWindow;

RandomNumberGenerator rng;

Game::Game()
{
}

void Game::Startup()
{
	InitializeCameras();
	LoadGameSounds();
	m_IsAttractMode = true;
	m_currentGameState = GameStates::GameWorld;
	m_player = new Player(this, Vec3(-0.0,0.0f,65.0f), EulerAngles(0.1f, 0.1f, 0.1f));
	BlockDefinition::InitializeDefinitions();
	BlockTemplateDefinition::InitializeDefinitions();
	InitializeRaytracing();
	InitializeImGui();
	m_world = new World(this);
	g_theRenderer->m_lightPosition = Vec4(100.0f,0.0f,70.0f,1.0f);
	//CreateSun();
	SwitchToGame(0.0f);
}
void Game::Update(float deltaSeconds)
{
	ChangeMouseSettings();
	switch (m_currentGameState)
	{
	case GameStates::EngineLogo:		 UpdateEngineLogo(deltaSeconds);		break;
	case GameStates::Attract:			 UpdateAttractMode(deltaSeconds);		break;
	case GameStates::GameWorld:			 UpdateGame(deltaSeconds);				break;
	default:																	break;
	}
}
void Game::Render()
{
	Rgba8 clearScreen = Rgba8(255, 255, 255, 255);
	//g_theRenderer->ClearScreen(clearScreen);
	switch (m_currentGameState)
	{
	case GameStates::EngineLogo:		 RenderEngineLogo();							break;
	case GameStates::Attract:			 RenderAttractMode();							break;
	case GameStates::GameWorld:			 RenderGame();									break;
	default:																			break;
	}
}
void Game::ShutDown()
{
	m_imGUI.ShutdownImGui();
}
void Game::InitializeRaytracing()
{
	g_theRenderer->CreateRaytracingInterfaces();
	g_theRenderer->CreateRootSignatures();
	g_theRenderer->CreateRaytracingPipelineStateObject();
//	g_theRenderer->CreateOrGetTextureFromFile("DiffuseTexture", "Data/Images/BasicSprites_64x64.jpg");
	g_theRenderer->LoadTexture("MinecraftTextureWICT", "Data/Images/BasicSprites_64x64.png", TextureType::WICT);
	g_theRenderer->LoadTexture("MinecraftTextureWICTNormal", "Data/Images/BasicSprites_64x64NormalMap2.png", TextureType::WICT);
	g_theRenderer->LoadTexture("MinecraftSkyboxWICT", "Data/Images/Skybox.png", TextureType::WICT);
	g_theRenderer->LoadTexture("MinecraftWICTSpecular", "Data/Images/SpecularMapMinecraft.png", TextureType::WICT);
	//g_theRenderer->InitializeSampler();
	g_theRenderer->InitializeGlobalIllumination();
	//g_theRenderer->InitializeIrradianceCaching();
	g_theRenderer->BuildShaderTables();
	g_theRenderer->CreateRaytracingOutputResources();
	g_theRenderer->InitializeDenoising();
	g_theRenderer->InitializeComposition();
	g_theRenderer->InitializePostProcess();
	ImportBunny();
}
void Game::InitializeImGui()
{
	m_imGUI.InitializeImGui();
	m_imGUI.m_thesisVariables.m_samplingData = Vec4(1.0f, 1.0f,1.0f,0.0f);
	m_imGUI.m_thesisVariables.m_color = Vec4(1.0f, 1.0f, 1.0f, 1.0f);
	m_imGUI.m_thesisVariables.m_lightBools = Vec4(1.0f, 1.0f, 1.0f, 1.0f);
	m_imGUI.m_thesisVariables.m_renderOutput = 0;
	m_imGUI.m_thesisVariables.m_temporalSamplerOn = 1;
	m_imGUI.m_thesisVariables.m_denoiserOn = 1;
	m_imGUI.m_thesisVariables.m_denoiserType = 1;
	m_imGUI.m_thesisVariables.m_sunRadius = 2.0f;
	m_imGUI.m_thesisVariables.m_currentScene = (int)g_theRenderer->m_currentScene;
	m_imGUI.m_thesisVariables.m_atrousStepSize = 4;
	m_imGUI.m_thesisVariables.m_varianceFiltering = true;
	m_imGUI.m_thesisVariables.m_godRaysOn = true;
	m_imGUI.m_thesisVariables.m_temporalFade = 0.1f;
	m_imGUI.m_thesisVariables.m_lightFallof = 0.01f;
	m_imGUI.m_thesisVariables.m_textureMappings = Vec4(0.0f,0.0f,1.0f,0.0f);
	m_imGUI.m_thesisVariables.m_ambientIntensity = 0.1f;
}

void Game::UpdateGame(float deltaSeconds)
{
	SwitchGBuffers();
	UpdateSceneConstants();
	Visualizations();
	m_worldCamera.SetPerspectiveView(g_theWindow->GetConfig().m_clientAspect, 60.0f, 1.0f, 10000.0f);
	g_theRenderer->BeginCamera(m_worldCamera);
	GetInputAndMovePlayer(deltaSeconds);
	//if (m_minimumChunksForASBuild > m_world->m_activeChunks.size())
	//{
	//	m_world->Update(deltaSeconds);
	//}

	m_world->Update(deltaSeconds);
	UpdateImGuiInfo(deltaSeconds);
	UpdateGameBar(deltaSeconds);
	BuildAccelerationStructures();

	if (m_frameNumber > 500000)
	{
		m_frameNumber = 0;
	}
	m_frameNumber++;
}
void Game::RenderGame() 
{
	//m_world->Render();
	bool isTemporalSamplerOn = m_imGUI.m_thesisVariables.m_temporalSamplerOn;
	bool isDenoiserOn = m_imGUI.m_thesisVariables.m_denoiserOn;
	bool isGodRaysOn = m_imGUI.m_thesisVariables.m_godRaysOn;
	if ( m_canRender)
	{
		g_theRenderer->Prepare();
		//g_theRenderer->CreateOrGetTextureFromFile("DiffuseTexture", "Data/Images/BasicSprites_64x64.png");
	/*	g_theRenderer->CreateOrGetTextureFromFile("NormalTexture", "Data/Images/BasicSprites_64x64NormalMap2.png");
		g_theRenderer->CreateOrGetTextureFromFile("Skybox", "Data/Images/BlueSkybox.png");
		g_theRenderer->CreateOrGetTextureFromFile("SpecularMap", "Data/Images/SpecularMapMinecraft.png");*/
		g_theRenderer->RunRaytracer();
		g_theRenderer->RunGI();
		g_theRenderer->RunDenoiser(isTemporalSamplerOn, isDenoiserOn);
		g_theRenderer->RunCompositor(isDenoiserOn, isGodRaysOn);
		g_theRenderer->CopyRaytracingOutputToBackbuffer();
		m_imGUI.RenderImGui();
		g_theRenderer->FinishRaytraceCopyToBackBuffer();
		g_theRenderer->Present();
	}

	g_theRenderer->MoveToNextFrame();

}

bool Game::ImportBunny()
{
	std::string modelName = "Bunny";
	std::string  modelPath = "Data/Models/" + modelName + '/' + modelName + ".obj";
	const char* modelPathString = modelPath.c_str();
	if (FileExists(modelPath))
	{
		g_theRenderer->CreateOrGetMesh(modelPathString);
	}

	MeshBuilder* mesh = g_theRenderer->GetMesh(modelPath.c_str());
	MeshImportOptions& options = mesh->m_importOptions;
	std::string text = "j,k,i";
	Mat44 transformMatrix = Mat44(text);
	options.m_transform = transformMatrix;
	mesh->m_importOptions = options;
	mesh->ApplyMeshOptions();

	std::vector<Vertex_PNCUTB>& bunnyVerts = mesh->m_cpuMesh->m_verticesWithTangent;
	std::vector<unsigned int>& indexes = mesh->m_cpuMesh->m_indices;

	//std::vector<Vertex_PNCUT> bunnyVerts ;
	//std::vector<unsigned int> indexes;

	Vec3 bottomLeft, bottomRight, topLeft, topRight;
	Vec3 quadNormal = Vec3(0,0,1);
	AABB3 blockbounds = AABB3(Vec3(-10,-10,-0.0), Vec3(10,10,10));
	float minx, miny, minz, maxx, maxy, maxz;
	minx = blockbounds.m_mins.x;
	miny = blockbounds.m_mins.y;
	minz = blockbounds.m_mins.z;

	maxx = blockbounds.m_maxs.x;
	maxy = blockbounds.m_maxs.y;
	maxz = blockbounds.m_maxs.z;

	bottomLeft = Vec3(maxx, maxy, minz);
	bottomRight = Vec3(maxx, miny, minz);
	topLeft = Vec3(minx, maxy, minz);
	topRight = Vec3(minx, miny, minz);

	float colorFloat[4];
	Rgba8::BLACK.GetAsFloats(colorFloat);
	Vec4 boxColor = Vec4(colorFloat[0], colorFloat[1], colorFloat[2], colorFloat[3]);
	//----------------------------------BOTTOM WALL-----------------------------------------------
	AddVertsForIndexedPNCUQuadtangent3D(bunnyVerts, indexes, quadNormal, (int)bunnyVerts.size(), topLeft, bottomLeft, bottomRight, topRight, boxColor,
		AABB2(Vec2(0.5f, 0.6f), Vec2(0.5f, 0.6f)));

	//----------------------------------ROOF WALL-----------------------------------------------
	quadNormal = Vec3(0, 0, -1);
	bottomLeft = Vec3(maxx, miny, maxz);
	bottomRight = Vec3(maxx, maxy, maxz);
	topLeft = Vec3(minx, miny, maxz);
	topRight = Vec3(minx, maxy, maxz);

	
	AddVertsForIndexedPNCUQuadtangent3D(bunnyVerts, indexes, quadNormal, (int)bunnyVerts.size(), topLeft, bottomLeft, bottomRight, topRight, boxColor,
		AABB2(Vec2(0.5f, 0.6f), Vec2(0.5f, 0.6f)));
	//----------------------LEFT WALL-------------------------------

	bottomLeft = Vec3(maxx, maxy, minz);
	bottomRight = Vec3(minx, maxy, minz);
	topLeft = Vec3(maxx, maxy, maxz);
	topRight = Vec3(minx, maxy, maxz);
	quadNormal = Vec3(0.0f, -1.0f, 0.0f);
	AddVertsForIndexedPNCUQuadtangent3D(bunnyVerts, indexes, quadNormal, (int)bunnyVerts.size(), topLeft, bottomLeft, bottomRight, topRight, boxColor,
		AABB2(Vec2(0.5f, 0.5f), Vec2(0.5f, 0.5f)));


	//-------------------------RIGHT WALL-----------------------------
	bottomLeft = Vec3(minx, miny, minz);
	bottomRight = Vec3(maxx, miny, minz);
	topLeft = Vec3(minx, miny, maxz);
	topRight = Vec3(maxx, miny, maxz);
	quadNormal = Vec3(0.0f, 1.0f, 0.0f);
	AddVertsForIndexedPNCUQuadtangent3D(bunnyVerts, indexes, quadNormal,(int) bunnyVerts.size(), topLeft, bottomLeft, bottomRight, topRight, boxColor,
		AABB2(Vec2(0.5f, 0.5f), Vec2(0.5f, 0.5f)));


	//------------------------BACK  WALL---------------------------
	quadNormal = Vec3(1.0f, 0.0f, 0.0f);
	bottomLeft = Vec3(minx, maxy, minz);
	bottomRight = Vec3(minx, miny, minz);
	topLeft = Vec3(minx, maxy, maxz);
	topRight = Vec3(minx, miny, maxz);

	AddVertsForIndexedPNCUQuadtangent3D(bunnyVerts, indexes, quadNormal, (int)bunnyVerts.size(), topLeft, bottomLeft, bottomRight, topRight, boxColor,
		AABB2(Vec2(0.5f, 0.5f), Vec2(0.5f, 0.5f)));

	//----------------------FRONT WALL------------------------------
	quadNormal = Vec3(-1.0f, 0.0f, 0.0f);
	bottomLeft = Vec3(maxx, miny, minz);
	bottomRight = Vec3(maxx, maxy, minz);
	topLeft = Vec3(maxx, miny, maxz);
	topRight = Vec3(maxx, maxy, maxz);

	AddVertsForIndexedPNCUQuadtangent3D(bunnyVerts, indexes, quadNormal, (int)bunnyVerts.size(), topLeft, bottomLeft, bottomRight, topRight, boxColor,
		AABB2(Vec2(0.5f,0.5f), Vec2(0.5f, 0.5f)));

	g_theRenderer->BuildModelGeometryAndAS(bunnyVerts, indexes, 0);
	return false;
}
void Game::Visualizations()
{
	if (g_theInputSystem->WasKeyJustPressed(KEYCODE_F1))
	{
		m_player->m_position = Vec3(0.0f,0.0f,75.00f);
		m_player->m_orientationDegrees = EulerAngles(0.0f,0.0f,0.0f);
		g_theRenderer->m_lightPosition = Vec4(100.0f,0.0f,70.0f, 1.0f);
	}
	if (g_theInputSystem->WasKeyJustPressed(KEYCODE_F2))
	{
		m_player->m_position = Vec3(1.0f, -0.8f, 46.6f);
		m_player->m_orientationDegrees = EulerAngles(14.0f, 11.40f, 0.0f);
		g_theRenderer->m_lightPosition = Vec4(0.0f, 0.0f, 120.0f, 1.0f);
	}
	if (g_theInputSystem->WasKeyJustPressed(KEYCODE_F3))
	{
		m_player->m_position = Vec3(41.0f, 8.0f, 67.77f);
		m_player->m_orientationDegrees = EulerAngles(95.0f, 23.40f, 0.0f);
		g_theRenderer->m_lightPosition = Vec4(7.0f, 63.0f, 221.0f, 1.0f);
	}
	if (g_theInputSystem->WasKeyJustPressed(KEYCODE_F4))
	{
		m_player->m_position = Vec3(3.8f, 23.8f, 50.8f);
		m_player->m_orientationDegrees = EulerAngles(19.7f, -3.90f, 0.0f);
		g_theRenderer->m_lightPosition = Vec4(7.0f, 63.0f, 240.0f, 1.0f);
	}
	if (g_theInputSystem->WasKeyJustPressed(KEYCODE_F5))
	{
		m_player->m_position = Vec3(3.0f, -15.0f, 58.77f);
		m_player->m_orientationDegrees = EulerAngles(-57.0f, 34.90f, 0.0f);
		g_theRenderer->m_lightPosition = Vec4(7.0f, 63.0f, 240.0f, 1.0f);
	}
	if (g_theInputSystem->WasKeyJustPressed(KEYCODE_F6))
	{
		m_player->m_position = Vec3(-9.0f, -11.0f, 52.0f);
		m_player->m_orientationDegrees = EulerAngles(380.0f, 0.3f, 0.0f);
		g_theRenderer->m_lightPosition = Vec4(7.0f, 520.0f, 240.0f, 1.0f);
	}
	if (g_theInputSystem->WasKeyJustPressed(KEYCODE_F7))
	{
		m_player->m_position = Vec3(0.13f, 101.50f,58.19f);
		m_player->m_orientationDegrees = EulerAngles(633.0f, 2.5f, 0.0f);
		g_theRenderer->m_lightPosition = Vec4(-110.0f, 4500.0f, 341.95f, 1.0f);
	}
}
void Game::UpdateImGuiInfo(float deltaSeconds)
{
	Vec3 playerPosition = m_player->m_position;
	EulerAngles playerOrientation = m_player->m_orientationDegrees;
	IntVec2 playerChunkCoord;
	/*BlockIterator iterator = m_world->GetPlayerPositionBlockIterator();*/
	playerChunkCoord = Chunk::GetChunkCoordsForWorldPosition(playerPosition);
	std::string MinerText = Stringf("XYZ = (%.2f,%.2f,%.2f) YPR = (%.2f,%.2f,%.2f) ds = %.1f MS",
		playerPosition.x, playerPosition.y, playerPosition.z, playerOrientation.m_yawDegrees,
		playerOrientation.m_pitchDegrees, playerOrientation.m_rollDegrees, (double)deltaSeconds * 1000.0f);

	m_imGUI.m_playerInfoText = MinerText;
	MinerText = Stringf("Chunks = % i Verts = % i PlayerChunkCoord = (% i, % i) Light Position = (%.2f, %.2f, %.2f)", (int)m_world->m_activeChunks.size(),
		g_theRenderer->m_DXRverts.size(), playerChunkCoord.x, playerChunkCoord.y, g_theRenderer->m_lightPosition.x,
		g_theRenderer->m_lightPosition.y, g_theRenderer->m_lightPosition.z);

	m_imGUI.m_gameInfoText = MinerText;
}
void Game::CreateSun()
{
	float& sunRadius = m_imGUI.m_thesisVariables.m_sunRadius;
	Vec3 lightPosition = Vec3(g_theRenderer->m_lightPosition.x, g_theRenderer->m_lightPosition.y, g_theRenderer->m_lightPosition.z);
	//AddVertsForIndexedNormalSphere3D(g_theRenderer->m_DXRverts, g_theRenderer->m_DXRindexes, sunRadius, lightPosition, AABB2::ZERO_TO_ONE, Rgba8::WHITE, 0);
}
void Game::BuildAccelerationStructures()
{
	if (m_rebuildAccelerationStructures && m_world->m_totalChunkVerts > 0 && g_theRenderer->MINECRAFTCHUNKS <= m_world->m_activeChunks.size())
	{
		g_theRenderer->BuildGeometryAndAS(g_theRenderer->m_DXRverts, g_theRenderer->m_DXRindexes,1);

		m_rebuildAccelerationStructures = false;
		m_canRender = true;
	}
	//if (g_theRenderer->m_isTopLevelASRebuildRequired /*&& m_world->m_activeChunks.size() == g_theRenderer->MINECRAFTCHUNKS*/)
	//{
	//	g_theRenderer->BuildTLAS();
	//	m_canRender = true;
	//}
}
void Game::UpdateGameBar(float deltaSeconds)
{
	//CalculateRaytraceQuadVerts();
	//std::string GameBarUpdate = Stringf("Raytracing SimpleMiner");
	std::string GameBarUpdate = Stringf("Raytracing SimpleMiner DispatchRaysDuration : %.4f ms GPUWaitTime: %.4f ms FPS:%.1f Vertices: %i", 
	g_theRenderer->m_dispatchRayRuntime * 1000.0f, g_theRenderer->m_gpuWaitTime * 1000.0f, 1/deltaSeconds , (int)g_theRenderer->m_DXRverts.size());
	g_theApp->UpdateGameName(GameBarUpdate);
}
void Game::CalculateRaytraceQuadVerts()
{
	Vec3 ibasis = Vec3(1,0,0), jbasis = Vec3(0,1,0), kbasis = Vec3(0,0,1);
    m_worldCamera.GetCameraOrientation().GetAsVectors_XFwd_YLeft_ZUp(ibasis, jbasis, kbasis);
	DebuggerPrintf(Stringf("Player Rotation - x: %.2f y:%.2f z:%.2f \n", m_player->m_orientationDegrees.m_yawDegrees, m_player->m_orientationDegrees.m_pitchDegrees, m_player->m_orientationDegrees.m_rollDegrees).c_str());
	AABB2 actorQuad;
	actorQuad.m_mins.x = -2.4f;
	actorQuad.m_maxs.x = 2.4f;
	actorQuad.m_mins.y = -1.4f;
	actorQuad.m_maxs.y = 1.4f;
	Vec3 topleft, bottomleft, topRight, bottomRight;
	topleft = m_worldCamera.m_position + (jbasis * actorQuad.m_maxs.x) + (kbasis * actorQuad.m_maxs.y) + ibasis *2;
	bottomleft = m_worldCamera.m_position + (jbasis * actorQuad.m_maxs.x) + (kbasis * actorQuad.m_mins.y) + ibasis * 2;
	topRight = m_worldCamera.m_position + (-jbasis * actorQuad.m_maxs.x) + (kbasis * actorQuad.m_maxs.y) + ibasis *2;
	bottomRight = m_worldCamera.m_position + (-jbasis * actorQuad.m_maxs.x) + (kbasis * actorQuad.m_mins.y) + ibasis * 2;

	//g_theRenderer->SetRaytraceQuadCamera(topleft, bottomleft, topRight, bottomRight);
}

void Game::SwitchToGame(float deltaSeconds)
{
	UNUSED((void)deltaSeconds);
	m_worldCamera.SetPerspectiveView(g_theWindow->GetConfig().m_clientAspect, 60.0f, 0.1f, 1000.0f);
	g_theInputSystem->SetCursorAtCenter();
	m_IsAttractMode = false;
	m_currentGameState = GameStates::GameWorld;
}
void Game::EndFrame()
{
	
}
void Game::UpdateSceneConstants()
{
	std::uniform_int_distribution<UINT> seedDistribution(0, UINT_MAX);
	int kernelSize = m_imGUI.m_thesisVariables.m_denoiserKernelSize;
	g_theRenderer->m_gameValues.samplingData = m_imGUI.m_thesisVariables.m_samplingData;
	//g_theRenderer->m_gameValues.samplingData.w = (float)seedDistribution(g_theRenderer->m_sampler->m_generatorURNG2);
	g_theRenderer->m_gameValues.samplingData.w = (float)m_frameNumber;
	g_theRenderer->m_gameValues.GIColor = m_imGUI.m_thesisVariables.m_color;
	g_theRenderer->m_gameValues.lightBools = m_imGUI.m_thesisVariables.m_lightBools;
	g_theRenderer->m_temporalFade = m_imGUI.m_thesisVariables.m_temporalFade;
	g_theRenderer->m_gameValues.textureMappings = m_imGUI.m_thesisVariables.m_textureMappings;


	g_theRenderer->m_gameValues.lightfallOff_AmbientIntensity_CosineSampling.x = m_imGUI.m_thesisVariables.m_lightFallof;
	g_theRenderer->m_gameValues.lightfallOff_AmbientIntensity_CosineSampling.y = m_imGUI.m_thesisVariables.m_ambientIntensity;
	g_theRenderer->m_gameValues.lightfallOff_AmbientIntensity_CosineSampling.z = m_imGUI.m_thesisVariables.m_samplingMode;
	g_theRenderer->m_compositor->m_renderOutput = m_imGUI.m_thesisVariables.m_renderOutput;
	g_theRenderer->m_denoiser->SetFilterSize(kernelSize);
	g_theRenderer->m_denoiser->m_denoiserType = m_imGUI.m_thesisVariables.m_denoiserType == 0 ? 
		DenoiserType::GaussianFilter : DenoiserType::AtrousBilateral;

	g_theRenderer->m_denoiser->m_totalAtrousSteps = m_imGUI.m_thesisVariables.m_atrousStepSize;
	g_theRenderer->m_denoiser->m_varianceFilteringOn = m_imGUI.m_thesisVariables.m_varianceFiltering;
	
	if (g_theRenderer->m_currentScene != (Scenes)m_imGUI.m_thesisVariables.m_currentScene)
	{
		if (m_imGUI.m_thesisVariables.m_currentScene == (int)Scenes::Bunny)
		{
			m_player->m_position = Vec3(8, 0, 5);
			g_theRenderer->m_lightPosition = Vec4(0, 0, 5, 1.0f);
			m_player->m_orientationDegrees =EulerAngles(175,0,0);
		}
		else
		{
			m_player->m_position = Vec3(0, 0, 65);
			g_theRenderer->m_lightPosition = Vec4(0, 0, 70, 1.0f);
		}
		g_theRenderer->m_currentScene = (Scenes)m_imGUI.m_thesisVariables.m_currentScene;
	}
}
void Game::ChangeMouseSettings()
{

	bool windowHasFocus = g_theRenderer->GetRenderConfig().m_window->HasFocus();
	if (g_theInputSystem->WasKeyJustPressed(KEYCODE_SPACE))
	{
		m_ImguiMode = !m_ImguiMode;
	}
	if (!windowHasFocus)
	{
		g_theInputSystem->SetMouseMode(false, false, false);
	}
	else if (windowHasFocus)
	{
		if (m_IsAttractMode || m_ImguiMode)
		{
			g_theInputSystem->SetMouseMode(false, false, false);
		}
		else
		{
			g_theInputSystem->SetMouseMode(true, true, true);
		}
	}
	/*if (m_ImguiMode)
	{
		g_theInputSystem->UpdateCursorPositionForThesis();
	}*/
}
void Game::DebugRender() const
{

}

void Game::RenderAttractMode() const
{

}
void Game::UpdateAttractMode(float deltaSeconds)
{
	if (g_theInputSystem->WasKeyJustPressed(' ') ||
		g_theInputSystem->WasKeyJustPressed('N') ||
		g_theInputSystem->GetController(0).WasButtonJustPressed(XboxButtonID::XBOX_BUTTON_A) ||
		g_theInputSystem->GetController(0).WasButtonJustPressed(XboxButtonID::XBOX_BUTTON_START))
	{
		SwitchToGame(deltaSeconds);
	}
}
void Game::GetInputAndMovePlayer(float deltaSeconds)
{
	if (g_theInputSystem->IsKeyDown(16) || g_theInputSystem->GetController(0).IsButtonDown(XboxButtonID::XBOX_BUTTON_A))
	{
		m_playerVelocity = 25.0f;
	}
	else
	{
		m_playerVelocity = 15.0f;
	}
	//if (g_theInputSystem->IsKeyDown('H') || g_theInputSystem->GetController(0).IsButtonDown(XboxButtonID::XBOX_BUTTON_START))
	//{
	//	m_player->m_position = Vec3(0.0f, 0.0f, 0.0f);
	//	m_player->m_orientationDegrees = EulerAngles(0.0f, 0.0f, 0.0f);
	//}
	Mat44 playerMatrix = m_player->GetModalMatrix();
	if (g_theInputSystem->IsKeyDown('W'))
	{

		m_player->m_position += playerMatrix.GetIBasis3D() * deltaSeconds * m_playerVelocity;
	}
	if (g_theInputSystem->IsKeyDown('S'))
	{

		m_player->m_position -= playerMatrix.GetIBasis3D() * deltaSeconds * m_playerVelocity;
	}
	if (g_theInputSystem->IsKeyDown('D'))
	{

		m_player->m_position -= playerMatrix.GetJBasis3D() * deltaSeconds * m_playerVelocity;
	}
	if (g_theInputSystem->IsKeyDown('A'))
	{

		m_player->m_position += playerMatrix.GetJBasis3D() * deltaSeconds * m_playerVelocity;
	}
	if (g_theInputSystem->IsKeyDown('Z'))
	{

		m_player->m_position += playerMatrix.GetKBasis3D() * deltaSeconds * m_playerVelocity;
	}
	if (g_theInputSystem->IsKeyDown('C'))
	{

		m_player->m_position -= playerMatrix.GetKBasis3D() * deltaSeconds * m_playerVelocity;
	}
	if (g_theInputSystem->IsKeyDown('I'))
	{
		g_theRenderer->m_lightPosition.x += deltaSeconds * m_playerVelocity  * 4.0f;
	}
	if (g_theInputSystem->IsKeyDown('J'))
	{
		g_theRenderer->m_lightPosition.y -= deltaSeconds * m_playerVelocity  * 4.0f;
	}
	if (g_theInputSystem->IsKeyDown('K'))
	{
		g_theRenderer->m_lightPosition.x -= deltaSeconds * m_playerVelocity  * 4.0f;
	}
	if (g_theInputSystem->IsKeyDown('L'))
	{
		g_theRenderer->m_lightPosition.y += deltaSeconds * m_playerVelocity  * 4.0f;
	}
	if (g_theInputSystem->IsKeyDown('N'))
	{
		g_theRenderer->m_lightPosition.z -= deltaSeconds * m_playerVelocity  * 4.0f;
	}
	if (g_theInputSystem->IsKeyDown('M'))
	{
		g_theRenderer->m_lightPosition.z += deltaSeconds * m_playerVelocity  * 4.0f;
	}
	g_theRenderer->m_lightPosition.w =1.0f;
	Vec2 mouseDelta = g_theInputSystem->GetMouseClientDelta();
	m_player->m_orientationDegrees.m_yawDegrees += (mouseDelta.x * m_mouseSensitivity.x);
	m_player->m_orientationDegrees.m_pitchDegrees -= (mouseDelta.y * m_mouseSensitivity.y);
	//DebuggerPrintf(Stringf("Player Position - x: %.2f y:%.2f z:%.2f \n", m_player->m_position.x, m_player->m_position.y, m_player->m_position.z).c_str());
	m_player->Update(deltaSeconds);
}
void Game::InitializeCameras()
{
	m_worldCamera = Camera(-1.0f, -1.0f, CAMERA_WORLD_SIZEX, CAMERA_WORLD_SIZEY);
	m_screenCamera = Camera(0.0f, 0.0f, CAMERA_SCREEN_SIZEX, CAMERA_SCREEN_SIZEY);

	Vec3 ibasis = Vec3(0.0f, 0.0f, 1.0f);
	Vec3 jbasis = Vec3(-1.0f, 0.0f, 0.0f);
	Vec3 kbasis = Vec3(0.0f, 1.0f, 0.0f);
	m_worldCamera.SetViewToRenderTransform(ibasis,jbasis,kbasis);
}
void Game::LoadGameSounds()
{
	m_gameSounds[(int)GameSounds::GamePause] = g_theAudioSystem->CreateOrGetSound("Data/Audio/Pause.mp3");
	m_gameSounds[(int)GameSounds::GameVictory] = g_theAudioSystem->CreateOrGetSound("Data/Audio/Victory.mp3");
}
SoundPlaybackID Game::PlaySounds(std::string soundPath)
{
	SoundID NewLevel = g_theAudioSystem->CreateOrGetSound("Data/Audio/NewLevel.mp3");
	return g_theAudioSystem->StartSound(NewLevel);
}
void Game::StopSounds(SoundPlaybackID soundInstance)
{
	g_theAudioSystem->StopSound(soundInstance);
}

void Game::RenderEngineLogo() const
{
	
}
void Game::UpdateEngineLogo(float deltaSeconds)
{
	g_theApp->m_isDebugDraw = false;
	m_currentEngineLogoSeconds += deltaSeconds;
	m_engineLogoFireSeconds -= deltaSeconds;
	if (m_currentEngineLogoSeconds >= m_engineLogoSeconds)
	{
		SwitchGameModes(GameStates::Attract);
	}
	if (g_theInputSystem->WasKeyJustPressed(' '))
	{
		m_currentGameState = GameStates::Attract;
	}
}
void Game::UpdateGameState()
{
	//if (m_PlayerVictory)
	//	SwitchGameModes(GameStates::Victory);
	//if (m_isGameOver)
	//	SwitchGameModes(GameStates::PlayerDead);
}
void Game::SwitchGameModes(GameStates gameState)
{
	if (m_currentGameState != gameState)
	{
		m_currentGameState = gameState;
		if (m_currentGameState == GameStates::Victory)
		{
			PlaySoundSfx(m_gameSounds[(int)GameSounds::GameVictory]);
		}
		/*if (m_currentGameState != GameStates::Victory)
		{
			m_PlayerVictory = false;
		}*/
	}

}
void Game::PlaySoundSfx(SoundPlaybackID soundID)
{
	if (soundID != MISSING_SOUND_ID)
		g_theAudioSystem->StartSound(soundID, false, 0.0f);

	else return;

}
bool Game::GetIsMenuScreen()
{
	return m_IsAttractMode;
}
void Game::UpdateMenuScreenPlayButton(float deltaSeconds)
{
	m_playButtonAlpha -= deltaSeconds * 100.0f;

	if (m_playButtonAlpha <= 0)
	{
		m_playButtonAlpha = 255.0f;
	}
}
void Game::RenderMenuScreenPlayButton() const
{
	Vertex_PCU tempVertexArrays[3];
	VertexArray tempVertexArray;
	tempVertexArrays[0] = Vertex_PCU(Vec3(5.0f, 0.0f, 0.f), Rgba8(0, 255, 0, (unsigned char)m_playButtonAlpha), Vec2(0.f, 0.f));
	tempVertexArrays[1] = Vertex_PCU(Vec3(-5.0f, -5.0f, 0.f), Rgba8(0, 255, 0, (unsigned char)m_playButtonAlpha), Vec2(0.f, 0.f));
	tempVertexArrays[2] = Vertex_PCU(Vec3(-5.0f, 5.0f, 0.f), Rgba8(0, 255, 0, (unsigned char)m_playButtonAlpha), Vec2(0.f, 0.f));
	tempVertexArray.push_back(tempVertexArrays[0]);
	tempVertexArray.push_back(tempVertexArrays[1]);
	tempVertexArray.push_back(tempVertexArrays[2]);

	TransformVertexArray3D(3, tempVertexArray, 8.0f, 0.0f, Vec3(WORLD_CENTER_X * 8, WORLD_CENTER_Y * 8, 0.0f));
	g_theRenderer->DrawVertexArray(3, tempVertexArray);

}
void Game::SwitchGBuffers()
{
	if (g_theInputSystem->WasKeyJustPressed('1'))
	{
		m_imGUI.m_thesisVariables.m_renderOutput = 1;
	}
	if (g_theInputSystem->WasKeyJustPressed('2'))
	{
		m_imGUI.m_thesisVariables.m_renderOutput = 2;
	}
	if (g_theInputSystem->WasKeyJustPressed('3'))
	{
		m_imGUI.m_thesisVariables.m_renderOutput = 3;
	}
	if (g_theInputSystem->WasKeyJustPressed('4'))
	{
		m_imGUI.m_thesisVariables.m_renderOutput = 4;
	}
	if (g_theInputSystem->WasKeyJustPressed('5'))
	{
		m_imGUI.m_thesisVariables.m_renderOutput = 5;
	}
	if (g_theInputSystem->WasKeyJustPressed('6'))
	{
		m_imGUI.m_thesisVariables.m_renderOutput = 6;
	}
	if (g_theInputSystem->WasKeyJustPressed('7'))
	{
		m_imGUI.m_thesisVariables.m_renderOutput = 7;
	}
	if (g_theInputSystem->WasKeyJustPressed('8'))
	{
		m_imGUI.m_thesisVariables.m_renderOutput = 8;
	}
	if (g_theInputSystem->WasKeyJustPressed('9'))
	{
		m_imGUI.m_thesisVariables.m_renderOutput = 9;
	}
	if (g_theInputSystem->WasKeyJustPressed('0'))
	{
		m_imGUI.m_thesisVariables.m_renderOutput = 0;
	}
}