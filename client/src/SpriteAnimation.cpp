#include "stdafx.h"
#include "SpriteAnimation.h"
#include "..\StringHelpers.h"
#include "..\Files.h"
#include "Application.h"
#include "ContentMgr.h"
#include "Sprite.h"

#include <assert.h>

/**
* SpriteAnimation
*/

SpriteAnimation::SpriteAnimation(shared_ptr<SpriteAnimData> data) :
	m_currentFrame(0),
	m_timer(0.0f),
	m_finished(false),
	m_forceLoop(false),
	m_color(sf::Color::White),
	m_data(data),
	m_blendMode(sf::BlendAlpha)
{
	ASSERT(m_data != nullptr);
}

SpriteAnimation::~SpriteAnimation()
{
	if (m_data.use_count() <= 2)
		m_data->unload();
}

void SpriteAnimation::stop()
{
	m_finished = true;
}

int SpriteAnimation::getSize() const
{
	return m_data->getSize() / m_data->getRatio();
}

void SpriteAnimation::load()
{
	if (!m_data->loaded())
		m_data->load();
}

void SpriteAnimation::render(const sf::Vector2i& pos)
{
	if (!m_data->loaded())
		m_data->load();

	if (!m_data->loadedEnough())
		return;

	m_data->render(*this, pos);
}

float SpriteAnimation::durationInSeconds() const
{
	return m_data->durationInSeconds();
}

/**
* SpriteAnimData
*/
		
SpriteAnimData::SpriteAnimData() :
	m_loopStart(0),
	m_size(0),
	m_loopEnd(0),
	m_delay(0.0f),
	m_ratio(1),
	m_loading(false),
	m_loaded(false),
	m_numLoadedFrames(0),
	m_frameLoadTime(0),
	m_duration(0)
{

}
	
SpriteAnimData::~SpriteAnimData()
{

}

bool SpriteAnimData::loadedEnough() const
{
	if (m_numLoadedFrames < 2 && m_sprites.size() >= 2)
		return false;

	if (m_frameLoadTime <= 0)
		return true;

	if (m_loaded)
		return true;

	int numFramesLeftToBuffer = int(m_sprites.size()) - m_numLoadedFrames;
	
	float animDuration = m_delay * float(m_sprites.size());
	float frameDuration = m_delay;

	float secondsBuffered = frameDuration * float(m_numLoadedFrames);
	float secondsLeftToBuffer = animDuration - secondsBuffered;
	
	float secondsItTakesToBufferOneFrame = float(m_frameLoadTime) / 1000.f;
	
	// If loading super fast, don't believe it until we've seen at least 5 frames
	if (secondsItTakesToBufferOneFrame < m_delay * 1.25f && m_numLoadedFrames < 5)
		secondsItTakesToBufferOneFrame = m_delay * 1.25f;

	float secondsItTakesToFinishBuffering = secondsItTakesToBufferOneFrame * float(numFramesLeftToBuffer);
	   	
	return secondsItTakesToFinishBuffering - secondsBuffered < secondsLeftToBuffer;
}

void SpriteAnimData::load()
{
	if (m_loading || m_loaded)
		return;

	if (m_loadingThread.joinable())
		m_loadingThread.join();

	m_loading = true;

	auto loadingFunc = [&]()
	{
		for (size_t i = 0; i < m_sprites.size() && i < m_textureNames.size() && m_loading; ++i)
		{
			const string& texname = m_textureNames[i];
			clock_t ttbefore = std::clock();

			if (m_sprites[i] == nullptr)
			{
				// Wait for it to be loaded, or if it was loaded then we can just spawn a sprite
				while (!sContentMgr->isTextureLoaded(texname))
				{
					sContentMgr->queueTextureLoad(texname);
					std::this_thread::sleep_for(std::chrono::milliseconds(1));

					if (!m_loading)
						return;
				}

				m_sprites[i] = sContentMgr->spawnSprite(texname);
			}

			++m_numLoadedFrames;
			m_frameLoadTime = max(std::clock() - ttbefore, m_frameLoadTime);
		}

		m_loaded = true;
		m_loading = false;
	};

	m_loadingThread = thread(loadingFunc);
}

void SpriteAnimData::unload()
{
	m_loading = false;

	if (m_loadingThread.joinable())
		m_loadingThread.join();

	for (size_t i = 0; i < m_sprites.size(); ++i)
		m_sprites[i] = nullptr;

	m_frameLoadTime = 0;
	m_numLoadedFrames = 0;
	m_loaded = false;
}

void SpriteAnimData::render(SpriteAnimation& owner, sf::Vector2i pos)
{
	if (m_sprites.empty())
		return;

	owner.m_timer += sApplication->delta();

	if (owner.m_timer > m_delay)
	{
		++owner.m_currentFrame;
		owner.m_timer = 0.0f;

		const size_t loopEnd = owner.m_forceLoop ? (m_loopEnd ? m_loopEnd : m_sprites.size()) : m_loopEnd;
		
		if (owner.m_currentFrame >= loopEnd && loopEnd != 0)
			owner.m_currentFrame = m_loopStart > 0 ? m_loopStart - 1 : 0;
	}

	if (owner.m_currentFrame >= m_sprites.size())
	{
		owner.m_finished = true;
		return;
	}

	ASSERT(owner.m_currentFrame < m_offsets.size());
	ASSERT(owner.m_currentFrame < m_textureNames.size());

	auto spr = m_sprites[owner.m_currentFrame];
	
	if (spr == nullptr)
		return;

	if (spr->getGlobalBounds().height == 1 && spr->getGlobalBounds().width == 1)
		return;
	
	if (spr->getColor() != sf::Color(owner.m_color))
		spr->setColor(sf::Color(owner.m_color));

	pos += m_offsets[owner.m_currentFrame] / m_ratio;
	pos -= sf::Vector2i(m_size, m_size) / m_ratio;

	spr->setBlendMode(owner.m_blendMode);
	spr->render(sf::Vector2f(pos));
}

bool SpriteAnimData::parse(const string& filepath)
{
	if (!m_textureNames.empty() || !m_offsets.empty())
		return false;

	vector<string> lines;
	Util::readLinesFromFile(filepath, lines);

	if (lines.size() < 8)
		return false;

	if (lines[0].size() < 7)
		return false;

	if (lines[1].size() < 6)
		return false;

	if (lines[2].size() < 10)
		return false;

	if (lines[3].size() < 11)
		return false;

	if (lines[4].size() < 9)
		return false;

	if (lines[5].size() < 7)
		return false;

	const string ratioStr = lines[0].substr(6, lines[0].size() - 6);
	const string sizeStr = lines[1].substr(5, lines[1].size() - 5);
	const string filenameStr = lines[2].substr(9, lines[2].size() - 9);
	const string loopstartStr = lines[3].substr(10, lines[3].size() - 10);
	const string loopendStr = lines[4].substr(8, lines[4].size() - 8);
	const string delayStr = lines[5].substr(6, lines[5].size() - 6);
	
	for (size_t i = 7; i < lines.size(); ++i)
	{
		vector<int> str;
		Util::splitStr(lines[i].c_str(), str, ',');

		if (str.size() != 3)
			return false;

		m_textureNames.push_back(filenameStr + "_" + to_string(str[0]) + ".png");
		m_offsets.push_back({ str[1], str[2] });
	}

	m_ratio = atoi(ratioStr.c_str());
	m_size = atoi(sizeStr.c_str());
	m_loopStart = static_cast<size_t>(atoi(loopstartStr.c_str()));
	m_loopEnd = static_cast<size_t>(atoi(loopendStr.c_str()));
	m_delay = static_cast<float>(atof(delayStr.c_str())) / 1000.0f;

	const bool result = m_loopStart <= m_textureNames.size() && m_loopEnd <= m_textureNames.size();

	if (!result)
		return false;

	m_sprites.resize(m_textureNames.size(), nullptr);
	m_duration = m_delay * float(m_textureNames.size());
	return true;
}
