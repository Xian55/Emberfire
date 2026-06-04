#include "stdafx.h"
#include "ModelScript.h"
#include "ContentMgr.h"

#include <sstream>
#include <assert.h>

ModelScript::ModelScript()
{

}

ModelScript::~ModelScript()
{

}

bool ModelScript::parse(const vector<string>& lines)
{
	if (lines.empty())
		return false;

	auto getRemainStr = [](const string& str, const size_t idx)
	{
		if (idx >= str.size())
			return string();
		
		return str.substr(idx, str.size() - idx);
	};

	auto setCurrent = [](UnitDefines::AnimId& current, const string& line, const string& key, const UnitDefines::AnimId result)
	{
		if (line == key)
		{
			current = result;
			return true;
		}

		return false;
	};
		
	auto getVal = [&](const string& line, const string& key)
	{
		for (size_t i = 0; i < line.size() && i < key.size(); ++i)
		{
			if (line[i] != key[i])
				return unique_ptr<string>();
		}

		if (line.size() > key.size())
			return make_unique<string>(getRemainStr(line, key.size()));

		return unique_ptr<string>();
	};

	auto current = UnitDefines::AnimId::Stance;
	size_t numFrames = 0;

	// Fill in the data as we go along, then when we finish compiling this animation, we make another and start over.
	FlareAnimation a;

	for (size_t i = 0; i < lines.size(); ++i)
	{
		const auto& line = lines[i];

		if (auto stringPtr = getVal(line, "image="))
			m_textureName = *stringPtr;

		// The number of frames, not to be confused with an individual frame
		if (auto stringPtr = getVal(line, "frames="))
		{
			numFrames = atoi(stringPtr->c_str());
			a.m_frames.resize(numFrames);

			for (auto& itr : a.m_frames)
				itr.resize(UnitDefines::NumDirections);
		}

		if (auto stringPtr = getVal(line, "duration="))
			a.m_durationInSeconds = static_cast<float>(atoi(stringPtr->c_str())) / 1000.0f;
		
		// An individual frame, not to be confused with the number of frames
		if (auto stringPtr = getVal(line, "frame="))
		{
			std::stringstream ss(*stringPtr);

			int i;
			vector<int> numbers;

			while (ss >> i)
			{
				numbers.push_back(i);

				if (ss.peek() == ',')
					ss.ignore();
			}

			if (numbers.size() == 8)
			{
				const size_t frameId = numbers[0];
				
				if (frameId >= numFrames)
				{
					blog(Logger::LOG_ERROR, "Bad frame id when parsing npc model script.");
					return false;
				}

				auto dir = static_cast<UnitDefines::Direction>(numbers[1]);

				auto frame = &a.m_frames[frameId][dir];
				ASSERT(frame->renderOffset.x == 0);

				frame->spriteRect = sf::IntRect(numbers[2], numbers[3], numbers[4], numbers[5]);
				frame->renderOffset.x = numbers[6];
				frame->renderOffset.y = numbers[7];
			}
			else
			{
				blog(Logger::LOG_ERROR, "Bad frame data when parsing npc model script.");
				return false;
			}
		}

		if (auto stringPtr = getVal(line, "type="))
		{
			if (*stringPtr == "play_once")
				a.m_type = FlareAnimation::PlayOnce;
			else if (*stringPtr == "looped")
				a.m_type = FlareAnimation::Loop;
			else if (*stringPtr == "back_forth")
				a.m_type = FlareAnimation::BackForth;
			else
				return false;
		}

		// Last line of the line is also the end of an animation, the end of the last one.
		bool animDone = i == lines.size() - 1;
		const auto currentBefore = current;

		if (setCurrent(current, line, "[stance]", UnitDefines::AnimId::Stance) ||
			setCurrent(current, line, "[spawn]", UnitDefines::AnimId::Spawn) ||
			setCurrent(current, line, "[shoot]", UnitDefines::AnimId::Shoot) ||
			setCurrent(current, line, "[run]", UnitDefines::AnimId::Run) ||
			setCurrent(current, line, "[die]", UnitDefines::AnimId::Die) ||
			setCurrent(current, line, "[critdie]", UnitDefines::AnimId::CritDie) ||
			setCurrent(current, line, "[cast]", UnitDefines::AnimId::Cast) ||
			setCurrent(current, line, "[swing]", UnitDefines::AnimId::Swing) ||
			setCurrent(current, line, "[hit]", UnitDefines::AnimId::Hit) ||
			setCurrent(current, line, "[block]", UnitDefines::AnimId::Block) ||
			setCurrent(current, line, "[stance]", UnitDefines::AnimId::Stance) ||
			setCurrent(current, line, "[cast_alt]", UnitDefines::AnimId::CastAlt))
		{
			animDone = a.numFrames() > 0;
		}
		else if (line.size() > 0 && line[0] == '[')
		{
			blog(Logger::LOG_ERROR, "Bad anim name %s when parsing npc model script.", line.c_str());
			return false;
		}

		if (animDone)
		{
			a.m_id = currentBefore;
			m_animations[currentBefore] = a;
			a = FlareAnimation{};
		}
	}
	
	// If these are empty then what did we even just do?
	return m_animations.size() > 0 && !m_textureName.empty();
}

FlareAnimation* ModelScript::getAnim(const UnitDefines::AnimId id)
{
	auto itr = m_animations.find(id);

	if (itr == m_animations.end())
		return nullptr;

	return &itr->second;
}
