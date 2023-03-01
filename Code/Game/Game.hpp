
#pragma once
#include "Game/Player.hpp"
#include "Game/Props.hpp"
#include "Game/World.hpp"
#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Core/Vertex_PNCU.hpp"
#include "Engine/Renderer/D3D12ImGui.hpp"

class App;

enum class GameStates
{
	EngineLogo = 0,
	Attract,
	GameWorld,
	GamePaused,
	PlayerDead,
	Victory
};
enum class GameSounds
{
	GamePause = 0,
	GameVictory,
	GameOver,
	TotalSounds
};

class Game
{
public:
	Game();
	~Game() {}
	//------------------------------MAIN FUNCTIONS------------------------------------------------------
	void				Startup();
	void				InitializeRaytracing();
	void				InitializeImGui();
	void				Update(float deltaSeconds);
	void				Render();
	void				ShutDown();
	void				DebugRender() const;

	//------------------------------MENU SCREEN FUNCTIONS-----------------------------------------------
	void				RenderAttractMode() const;
	void				UpdateAttractMode(float deltaSeconds);
	bool				GetIsMenuScreen();
	void				UpdateMenuScreenPlayButton(float deltaSeconds);
	void				RenderMenuScreenPlayButton() const;

	//------------------------------GAME FUNCTIONS-------------------------------------------------------
	void				UpdateGameState();
	void				SwitchGameModes(GameStates gameState);
	void				UpdateGame(float deltaSeconds);
	void				RenderGame();

	void				CreateSun();
	bool				ImportBunny();
	void				Visualizations();
	void				UpdateImGuiInfo(float deltaSeconds);
	void				SwitchGBuffers();
	//-----------------------------GAME HELPER FUNCTIONS-------------------------
	void				UpdateGameBar(float deltaSeconds);
	void				CalculateRaytraceQuadVerts();
	void				BuildAccelerationStructures();

	//---------------------------RAYTRACING FUNCTIONS-------------------------------
	void				GetInputAndMovePlayer(float deltaSeconds);
	void				InitializeCameras();
	void				PlaySoundSfx(SoundPlaybackID soundID);
	void				UpdateEngineLogo(float deltaSeconds);
	void				RenderEngineLogo() const;
	void				LoadGameSounds();
	SoundPlaybackID		PlaySounds(std::string soundPath);
	void				StopSounds(SoundPlaybackID soundInstance);
	void				SwitchToGame(float deltaSeconds);
	void				ChangeMouseSettings();
	void				UpdateSceneConstants();

	Camera				m_worldCamera;
	Camera				m_screenCamera;

	void				EndFrame();


public:
	//--------------------------SIMPLEMINER VARIABLES------------------
	bool				m_ImguiMode = false;

	int					m_frameNumber = 0;
	D3D12ImGui			m_imGUI;
	int					m_minimumChunksForASBuild = 100;
	int					m_maximumChunksForASBuild = 400;
	bool				m_accelerationStructuresBuilt = false;
	bool				m_rebuildAccelerationStructures = true;
	bool				m_canRender = false;
	World*				m_world = nullptr;
	std::vector<Vertex_PNCU> raytraceQuadVerts;
	Player*				m_player;
	Props*				m_cubeProp;
	GameStates			m_currentGameState = GameStates::Attract;
	SoundID				m_gameSounds[(int)GameSounds::TotalSounds] = {};
	float				m_currentEngineLogoSeconds = 0.0f;
	float				m_engineLogoFireSeconds = 2.0f;
	float				m_engineLogoSeconds = 3.5f;
	Vec2				m_mouseSensitivity  = Vec2(0.1f, 0.1f);
	float				m_quadDistanceFromCamera = 0.2f;
	float				m_playerVelocity = 1.0f;
private:
	bool				m_IsAttractMode = false;
	float			    m_playButtonAlpha = 255.0f;
	Vec2				m_cursorPosition;			
};
