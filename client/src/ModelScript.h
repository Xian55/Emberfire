#pragma once

#include "..\Shared\UnitDefines.h"

class ContentMgr;

class FlareAnimation
{
	friend class ModelScript;

	struct Frame
	{
		sf::Vector2i renderOffset;
		sf::IntRect spriteRect;
	};

	public:
		enum AnimType
		{
			Loop,
			PlayOnce,
			BackForth,
		};

	public:
		FlareAnimation() :
			m_durationInSeconds(0.f),
			m_type(Loop),
			m_id(UnitDefines::AnimId::Stance)
		{}

		const auto& durationInSeconds() const { return m_durationInSeconds; }
		const auto& type() const { return m_type; }
		const auto& id() const { return m_id; }

		const auto numFrames() const { return m_frames.size(); }

		Frame* getFrame(const int frameId, const UnitDefines::Direction dir)
		{
			if (static_cast<size_t>(frameId) >= numFrames() || dir >= UnitDefines::NumDirections)
				return nullptr;

			// Model-script per-direction frames are stored rotated 90° (2 of 8 dirs) from
			// the Direction enum, so a unit's rendered facing lagged its heading by 90°.
			// Remap the lookup so the displayed sprite matches the actual direction.
			const int sdir = (static_cast<int>(dir) + 2) % UnitDefines::NumDirections;
			return &m_frames[frameId][sdir];
		}

	private:
		AnimType m_type;
		UnitDefines::AnimId m_id;
		float m_durationInSeconds;

		// [frameId][direction]
		vector<vector<Frame>> m_frames;
};

class ModelScript
{			
	public:
		ModelScript();
		~ModelScript();

		const string& getTextureName() const { return m_textureName; }

		bool parse(const vector<string>& lines);

		FlareAnimation* getAnim(const UnitDefines::AnimId id);

	private:
		string m_textureName;
		unordered_map<UnitDefines::AnimId, FlareAnimation> m_animations;
};

