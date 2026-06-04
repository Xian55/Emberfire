// Class partially based on the particle system from "Haaf's Game Engine".
// Revised to be compatible with SFML.

#pragma once

#include "ParticleSystemInfo.h"

class ParticleSystem : public sf::Drawable, public sf::Transformable
{
	#define MaxParticles 500
	
	struct Particle
	{
		float gravity;
		float radialAccel;
		float tangentialAccel;
		float spin;
		float spinDelta;
		float size;
		float sizeDelta;
		float age;
		float terminalAge;

		ColorRGBf color;
		ColorRGBf colorDelta;

		sf::Vector2f location;
		sf::Vector2f velocity;
	};

	public:
		ParticleSystem(const ParticleSystemInfo& info);
		virtual ~ParticleSystem();

		void transpose(const float dx, const float dy);
		void moveTo(const sf::Vector2f position, const bool moveParticles);
		void update(const sf::Time elapsed);
		void stop() { m_done = true; }
		void resume() { m_done = false; }

		bool stopped() const { return m_done; }
		bool finished() const;

		const auto& getLocation() const { return m_location; }

	private:
		void draw(sf::RenderTarget& target, sf::RenderStates states) const;

	private:
		void spawnParticle();
		void moveParticle(const int particleId, const sf::Vector2f& adjustment);
		void setParticleVerticiesColor(const int particleId, const sf::Color a);
		void eraseParticleVerts(const int particleId);
		void setParticleVerticiesPosition(const int particleId, const float x, const float y, const float scale = 1.0f);
		void normalizeVector(sf::Vector2f& v);
		
		float invSqrt(float x);

		float m_tx;
		float m_ty;
		float m_scale;
		float m_emissionResidue;
		float m_age;

		bool m_done;

		ParticleSystemInfo m_info;

		sf::Vector2f m_location;
		sf::Vector2f m_prevLocation;
		
		// Particle[i] == Verticies[(i * 4) + 0, 1, 2, 3]
		vector<Particle> m_particles;
		vector<sf::Vertex> m_vertices;

		shared_ptr<sf::Texture> m_texture;
};

