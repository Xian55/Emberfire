#include "stdafx.h"
#include "ParticleSystemInfo.h"
#include "..\Shared\StlBuffer.h"

ParticleSystemInfo::ParticleSystemInfo() :
	m_relative(false),
	m_emission(0),
	m_lifetime(0),
	m_particleLifeMin(0),
	m_particleLifeMax(0),
	m_direction(0),
	m_spread(0),
	m_speedMin(0),
	m_speedMax(0),
	m_gravityMin(0),
	m_gravityMax(0),
	m_radialAccelMin(0),
	m_radialAccelMax(0),
	m_tangentialAccelMin(0),
	m_tangentialAccelMax(0),
	m_sizeStart(0),
	m_sizeEnd(0),
	m_sizeVar(0),
	m_spinStart(0),
	m_spinEnd(0),
	m_spinVar(0),
	m_colorVar(0),
	m_alphaVar(0),
	m_addBlend(true)
{

}

ParticleSystemInfo::~ParticleSystemInfo()
{

}

bool ParticleSystemInfo::loadFromDisk(const string& filePath)
{
	StlBuffer data;
	
	if (!data.readFile(filePath))
		return false;

	int spriteData;
	data >> spriteData;

	m_addBlend = spriteData >> 16 != 6;
	m_textureX = (32 * ((spriteData & 0xFFFF) % 4));
	m_textureY = (32 * ((spriteData & 0xFFFF) / 4));

	data >> m_emission;
	data >> m_lifetime;
	data >> m_particleLifeMin;
	data >> m_particleLifeMax;
	data >> m_direction;
	data >> m_spread;
	data >> m_relative;
	data >> m_speedMin;
	data >> m_speedMax;
	data >> m_gravityMin;
	data >> m_gravityMax;
	data >> m_radialAccelMin;
	data >> m_radialAccelMax;
	data >> m_tangentialAccelMin;
	data >> m_tangentialAccelMax;
	data >> m_sizeStart;
	data >> m_sizeEnd;
	data >> m_sizeVar;
	data >> m_spinStart;
	data >> m_spinEnd;
	data >> m_spinVar;

	data >> m_colorStart.r;
	data >> m_colorStart.g;
	data >> m_colorStart.b;
	data >> m_colorStart.a;

	data >> m_colorEnd.r;
	data >> m_colorEnd.g;
	data >> m_colorEnd.b;
	data >> m_colorEnd.a;

	data >> m_colorVar;
	data >> m_alphaVar;

	return true;
}
