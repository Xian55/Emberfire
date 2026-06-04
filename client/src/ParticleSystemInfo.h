#pragma once

#include "ColorRGBf.h"

class ParticleSystemInfo
{
	public:
		ParticleSystemInfo();
		~ParticleSystemInfo();

		bool loadFromDisk(const string& filePath);

		auto relative() const { return m_relative != 0; }
		auto emission() const { return m_emission; }
		auto colorStart() const { return m_colorStart; }
		auto colorEnd() const { return m_colorEnd; }
		auto lifetime() const { return m_lifetime; }
		auto particleLifeMin() const { return m_particleLifeMin; }
		auto particleLifeMax() const { return m_particleLifeMax; }
		auto direction() const { return m_direction; }
		auto spread() const { return m_spread; }
		auto speedMin() const { return m_speedMin; }
		auto speedMax() const { return m_speedMax; }
		auto gravityMin() const { return m_gravityMin; }
		auto gravityMax() const { return m_gravityMax; }
		auto radialAccelMin() const { return m_radialAccelMin; }
		auto radialAccelMax() const { return m_radialAccelMax; }
		auto tangentialAccelMin() const { return m_tangentialAccelMin; }
		auto tangentialAccelMax() const { return m_tangentialAccelMax; }
		auto sizeStart() const { return m_sizeStart; }
		auto sizeEnd() const { return m_sizeEnd; }
		auto sizeVar() const { return m_sizeVar; }
		auto spinStart() const { return m_spinStart; }
		auto spinEnd() const { return m_spinEnd; }
		auto spinVar() const { return m_spinVar; }
		auto colorVar() const { return m_colorVar; }
		auto alphaVar() const { return m_alphaVar; }
		auto textureX() const { return m_textureX; }
		auto textureY() const { return m_textureY; }
		auto textureXf() const { return static_cast<float>(m_textureX); }
		auto textureYf() const { return static_cast<float>(m_textureY); }
		auto addBlend() const { return m_addBlend; }

		static float staticSize() { return 32.0f; }

	private:
		int m_relative;
		int m_emission;
		int m_textureX;
		int m_textureY;

		float m_lifetime;
		float m_particleLifeMin;
		float m_particleLifeMax;
		float m_direction;
		float m_spread;
		float m_speedMin;
		float m_speedMax;
		float m_gravityMin;
		float m_gravityMax;
		float m_radialAccelMin;
		float m_radialAccelMax;
		float m_tangentialAccelMin;
		float m_tangentialAccelMax;
		float m_sizeStart;
		float m_sizeEnd;
		float m_sizeVar;
		float m_spinStart;
		float m_spinEnd;
		float m_spinVar;
		float m_colorVar;
		float m_alphaVar;

		bool m_addBlend;

		ColorRGBf m_colorStart;
		ColorRGBf m_colorEnd;
};

