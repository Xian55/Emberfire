#pragma once

#include <mutex>

#include "..\Shared\PlayerDefines.h"

class Sprite;
class ButtonScript;
class SpriteScript;
class ModelScript;
class SpriteAnimData;
class SpriteAnimation;
class DatFile;
class ParticleSystemInfo;
class QueryResult;
class SqlConnector;
class ParticleSystem;
class Tooltip;

class ContentMgr
{
	public:
		void update();
		void init();
		void loadButtonScripts();
		void loadSpriteRenderScripts();
		void loadSpriteAnimations();
		void loadDbTables();
		void loadDbTablesDev();
		void loadPsi();
		void loadFootsteps();
		void loadDbTable(shared_ptr<SqlConnector> db, const string& tableName);
		void loadDbTableEntry(shared_ptr<SqlConnector> db, const string& tableName, const string& entry);
		void loadSmoothTextureNames();
		void loadPortraitOffsets();
		void loadStatDescriptions();
		void reload();
		void onAudioDeviceChanged();

		// Textures not being used anywhere will be unloaded
		void dumpTextures();

		// The sounds need to be contained somewhere to keep playing.
		// There is also a limit to how many can be playing at once in SFML.
		// The volume is a value between 0 (mute) and 1 (full volume).
		shared_ptr<SfSoundSafe> playSound(const string& filename, const float volume = 1.f, sf::Vector2f screenPosition = { -1.f, -1.f });

		void stopAllLoopSound();
		void queueSound(const string& filename, const clock_t delayMiliseconds, const float vol = 1.f);
		void getFootstepsForTexture(const string& texture, vector<string>& soundEffects) const;
		void setMusicPlaylist(const vector<string>& filenames);
		void loopMusic(const string& filename, const bool refreshCurrent = true);
		void loopAmbience(const string& filename);
		void queueMusicTransition(const string& filename);
		void queueAmbienceTransition(const string& filename);
		void stopMusic();
		void stopAmbience();
		
		bool isFootstepsForTexture(const string& texture) const;
		bool isPsiExist(const string& filename) const;
		bool isTextureExist(const string& filename) const;
		bool isTextureLoaded(const string& filename);
		bool queueTextureLoad(const string& str);
		bool compareMusicPlaylist(const vector<string>& playlist) const;
		bool compareAmbienceTrack(const string& ambience) const;

		int getPortraitOffset(const Sprite& spr) const;
		
		auto& getBrightShader() { return m_brightShader; }
		auto& getRecolorShader() { return m_recolorShader; }

		shared_ptr<sf::Texture> getTexture(const string& filename);
		shared_ptr<sf::Music> getMusic(const string& filename);
		shared_ptr<sf::SoundBuffer> getSoundBuffer(const string& filename);
		shared_ptr<sf::Font> getFont(const string& fontname);
		shared_ptr<SpriteScript> getSpriteRenderScript(const string& textureName) const;
		shared_ptr<SpriteScript> ensureSpriteScript(const string& filename);
		shared_ptr<Sprite> spawnSprite(const string& filename);
		shared_ptr<Sprite> spawnPortrait(const string& filename);
		shared_ptr<Sprite> spawnPlayerPortrait(int id, int gender);
		shared_ptr<ButtonScript> buttonScript(const string& scriptName) const;		
		shared_ptr<SpriteAnimation> spawnSpriteAnimation(const string& name);
		shared_ptr<Tooltip> getStatTooltip(const PlayerDefines::Classes c, const int stat) const;

		unique_ptr<ModelScript> spawnModelScript(const string& scriptName);
		unique_ptr<ParticleSystem> spawnParticleSystem(const string& scriptName);
		
		QueryResult const& db(const string& tablename);
		QueryResult& editableDb(const string& tablename);

	public:
		static ContentMgr* instance()
		{
			static ContentMgr selfInstance;
			return &selfInstance;
		}

	private:
		ContentMgr();
		~ContentMgr();
		
		void loadingThread();
		void joinModelThreads();
		void loadModelScripts();
		void playMusic(const string& filename);
		void documentZips();

		// Returns true only when its time to play music
		bool updateTransitionTimer(float& timer, unique_ptr<string>& filename) const;

		// After using, cleanup output with delete[]
		//	Returns true on success
		bool unzip(const string& filename, char*& output, unsigned int& outputSize);

		shared_ptr<SfSoundSafe> spawnSound(const string& filename);

		int getSfxVolume() const;
		int getMusicVolume() const;
		float getAmbienceVolume() const;

		float calcTranstionVol(const float& transitionTimer) const;

		bool m_done{false};
		
		float m_transitionMusic{-1.f};
		float m_transitionAmbience{-1.f};

		sf::Shader m_brightShader;
		sf::Shader m_recolorShader;
				
		mutex m_mutex;
		thread m_loadingThread;
		size_t m_musicPlaylistIdx;
		string m_currentAmbienceName;
		clock_t m_musicFinishDate;
		clock_t m_lastTextureDump;

		shared_ptr<sf::Music> m_currentMusic;
		shared_ptr<sf::Music> m_currentAmbience;

		unique_ptr<string> m_transitionMusicStr;
		unique_ptr<string> m_transitionAmbienceStr;

		set<string> m_queuedLoads;
		set<string> m_smoothTextures;
		
		vector<string> m_zips;
		vector<shared_ptr<SfSoundSafe>> m_sounds;
		vector<string> m_musicPlaylist;
		vector<thread> m_modelScriptThreads;

		struct QueuedSound
		{
			string filename;
			clock_t timems{0};
			float vol{1.f};
		};

		vector<QueuedSound> m_queuedSounds;

		map<string, bool> m_dummyTextures;
		map<PlayerDefines::Classes, map<int, shared_ptr<Tooltip>>> m_statTooltips;
		map<string, shared_ptr<ButtonScript>> m_buttonScripts;
		map<string, shared_ptr<sf::Texture>> m_resourcesImages;
		map<string, shared_ptr<sf::Music>> m_resourcesMusics;
		map<string, shared_ptr<sf::SoundBuffer>> m_resourceSoundBuffers;
		map<string, shared_ptr<sf::Font>> m_resourceFonts;
		map<string, shared_ptr<SpriteScript>> m_spriteRenderScripts;
		map<string, shared_ptr<ModelScript>> m_modelScript;
		map<string, shared_ptr<SpriteAnimData>> m_spriteAnimations;
		map<string, shared_ptr<QueryResult>> m_clientdat;
		map<string, unique_ptr<ParticleSystemInfo>> m_psi;
		map<string, vector<string>> m_footsteps;
		map<string, int> m_portraitOffsets;

		// Example would be .png][.zip
		map<string, string> m_zipContents;
};

#define sContentMgr ContentMgr::instance()