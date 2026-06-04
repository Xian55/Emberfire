#pragma once

#include <thread>

class Sprite;
class SpriteAnimation;

class SpriteAnimData
{
	public:
		SpriteAnimData();
		~SpriteAnimData();

		bool loaded() const { return m_loaded; }
		bool parse(const string& filepath);
		bool loadedEnough() const;
		
		void load();
		void unload();
		void render(SpriteAnimation& owner, sf::Vector2i pos);
		
		int getSize() const { return m_size; }
		int getRatio() const { return m_ratio; }

		float durationInSeconds() const { return m_duration; }

	private:
		bool m_loading;
		bool m_loaded;

		thread m_loadingThread;

		int m_size;
		int m_ratio;
		int m_numLoadedFrames;

		clock_t m_frameLoadTime;
		
		float m_delay;
		float m_duration;

		size_t m_loopEnd;
		size_t m_loopStart;

		vector<string> m_textureNames;
		vector<sf::Vector2i> m_offsets;
		vector<shared_ptr<Sprite>> m_sprites;
};

class SpriteAnimation
{
	friend class SpriteAnimData;

	public:
		SpriteAnimation(shared_ptr<SpriteAnimData> data); 
		~SpriteAnimation();

		void stop();
		void render(const sf::Vector2i& pos);
		void setForceLoop(const bool v) { m_forceLoop = v; }
		void setColor(const sf::Color c) { m_color = c; }
		void setBlendMode(const sf::BlendMode bm) { m_blendMode = bm; }
		void load();

		bool loadedEnough() const { return m_data->loadedEnough(); }
		bool finished() const { return m_finished; }

		int getSize() const;

		float durationInSeconds() const;

	private:
		bool m_finished;
		bool m_forceLoop;

		sf::Color m_color;
		float m_timer;
		size_t m_currentFrame;
		sf::BlendMode m_blendMode;
		shared_ptr<SpriteAnimData> m_data;
};

