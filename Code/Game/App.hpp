#pragma once
#include "Engine/Math/Vec2.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Core/Clock.hpp"
#include <string>
#include "Engine/Core/EventSystem.hpp"

class RendererD12;
class Game;
static bool m_isQuitting = false;;
class App 
{
	public:
		App();
		~App();
		void Startup();
		void Shutdown();
		void RunFrame();
		void Run();

		bool isQuitting() const { return m_isQuitting; };
		bool HandleKeyPressed (unsigned char keyCode );
		bool HandleKeyReleased (unsigned char keyCode );
		void HandleQuitRequested();
		void RestartGame();
		bool IsKeyPressed(unsigned char keyCode);
		bool WasKeyPressedPreviousFrame(unsigned char keyCode);
		void UpdateGameName(std::string name);
		static bool Application_Quit(EventArgs& args);
		Clock		m_gameClock;
		bool m_isDebugDraw = false;
		std::string GameName = "Raytracing Minecraft";
	private:
		void BeginFrame();
		void Update(float deltaSeconds);
		void DebugRender() const;
		void Render() const;
		void EndFrame();

	private:
		
		bool m_isPaused = false;
	
		bool m_isSlowMo = false;
		bool m_isStepMove = false;
		bool m_isRestart = false;
		Vec2 m_shipPos;
		float gameSeconds = 3.0f;
		Rgba8 m_clearScreenColoring = Rgba8(25, 200, 50, 255);
		

};