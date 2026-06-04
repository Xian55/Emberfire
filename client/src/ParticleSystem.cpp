#include "stdafx.h"
#include "ParticleSystem.h"
#include "..\Rand.h"
#include "..\Math.h"
#include "ContentMgr.h"

#include <assert.h>

ParticleSystem::ParticleSystem(const ParticleSystemInfo& info) :
	m_age(0.0f),
	m_location(0, 0),
	m_tx(0),
	m_ty(0),
	m_scale(1.0f),
	m_emissionResidue(0),
	m_done(false),
	m_info(info)
{
	ASSERT(m_texture = sContentMgr->getTexture("particles.png"));
}

ParticleSystem::~ParticleSystem()
{

}

void ParticleSystem::transpose(const float dx, const float dy)
{
	if (dx == 0.0f && dy == 0.0f)
		return;

	for (size_t i = 0; i < m_particles.size(); ++i)
	{
		auto& p = m_particles[i];

		p.location.x += dx;
		p.location.y += dy;

		setParticleVerticiesPosition(i, p.location.x, p.location.y, p.size);
	}

	m_location.x += dx;
	m_location.y += dy;

	m_prevLocation.x += dx;
	m_prevLocation.y += dy;
}

void ParticleSystem::moveTo(const sf::Vector2f position, const bool moveParticles)
{
	if (m_location == position)
		return;

	float dx = 0.0f;
	float dy = 0.0f;

	if (moveParticles)
	{
		dx = position.x - m_location.x;
		dy = position.y - m_location.y;

		for (size_t i = 0; i < m_particles.size(); ++i)
		{
			auto& p = m_particles[i];

			p.location.x += dx;
			p.location.y += dy;

			setParticleVerticiesPosition(i, p.location.x, p.location.y, p.size);
		}

		m_prevLocation.x = m_prevLocation.x + dx;
		m_prevLocation.y = m_prevLocation.y + dy;
	}
	else 
	{
		if (m_age == 0.0f)
			m_prevLocation = position;
		else
			m_prevLocation = m_location;
	}

	m_location = position;
}

void ParticleSystem::update(const sf::Time elapsed)
{
	m_age += elapsed.asSeconds();

	// Generate a certain number of particles per second based on emission
	const float particlesNeeded = m_info.emission() * elapsed.asSeconds() + m_emissionResidue;
	const int nParticlesCreated = static_cast<int>(abs(particlesNeeded));
	m_emissionResidue = particlesNeeded - nParticlesCreated;

	if (!m_done)
	{
		for (int i = 0; i < nParticlesCreated && m_particles.size() < MaxParticles; ++i)
			spawnParticle();
	}

	float ang = 0.0f;
	sf::Vector2f vecAccel;
	sf::Vector2f vecAccel2;

	int particleId = 0;
	auto itr = m_particles.begin();

	// Update the particles that we have
	while (itr != m_particles.end())
	{
		Particle& par = *itr;
		par.age += elapsed.asSeconds();

		if (par.age >= par.terminalAge)
		{
			itr = m_particles.erase(itr);
			eraseParticleVerts(particleId);
		}
		else
		{
			vecAccel = par.location - m_location;
			normalizeVector(vecAccel);

			vecAccel2 = vecAccel;
			vecAccel *= par.radialAccel;

			// Rotate(M_PI_2)
			ang = vecAccel2.x;

			vecAccel2.x = -vecAccel2.y;
			vecAccel2.y = ang;
			vecAccel2 *= par.tangentialAccel;

			par.velocity += (vecAccel + vecAccel2) * elapsed.asSeconds();
			par.velocity.y += par.gravity * elapsed.asSeconds();
			par.location += par.velocity * elapsed.asSeconds();
			par.spin += par.spinDelta * elapsed.asSeconds();
			par.size += par.sizeDelta * elapsed.asSeconds();
			par.color += par.colorDelta * elapsed.asSeconds();

			// Apply the newly computed data to the vertices
			setParticleVerticiesColor(particleId, par.color.toSf());
			setParticleVerticiesPosition(particleId, par.location.x * m_scale + m_tx, par.location.y * m_scale + m_ty, par.size * m_scale);

			// Continue
			++particleId;
			++itr;
		}
	}
}

bool ParticleSystem::finished() const
{
	if (m_info.lifetime() > 0.0f)
		return m_age >= m_info.lifetime() && m_particles.empty();

	return static_cast<int>(m_age) > 1.f && m_particles.empty() && m_done;
}

void ParticleSystem::draw(sf::RenderTarget& target, sf::RenderStates states) const
{
	// Off by default
	if (m_info.addBlend())
		states.blendMode = sf::BlendAdd;

	states.texture = m_texture.get();
	target.draw(m_vertices.data(), m_vertices.size(), sf::Quads, states);
}

void ParticleSystem::spawnParticle()
{
	if (finished())
		return;

	ASSERT(m_particles.size() < MaxParticles);

	sf::Vertex verts[4];

	verts[0].texCoords = sf::Vector2f(m_info.textureXf(), m_info.textureYf());
	verts[1].texCoords = sf::Vector2f(m_info.textureXf() + ParticleSystemInfo::staticSize(), m_info.textureYf());
	verts[2].texCoords = sf::Vector2f(m_info.textureXf() + ParticleSystemInfo::staticSize(), ParticleSystemInfo::staticSize() + m_info.textureYf());
	verts[3].texCoords = sf::Vector2f(m_info.textureXf(), ParticleSystemInfo::staticSize() + m_info.textureYf());

	ColorRGBf parColorFloat;
	parColorFloat.r = Util::frand(m_info.colorStart().r, m_info.colorStart().r + (m_info.colorEnd().r - m_info.colorStart().r) * m_info.colorVar());
	parColorFloat.g = Util::frand(m_info.colorStart().g, m_info.colorStart().g + (m_info.colorEnd().g - m_info.colorStart().g) * m_info.colorVar());
	parColorFloat.b = Util::frand(m_info.colorStart().b, m_info.colorStart().b + (m_info.colorEnd().b - m_info.colorStart().b) * m_info.colorVar());
	parColorFloat.a = Util::frand(m_info.colorStart().a, m_info.colorStart().a + (m_info.colorEnd().a - m_info.colorStart().a) * m_info.alphaVar());

	sf::Color sfColor(parColorFloat.toSf());
	verts[0].color = sfColor;
	verts[1].color = sfColor;
	verts[2].color = sfColor;
	verts[3].color = sfColor;

	verts[0].position = m_location;
	verts[1].position = sf::Vector2f(m_location.x + ParticleSystemInfo::staticSize(), m_location.y);
	verts[2].position = sf::Vector2f(m_location.x + ParticleSystemInfo::staticSize(), m_location.y + ParticleSystemInfo::staticSize());
	verts[3].position = sf::Vector2f(m_location.x, m_location.y + ParticleSystemInfo::staticSize());

	m_vertices.push_back(verts[0]);
	m_vertices.push_back(verts[1]);
	m_vertices.push_back(verts[2]);
	m_vertices.push_back(verts[3]);

	const float angle = (rand() % 360) * 3.14f / 180.f;
	const float speed = (rand() % 50) + 50.f;

	Particle par;
	par.age = 0.0f;
	par.terminalAge = Util::frand(m_info.particleLifeMin(), m_info.particleLifeMax());
	par.location = m_location;// m_prevLocation + (m_location - m_prevLocation) * Util::frand(0.0f, 1.0f);
	par.location.x += Util::frand(-2.0f, 2.0f);
	par.location.y += Util::frand(-2.0f, 2.0f);
	par.color = parColorFloat;
	
	float ang = m_info.direction() - M_PI_2 + Util::frand(0, m_info.spread()) - m_info.spread() / 2.0f;

	if (m_info.relative())
	{
		const auto newPos = m_prevLocation - m_location;		
		ang += atan2f(newPos.y, newPos.x) + M_PI_2;
	}

	par.velocity.x = cosf(ang);
	par.velocity.y = sinf(ang);
	par.velocity *= Util::frand(m_info.speedMin(), m_info.speedMax());
	par.gravity = Util::frand(m_info.gravityMin(), m_info.gravityMax());
	par.radialAccel = Util::frand(m_info.radialAccelMin(), m_info.radialAccelMax());
	par.tangentialAccel = Util::frand(m_info.tangentialAccelMin(), m_info.tangentialAccelMax());
	par.size = Util::frand(m_info.sizeStart(), m_info.sizeStart() + (m_info.sizeEnd() - m_info.sizeStart()) * m_info.sizeVar());
	par.sizeDelta = (m_info.sizeEnd() - par.size) / par.terminalAge;
	par.spin = Util::frand(m_info.spinStart(), m_info.spinStart() + (m_info.spinEnd() - m_info.spinStart()) * m_info.spinVar());
	par.spinDelta = (m_info.spinEnd() - par.spin) / par.terminalAge;
	par.colorDelta.r = (m_info.colorEnd().r - par.color.r) / par.terminalAge;
	par.colorDelta.g = (m_info.colorEnd().g - par.color.g) / par.terminalAge;
	par.colorDelta.b = (m_info.colorEnd().b - par.color.b) / par.terminalAge;
	par.colorDelta.a = (m_info.colorEnd().a - par.color.a) / par.terminalAge;

	m_particles.push_back(par);

	ASSERT(m_particles.size() * 4 == m_vertices.size());
}

void ParticleSystem::moveParticle(const int particleId, const sf::Vector2f& adjustment)
{
	const size_t vertIdx = particleId * 4;
	ASSERT(vertIdx + 3 < m_vertices.size());

	sf::Vertex* arry = &m_vertices[vertIdx];

	arry[0].position += adjustment;
	arry[1].position += adjustment;
	arry[2].position += adjustment;
	arry[3].position += adjustment;
}

void ParticleSystem::setParticleVerticiesColor(const int particleId, const sf::Color col)
{
	const size_t vertIdx = particleId * 4;
	ASSERT(vertIdx + 3 < m_vertices.size());

	sf::Vertex* arry = &m_vertices[vertIdx];

	arry[0].color = col;
	arry[1].color = col;
	arry[2].color = col;
	arry[3].color = col;
}

void ParticleSystem::eraseParticleVerts(const int particleId)
{
	const size_t vertIdx = particleId * 4;
	ASSERT(vertIdx + 3 < m_vertices.size());

	// Erase four elements from the starting point onward
	auto itr = m_vertices.begin() + vertIdx;
	m_vertices.erase(itr, itr + 4);
}

void ParticleSystem::setParticleVerticiesPosition(const int particleId, const float x, const float y, const float scale /*= 1.0f*/)
{
	const size_t vertIdx = particleId * 4;
	ASSERT(vertIdx + 3 < m_vertices.size());

	const float width = ParticleSystemInfo::staticSize() * scale;
	const float height = ParticleSystemInfo::staticSize() * scale;

	sf::Vertex* arry = &m_vertices[vertIdx];

	// Also center the sprite
	arry[0].position = { x - (width / 2.0f), y - (height / 2.0f) };
	arry[1].position = { x + width - (width / 2.0f), y - (height / 2.0f) };
	arry[2].position = { x + width - (width / 2.0f), y + height - (height / 2.0f) };
	arry[3].position = { x - (width / 2.0f), y + height - (height / 2.0f) };
}

void ParticleSystem::normalizeVector(sf::Vector2f& v)
{
	const float dot = v.x * v.x + v.y * v.y;
	const float rc = invSqrt(dot); 

	v.x *= rc; 
	v.y *= rc;
}

// Google math
float ParticleSystem::invSqrt(float x)
{
	union {
		int intPart;
		float floatPart;
	} convertor;

	convertor.floatPart = x;
	convertor.intPart = 0x5f3759df - (convertor.intPart >> 1);
	return convertor.floatPart*(1.5f - 0.4999f * x * convertor.floatPart * convertor.floatPart);
}