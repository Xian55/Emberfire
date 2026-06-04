#pragma once

#include "MapCellClient.h"

#include "..\Geo2d.h"

class ClientMap;
class MapCellClient;
class World;

class WorldRenderable
{
	public:
		virtual ~WorldRenderable();

		virtual void render();
		virtual void update();
		virtual void initCamera() {}
		virtual void setCurrentCell(const int c, MapCellClient& cell);

		virtual bool isRenderedOnLayer(const ClientMap::Defines layer) { return layer == ClientMap::Defines::Layer3; }
		virtual bool emitsLight(MapCellClient::LightSource* ls = nullptr) { return false; }

		void computeScreenPosition();
		void setScreenPosition(const sf::Vector2i p) { m_screenPosition = p; }
		void setWorldPosition(const sf::Vector2f p) { m_worldPosition = { p.x, p.y }; }
		void setDone(const bool v) { m_done = v; }
		void setCameraPtr(Geo2d::Vector2 const* ptr);
		void playSound(const string& soundname, const float distanceModifier = 1.f, const float volume = 1.f, const int delayms = 0);
		void playSound(const Assets::SfxId id, const float distanceModifier = 1.f, const float volume = 1.f, const int delayms = 0)
			{ playSound(Assets::sfxFile(id), distanceModifier, volume, delayms); }

		bool done() { return m_done; }

		int getCurrentCell() const { return m_currentCell; }

		auto& getWorldPositionRef() { return m_worldPosition; }

		const auto& getWorldPosition() const { return m_worldPosition; }
		const auto& getScreenPosition() const { return m_screenPosition; }

		// Ignores camera
		Geo2d::Vector2 computeRawScreenPosition() const;
		sf::Vector2i computeRawScreenPositioni() const;
		
		ClientMap* getMap() const { return m_map; }
		World* getWorld() const { return m_world; }

	protected:
		WorldRenderable(ClientMap* clientMap);

		bool m_done;

		int m_currentCell;

		ClientMap* m_map{nullptr};
		World* m_world{nullptr};

		sf::Vector2i m_screenPosition;

		Geo2d::Vector2 m_worldPosition;
		Geo2d::Vector2 const* m_cameraPtr;
};

