
#include "Engine/Math/Vec2.hpp"
#include "Engine/Renderer/RendererD12.hpp"
#include "Engine/Core/Time.cpp"
#include "Engine/Input/InputSystem.hpp"
#include <Engine/Core/ErrorWarningAssert.hpp>
#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/Window/Window.hpp"
#include "Engine/Core/DevConsole.hpp"
#include "Game/App.hpp"
#include "Game/Game.hpp"
#include "Engine/Core/Clock.hpp"
#include "Engine/Renderer/DebugRenderer.hpp"
#include <Engine/Core/EngineCommon.cpp>

//#define _ITERATOR_DEBUG_LEVEL 0

App* g_TheApp = nullptr;
Game* g_theGame = nullptr;
RendererD12* g_theRenderer = nullptr;
InputSystem* g_theInputSystem = nullptr;
AudioSystem* g_theAudioSystem = nullptr;
Window* g_theWindow = nullptr;
DevConsole* g_theConsole = nullptr;



float g_TimeLastFrame = 0.0f;
	App::App()
	{

	}
	App::~App()
	{

	}
	void App::Startup()
	{
		PopulateGameConfigs("Data/GameConfig.xml");
		EventSystemConfig eventSystemConfig;
		g_theEventSystem = new EventSystem(eventSystemConfig);

		InputSystemConfig inputConfig;
		g_theInputSystem = new InputSystem(inputConfig);

		WindowConfig windowConfig;
		windowConfig.m_windowTitle = GameName;
		windowConfig.m_clientAspect = 2.0f;
		windowConfig.m_inputSystem = g_theInputSystem;
		g_theWindow = new Window(windowConfig);

		RendererD12Config rendererConfig;
		rendererConfig.m_window = g_theWindow;
		rendererConfig.m_isRaytracingEnabled = true;
		g_theRenderer = new RendererD12(rendererConfig);

		/*DevConsoleConfig devConsoleConfig;
		devConsoleConfig.m_renderer = g_theRenderer;
		g_theConsole = new DevConsole(devConsoleConfig);*/

		AudioSystemConfig audioConfig;
		g_theAudioSystem = new AudioSystem(audioConfig);

		g_theEventSystem->Startup();
		g_theInputSystem->Startup();
		g_theAudioSystem->Startup();
		g_theWindow->SetRenderTextureDimensions(IntVec2(1280, 720));
		g_theWindow->Startup();
		g_theRenderer->Startup();
	/*	g_theConsole->Startup();*/

		g_theGame = new Game();
		g_theGame->Startup();

		g_theEventSystem->SubscribeEventCallbackFunction("ApplicationQuit", Application_Quit);
	}
	void App::Shutdown()
	{
		g_theRenderer->ShutDown(); 
		g_theWindow->ShutDown();
		g_theGame->ShutDown();
	/*	g_theConsole->ShutDown();*/
		g_theEventSystem->ShutDown();
		g_theInputSystem->ShutDown();
		g_theAudioSystem->Shutdown();
	
		delete g_theInputSystem;
		delete g_theAudioSystem;
		delete g_theWindow;
		delete g_theGame;
		/*delete g_theConsole;*/
		delete g_theRenderer;
		delete g_theEventSystem;

		g_theInputSystem = nullptr;
		g_theEventSystem = nullptr;
		g_theAudioSystem = nullptr;
		g_theWindow = nullptr;
		g_theGame = nullptr;
		g_theConsole = nullptr;
		g_theRenderer = nullptr;

		EngineShutdown();
	}
	void App::RunFrame()
	{
		float deltaSeconds = static_cast<float>(m_gameClock.GetDeltaTime());
		BeginFrame();
		Update(deltaSeconds);
		Render();
		EndFrame();
	}
	void App::Run()
	{
		while (!isQuitting() && !m_isQuitting)
		{
			RunFrame();
		}
	}
	void App::BeginFrame()
	{
		Clock::SystemBeginFrame();
		g_theInputSystem->BeginFrame();
		/*g_theConsole->BeginFrame();*/
		g_theAudioSystem->BeginFrame();
		g_theRenderer->BeginFrame();
		g_theWindow->BeginFrame();
	
	}
	void App::Update(float deltaSeconds)
	{
		if (g_theInputSystem->IsKeyDown(KEYCODE_ESCAPE) == true)
		{
			HandleQuitRequested();
			/*if (g_theConsole->GetMode() == DevConsoleMode::VISIBLE)
			{
				g_theInputSystem->ConsumeKeyJustPressed(KEYCODE_ESCAPE);
			}
			//else*//* if (!g_theGame->GetIsMenuScreen())*/
			//{
			//	g_theInputSystem->ConsumeKeyJustPressed(KEYCODE_ESCAPE);
			//	m_isRestart = true;
			//	RestartGame();
			//}
			//else
			//{
			//	HandleQuitRequested();
			//}
		}
		if (g_theInputSystem->WasKeyJustPressed(VK_OEM_3))
		{
			/*g_theConsole->ToggleMode(g_theConsole->GetMode());*/
		}
		if (g_theInputSystem->WasKeyJustPressed('P') == true)
		{
			g_theInputSystem->ConsumeKeyJustPressed('P');
			m_gameClock.TogglePause();
		}
	    if (g_theInputSystem->IsKeyDown('T') == true) // If the game is Slowed by pressing T
		{
			m_gameClock.SetTimeDilation(0.1f);
		}
		else
		{
			m_gameClock.SetTimeDilation(1.0);
		}
		if (g_theInputSystem->WasKeyJustPressed('O') == true) // If the game is in step move by pressing O
		{
			g_theInputSystem->ConsumeKeyJustPressed('O');
			m_gameClock.StepFrame();
			m_isPaused = false;
			m_isStepMove = true;
		}
		g_theGame->Update(deltaSeconds);
			
	}
	void App::DebugRender() const
	{
		g_theGame->DebugRender();
	}
	void App::Render() const
	{
		g_theGame->Render();
		
		if (m_isDebugDraw)
		{
			DebugRender();
		}
		if (g_theGame->GetIsMenuScreen())
		{
			/*DebugRenderScreen(g_theGame->m_screenCamera);*/
		}
		else
		{
			if (!g_theGame->GetIsMenuScreen())
			{
				/*DebugRenderScreen(g_theGame->m_screenCamera);
				DebugRenderWorld(g_theGame->m_worldCamera);*/
			}
		}
		
		//if (g_theConsole->m_mode == DevConsoleMode::VISIBLE)
		//{
		//	g_theRenderer->BeginCamera(g_theGame->m_screenCamera);
		//	g_theGame->m_screenCamera.SetOrthoView();
		//	g_theConsole->Render(AABB2(0.0f, 0.0f, 1600.0f, 800.0f), nullptr);
		//	g_theRenderer->EndCamera(g_theGame->m_screenCamera);
		//}
		
	}
	void App::EndFrame()
	{
		if (m_isStepMove)
		{
			m_isStepMove = false;
			m_isPaused = true;
		}
	/*	DebugRenderClear();*/
		g_theGame->EndFrame();
		g_theRenderer->EndFrame();
		g_theInputSystem->EndFrame();
		g_theAudioSystem->EndFrame();
	
	}

	bool App::HandleKeyPressed(unsigned char keyCode)
	{
		g_theInputSystem->HandleKeyPressed(keyCode);
		if (keyCode == 'O')
		{
			m_isStepMove = true; // Check if O was pressed and enable single step mode
		}
		if (keyCode == 'P')		// Check if P was pressed and pause the game
		{
			m_isPaused = m_isPaused == true? false:true;
		}
		if (keyCode == KEYCODE_F1)		// Check if f1 was pressed and enable debug draw mode
		{
			m_isDebugDraw = m_isDebugDraw == true ? false : true;
		}
		if (keyCode == KEYCODE_F8)		// Check if f8 was pressed and restart game
		{
			m_isRestart = true;
			RestartGame();
		}
		if (keyCode == KEYCODE_ESCAPE)
		{
			if (!g_theGame->GetIsMenuScreen())
			{
				m_isRestart = true;
				RestartGame();
			}
			else
			{
				HandleQuitRequested();
			}	
		}

		return false;
	}
	bool App::HandleKeyReleased(unsigned char keyCode)
	{
		g_theInputSystem->HandleKeyReleased(keyCode);
		return false;
	}
	bool App::IsKeyPressed(unsigned char keyCode)
 	{
		return g_theInputSystem->IsKeyDown(keyCode);
	
	}
	bool App::WasKeyPressedPreviousFrame(unsigned char keyCode)
	{
		return !g_theInputSystem->WasKeyJustPressed(keyCode);
	}
	void App::UpdateGameName(std::string name)
	{
		GameName = name;
		g_theRenderer->GetRenderConfig().m_window->ChangeTitle(name);
	}
	bool App::Application_Quit(EventArgs& args)
	{
		UNUSED((void) args);
		m_isQuitting = true;
		return false;
	}
	void App::HandleQuitRequested()
	{
		m_isQuitting = true;
		
	}
	void App::RestartGame()
	{
		m_isRestart = false;
		g_theGame->ShutDown();

		delete g_theGame;
		g_theGame = nullptr;

		g_theGame = new Game();
		g_theGame->Startup();
	}

	
