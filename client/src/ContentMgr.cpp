#include "stdafx.h"
#include "ContentMgr.h"
#include "Sprite.h"
#include "ButtonScript.h"
#include "SpriteScript.h"
#include "ModelScript.h"
#include "SpriteAnimation.h"
#include "ParticleSystem.h"
#include "ParticleSystemInfo.h"
#include "Application.h"
#include "Options.h"
#include "Tooltip.h"
#include "TextBox.h"

#include "..\Files.h"
#include "..\StringHelpers.h"
#include "..\Shared\Config.h"
#include "..\Shared\PlayerDefines.h"
#include "..\Shared\CharacterDefines.h"
#include "..\Shared\ItemDefiner.h"
#include "..\SqlConnector\SqlConnector.h"

#include <zip.h>
#include <thread>
#include <assert.h>
#include <regex>

ContentMgr::ContentMgr() : 
	m_musicFinishDate(0),
	m_musicPlaylistIdx(0),
	m_done(false),
	m_lastTextureDump(0)
{

}

ContentMgr::~ContentMgr()
{
	m_done = true;
	m_loadingThread.join();
	joinModelThreads();
}

void ContentMgr::init()
{
	m_loadingThread = thread(&ContentMgr::loadingThread, this);
	documentZips();
	reload();
}

void ContentMgr::onAudioDeviceChanged()
{
	lock_guard<mutex> lock(m_mutex);

	// Capture music state
	string musicName;
	sf::Time musicOffset;
	bool musicWasPlaying = m_currentMusic && m_currentMusic->getStatus() == sf::Music::Playing;
	if (musicWasPlaying) {
		musicOffset = m_currentMusic->getPlayingOffset();
		for (auto& [k, v] : m_resourcesMusics) {
			if (v == m_currentMusic) {
				musicName = k;
				break;
			}
		}
	}

	// Capture ambience state (you already have m_currentAmbienceName)
	string ambienceName = m_currentAmbienceName;
	sf::Time ambienceOffset;
	bool ambienceWasPlaying = m_currentAmbience && m_currentAmbience->getStatus() == sf::Music::Playing;
	if (ambienceWasPlaying)
		ambienceOffset = m_currentAmbience->getPlayingOffset();

	// Kill all sounds
	for (auto& s : m_sounds)
		s->stop();
	m_sounds.clear();
	m_queuedSounds.clear();

	// Kill all music
	m_currentMusic.reset();
	m_currentAmbience.reset();
	m_resourcesMusics.clear();

	// Kill all buffers
	m_resourceSoundBuffers.clear();

	// Cancel pending transitions
	m_transitionMusicStr.reset();
	m_transitionAmbienceStr.reset();
	m_transitionMusic = -1.f;
	m_transitionAmbience = -1.f;

	// Let OpenAL die
	sf::sleep(sf::milliseconds(50));

	// Restore music
	if (musicWasPlaying && !musicName.empty()) {
		loopMusic(musicName, false);
		if (m_currentMusic)
			m_currentMusic->setPlayingOffset(musicOffset);
	}

	// Restore ambience
	if (ambienceWasPlaying && !ambienceName.empty()) {
		loopAmbience(ambienceName);
		if (m_currentAmbience)
			m_currentAmbience->setPlayingOffset(ambienceOffset);
	}
}

void ContentMgr::reload()
{
	dumpTextures();
	loadModelScripts();
	loadButtonScripts();
	loadSpriteRenderScripts();
	loadSpriteAnimations();
	loadDbTables();
	loadPsi();
	loadSmoothTextureNames();
	loadFootsteps();
	loadPortraitOffsets();
	loadStatDescriptions();
}

void ContentMgr::update()
{
	if (std::clock() - m_lastTextureDump > 30000)
	{
		sContentMgr->dumpTextures();
		m_lastTextureDump = std::clock();
	}

	if (m_currentMusic != nullptr)
	{
		m_currentMusic->setVolume(static_cast<float>(getMusicVolume()) * calcTranstionVol(m_transitionMusic));

		if (std::clock() > m_musicFinishDate && !m_musicPlaylist.empty())
		{
			if (++m_musicPlaylistIdx >= m_musicPlaylist.size())
				m_musicPlaylistIdx = 0;

			playMusic(m_musicPlaylist[m_musicPlaylistIdx]);
		}
	}

	if (m_currentAmbience != nullptr)
		m_currentAmbience->setVolume(getAmbienceVolume() * calcTranstionVol(m_transitionAmbience));

	for (auto itr = m_queuedSounds.begin(); itr != m_queuedSounds.end();)
	{
		if (std::clock() > itr->timems)
		{
			playSound(itr->filename, itr->vol);
			itr = m_queuedSounds.erase(itr);
		}
		else
		{
			++itr;
		}
	}

	// When the sound effect finishes, its status updates inside of SFML
	if (m_sounds.size() > 0)
	{
		auto itr = m_sounds.begin();

		while (itr != m_sounds.end())
		{
			auto& data = (*itr);

			if (data->getStatus() != sf::Sound::Playing)
			{
				itr = m_sounds.erase(itr);
			}
			else
			{
				data->setVolume(static_cast<float>(getSfxVolume()) * data->getRememberedVolume());
				++itr;
			}
		}
	}

	if (updateTransitionTimer(m_transitionMusic, m_transitionMusicStr))
	{
		playMusic(*m_transitionMusicStr);
		m_transitionMusicStr = nullptr;
	}

	if (updateTransitionTimer(m_transitionAmbience, m_transitionAmbienceStr))
	{
		loopAmbience(*m_transitionAmbienceStr);
		m_transitionAmbienceStr = nullptr;
	}
}
		
bool ContentMgr::updateTransitionTimer(float& timer, unique_ptr<string>& filename) const
{
	if (timer != -1)
	{
		timer -= sApplication->delta() / 5.f;

		// Finished fading out
		if (timer <= 0 && filename != nullptr)
			return true;

		// Finished fading in
		if (timer <= -1.f)
			timer = -1.f;
	}

	return false;
}

float ContentMgr::calcTranstionVol(const float& transitionTimer)  const
{
	float transitionVol = 1.f;

	if (transitionTimer != -1.f)
	{
		if (transitionTimer > 0)
			transitionVol = transitionTimer;
		else
			transitionVol = -transitionTimer;
	}

	return transitionVol;
}

void ContentMgr::stopAllLoopSound()
{
	for (auto& itr : m_sounds)
		itr->setLoop(false);
}

void ContentMgr::queueSound(const string& filename, const clock_t delayMilisec, const float vol)
{
	QueuedSound qs;
	qs.filename = filename;
	qs.timems = std::clock() + delayMilisec;
	qs.vol = vol;
	m_queuedSounds.push_back(qs);
}

shared_ptr<SfSoundSafe> ContentMgr::playSound(const string& filename, const float volume /*= 100*/, sf::Vector2f screenPosition /*= { -1.f, -1.f }*/)
{
	if (sApplication->isAudioDeviceChangedThisTick())
		return nullptr;

	while (m_sounds.size() >= 254)
		m_sounds.erase(m_sounds.begin());

	if (auto sound = spawnSound(filename))
	{
		if (screenPosition != sf::Vector2f{-1.f, -1.f})
		{
			if (screenPosition.x < -200 || screenPosition.y < -200 ||
				screenPosition.x > sApplication->sW() + 200 || screenPosition.y > sApplication->sH() + 200)
			{
				// Over 200 pixels off screen
				return nullptr;
			}

			if (sound->getBuffer()->getChannelCount() > 1)
				blog(Logger::LOG_ERROR, "Trying to use screen position with non-mono sound file %s", filename.c_str());

			sound->setScreenPosition(screenPosition);
		}

		sound->rememberVolume(volume);
		sound->setVolume(static_cast<float>(getSfxVolume()) * volume);
		sound->play();
		m_sounds.push_back(sound);
		return sound;
	}

	return nullptr;
}

void ContentMgr::playMusic(const string& filename)
{
	stopMusic();

	if (auto music = getMusic(filename))
	{
		m_currentMusic = music;
		m_currentMusic->setVolume(static_cast<float>(getMusicVolume()));
		m_currentMusic->play();

		auto dur = static_cast<clock_t>(music->getDuration().asMilliseconds());
		m_musicFinishDate = std::clock() + static_cast<clock_t>(music->getDuration().asMilliseconds());
	}
}

bool ContentMgr::isPsiExist(const string& filename) const
{
	return m_psi.find(filename) != m_psi.end();
}

bool ContentMgr::isTextureExist(const string& filename) const
{
	return m_zipContents.find(filename) != m_zipContents.end();
}

bool ContentMgr::isTextureLoaded(const string& filename)
{
	lock_guard<mutex> grd(m_mutex);
	return m_resourcesImages.find(filename) !=m_resourcesImages.end();
}

void ContentMgr::setMusicPlaylist(const vector<string>& filenames)
{
	bool overwrote = !m_musicPlaylist.empty();

	if (!overwrote || filenames.empty())
		stopMusic();

	m_musicPlaylist = filenames;
	m_musicPlaylistIdx = 0;

	if (m_musicPlaylist.empty())
		return;

	if (overwrote)
		queueMusicTransition(*m_musicPlaylist.begin());
	else
		playMusic(*m_musicPlaylist.begin());
}

void ContentMgr::loopMusic(const string& filename, const bool refreshCurrent /*= true*/)
{
	m_musicFinishDate = 0;
	m_musicPlaylist.clear();
	m_musicPlaylistIdx = 0;

	auto music = getMusic(filename);

	if (!refreshCurrent && music != nullptr && music == m_currentMusic)
		return;

	if (m_currentMusic != nullptr)
		m_currentMusic->stop();

	if (music != nullptr)
	{
		m_currentMusic = music;
		m_currentMusic->setVolume(static_cast<float>(getMusicVolume()));
		m_currentMusic->setLoop(true);
		m_currentMusic->play();
	}
}

void ContentMgr::stopMusic()
{
	m_musicFinishDate = 0;

	if (m_currentMusic != nullptr)
	{
		m_currentMusic->stop();
		m_currentMusic = nullptr;
	}
}

void ContentMgr::loopAmbience(const string& filename)
{
	stopAmbience();
	
	if (m_currentAmbience = getMusic(filename))
	{
		m_currentAmbienceName = filename;
		m_currentAmbience->setVolume(static_cast<float>(getAmbienceVolume()));
		m_currentAmbience->setLoop(true);
		m_currentAmbience->play();
	}
}
		
void ContentMgr::queueMusicTransition(const string& filename)
{
	// Start new transition
	if (m_transitionMusic == -1.f)
		m_transitionMusic = 1.f;
	else
		m_transitionMusic = -m_transitionMusic;

	m_transitionMusicStr = make_unique<string>(filename);
	m_musicFinishDate = std::clock() + 3000000;
}
		
void ContentMgr::queueAmbienceTransition(const string& filename)
{
	if (m_transitionAmbience == -1.f)
	{
		// If not playing, then skip to fading in
		if (m_currentAmbience == nullptr)
			m_transitionAmbience = 0.f;
		else
			m_transitionAmbience = 1.f;
	}
	else
	{
		m_transitionAmbience = -m_transitionAmbience;
	}

	m_transitionAmbienceStr = make_unique<string>(filename);
}

void ContentMgr::stopAmbience()
{
	if (m_currentAmbience != nullptr)
	{
		m_currentAmbience->stop();
		m_currentAmbience = nullptr;
	}
}

void ContentMgr::loadSpriteRenderScripts()
{
	blog(Logger::LOG_INFO, "loadSpriteRenderScripts();");

	m_spriteRenderScripts.clear();
	
	// No reason to make this a member function since it will only be used here
	auto ensureScriptExists = [&](const string& textureName)
	{
		auto itr = m_spriteRenderScripts.find(textureName);

		if (itr != m_spriteRenderScripts.end())
			return itr->second;

		return m_spriteRenderScripts[textureName] = make_shared<SpriteScript>();
	};

	shared_ptr<SqlConnector> db = SqlConnector::create(SqlType::SQLite, sConfig->getString("GameDb", "Name"), sConfig->getString("GameDb", "Path"));

	if (auto result = db->query("SELECT filename, x_offset, y_offset FROM sprite_hotspot"))
	{
		for (auto& fields : result->fetchData())
		{
			const string& textureName = fields[0].getString();
			shared_ptr<SpriteScript> sptr = ensureScriptExists(textureName);
			sptr->setHotspot(fields[1].getInt32(), fields[2].getInt32());
		}
	}

	if (auto result = db->query("SELECT filename, psi, x_offset, y_offset FROM sprite_psi"))
	{
		for (auto& fields : result->fetchData())
		{
			const string& textureName = fields[0].getString();
			shared_ptr<SpriteScript> sptr = ensureScriptExists(textureName);
			sptr->setPsi(fields[1].getString(), sf::Vector2i(fields[2].getInt32(), fields[3].getInt32()));
		}
	}

	if (auto result = db->query("SELECT filename, sound, radius FROM sprite_proximity_sound"))
	{
		for (auto& fields : result->fetchData())
		{
			const string& textureName = fields[0].getString();
			shared_ptr<SpriteScript> sptr = ensureScriptExists(textureName);
			sptr->setProximitySound(fields[1].getString());
			sptr->setPromixitySoundDistFactor(fields[2].getFloat());
		}
	}

	if (auto result = db->query("SELECT filename, color, x_offset, y_offset, intensity, bool_applyground, bool_applytop, scale FROM sprite_light"))
	{
		for (auto& fields : result->fetchData())
		{
			const string& textureName = fields[0].getString();
			shared_ptr<SpriteScript> sptr = ensureScriptExists(textureName);
			sptr->setLight(sf::Color(fields[1].getUInt32()), fields[2].getInt32(), fields[3].getInt32(), fields[4].getInt32(), fields[5].getBool(), fields[6].getBool(), fields[7].getFloat());
		}
	}
}

void ContentMgr::loadSpriteAnimations()
{
	blog(Logger::LOG_INFO, "loadSpriteAnimations();");

	m_spriteAnimations.clear();

	vector<string> files;
	Util::getFileList("scripts\\animation", files);

	for (auto& file : files)
	{
		shared_ptr<SpriteAnimData> up = make_shared<SpriteAnimData>();

		if (up->parse(file))
		{
			Util::strReplaceAll(file, "scripts\\animation//", "");
			m_spriteAnimations[file] = up;
		}
		else
		{
			blog(Logger::LOG_ERROR, "Unable to parse sprite animation %s", file.c_str());
		}
	}
}

void ContentMgr::loadDbTables()
{
	blog(Logger::LOG_INFO, "loadDbTables();");

	m_clientdat.clear();

	shared_ptr<SqlConnector> database = SqlConnector::create(SqlType::SQLite, sConfig->getString("GameDb", "Name"), sConfig->getString("GameDb", "Path"));

	vector<string> tables;
	Util::readLinesFromFile("scripts\\dbtables.txt", tables);	

	for (auto& str : tables)
		loadDbTable(database, str);

	sItemDefiner->loadItemTemplates(database);
	sItemDefiner->loadAffixes(database);
}
		
void ContentMgr::loadDbTablesDev()
{
	shared_ptr<SqlConnector> database = SqlConnector::create(SqlType::SQLite, sConfig->getString("GameDb", "Name"), sConfig->getString("GameDb", "Path"));

	vector<string> tables;
	Util::readLinesFromFile("scripts\\dbtables_dev.txt", tables);	

	for (auto& str : tables)
		loadDbTable(database, str);
}

void ContentMgr::loadDbTable(shared_ptr<SqlConnector> db, const string& tableName)
{
	m_clientdat[tableName] = db->query(Util::fmtStr("SELECT * FROM %s", tableName.c_str()));

	if (m_clientdat[tableName] == nullptr)
		m_clientdat[tableName] = make_unique<QueryResult>();
}

void ContentMgr::getFootstepsForTexture(const string& texture, vector<string>& soundEffects) const
{
	auto itr = m_footsteps.find(texture);

	if (itr != m_footsteps.end())
		soundEffects = itr->second;
}

bool ContentMgr::isFootstepsForTexture(const string& texture) const
{
	return m_footsteps.find(texture) != m_footsteps.end();
}

void ContentMgr::loadStatDescriptions()
{
	blog(Logger::LOG_INFO, "loadStatDescriptions();");

	m_statTooltips.clear();

	// We want to just generate these exactly once since text is slow
	auto spawnTooltip = [&](const PlayerDefines::Classes c, const string& filename, const UnitDefines::Stat stat)
	{
		shared_ptr<Tooltip> tooltip;
		std::string txt = Util::readTextFile("scripts\\text\\stats\\" + filename + ".txt");

		if (!txt.empty())
		{
			tooltip = make_shared<Tooltip>(*sApplication, sf::Vector2i(0, 0));
			tooltip->addLine(FontId::Arial, 15, txt);
			tooltip->setShadowOffset(1);

			vector<UnitDefines::Stat> modStats;
			PlayerFunctions::getStatModifier(c, stat, modStats);

			string modifyStr;

			for (auto& mods : modStats)
				modifyStr += "/" + UnitFunctions::statAbbr(mods);

			if (!modifyStr.empty())
			{
				modifyStr.erase(modifyStr.begin());
				tooltip->addLine(FontId::FrizRegular, 15, "Modified By: " + modifyStr, sf::Color(240, 197, 2, 255));
			}

			return tooltip;
		}

		return tooltip;
	};

	for (int i = 0; i < UnitDefines::Stat::NumStats; ++i)
	{
		string statName = UnitFunctions::statName(UnitDefines::Stat(i));
		Util::strReplaceAll(statName, " ", "");

		// No reason to assign to the map if it doesn't actually exist, and we'll just let filesystem tell us if it does... not great but should be alright lol
		if (shared_ptr<Tooltip> ptr = spawnTooltip(PlayerDefines::Classes::Bishop, statName, UnitDefines::Stat(i)))
			m_statTooltips[PlayerDefines::Classes::Bishop][UnitDefines::Stat(i)] = ptr;

		if (shared_ptr<Tooltip> ptr = spawnTooltip(PlayerDefines::Classes::Mage, statName, UnitDefines::Stat(i)))
			m_statTooltips[PlayerDefines::Classes::Mage][UnitDefines::Stat(i)] = ptr;

		if (shared_ptr<Tooltip> ptr = spawnTooltip(PlayerDefines::Classes::Paladin, statName, UnitDefines::Stat(i)))
			m_statTooltips[PlayerDefines::Classes::Paladin][UnitDefines::Stat(i)] = ptr;

		if (shared_ptr<Tooltip> ptr = spawnTooltip(PlayerDefines::Classes::Ranger, statName, UnitDefines::Stat(i)))
			m_statTooltips[PlayerDefines::Classes::Ranger][UnitDefines::Stat(i)] = ptr;
	}
}

shared_ptr<Tooltip> ContentMgr::getStatTooltip(const PlayerDefines::Classes c, const int stat) const
{
	auto itr = m_statTooltips.find(c);

	if (itr == m_statTooltips.end())
		return nullptr;

	auto subItr = itr->second.find(UnitDefines::Stat(stat));

	if (subItr != itr->second.end())
		return subItr->second;

	return nullptr;
}

void ContentMgr::loadPortraitOffsets()
{
	blog(Logger::LOG_INFO, "loadPortraitOffsets();");

	m_portraitOffsets.clear();

	vector<string> output;
	Util::readLinesFromFile("scripts\\sprite\\portrait_offset.txt", output);

	for (auto& str : output)
	{
		string name;
		string number;

		size_t i = 0;

		for (;i < str.size() && str[i] != '='; ++i)
			name.push_back(str[i]);

		++i;
		
		for (;i < str.size(); ++i)
			number.push_back(str[i]);

		if (!name.empty() && !number.empty())
			m_portraitOffsets[name] = atoi(number.c_str());
	}
}

void ContentMgr::loadFootsteps()
{
	blog(Logger::LOG_INFO, "loadFootsteps();");

	auto loadloc = [&](const string& filename, const vector<string>& sfx)
	{
		std::vector<string> lines;
		Util::readLinesFromFile(filename, lines);

		for (auto& texturename : lines)
			m_footsteps[texturename] = sfx;
	};
	
	loadloc("scripts\\sprite\\step\\step_dirt.txt", { "step_dirt1.ogg", "step_dirt2.ogg", "step_dirt3.ogg", "step_dirt4.ogg" });
	loadloc("scripts\\sprite\\step\\step_sand.txt", { "step_sand1.ogg", "step_sand2.ogg", "step_sand3.ogg", "step_sand4.ogg" });
	loadloc("scripts\\sprite\\step\\step_stone.txt", { "step_stone1.ogg", "step_stone2.ogg", "step_stone3.ogg", "step_stone4.ogg" });
}

void ContentMgr::loadSmoothTextureNames()
{
	blog(Logger::LOG_INFO, "loadSmoothTextureNames();");

	m_smoothTextures.clear();

	std::vector<string> lines;
	Util::readLinesFromFile("scripts\\sprite\\smooth.txt", lines);
		
	for (auto& itr : lines)
		m_smoothTextures.insert(itr);
}

void ContentMgr::loadPsi()
{
	blog(Logger::LOG_INFO, "loadPsi();");

	m_psi.clear();

	vector<string> files;
	Util::getFileList("scripts\\particles", files);

	for (auto& file : files)
	{
		unique_ptr<ParticleSystemInfo> psi = make_unique<ParticleSystemInfo>();

		if (psi->loadFromDisk(file))
		{
			Util::strReplaceAll(file, "scripts\\particles//", "");
			m_psi[file] = move(psi);
		}
		else
		{
			blog(Logger::LOG_ERROR, "Unable to load psi %s", file.c_str());
		}
	}
}

bool ContentMgr::compareMusicPlaylist(const vector<string>& playlist) const
{
	if (playlist.size() != m_musicPlaylist.size())
		return false;

	for (size_t i = 0; i < playlist.size(); ++i)
	{
		if (playlist[i] != m_musicPlaylist[i])
			return false;
	}

	return true;
}

bool ContentMgr::compareAmbienceTrack(const string& ambience) const
{
	if (m_currentAmbience == nullptr)
		return false;

	if (m_transitionAmbience != -1 && m_transitionAmbienceStr != nullptr)
		return *m_transitionAmbienceStr == ambience;

	return m_currentAmbienceName == ambience;
}

bool ContentMgr::queueTextureLoad(const string& str)
{
	lock_guard<mutex> grd(m_mutex);

	auto itr = m_resourcesImages.find(str);

	// Already loaded?
	if (itr != m_resourcesImages.end())
		return false;

	m_queuedLoads.insert(str);
	return true;
}
		
// This is acceptable sfml takes care of making a new opengl context when it sees a new thread
void ContentMgr::loadingThread()
{
	sf::Texture tex;
	tex.loadFromFile(imageFile(ImageId::Icon));

	while (!m_done)
	{
		decltype(m_queuedLoads) cpy;

		{
			lock_guard<mutex> grd(m_mutex);
			cpy.swap(m_queuedLoads);
		}

		if (cpy.empty())
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
			continue;
		}

		for (auto& itr : cpy)
			getTexture(itr);
	}
}

void ContentMgr::joinModelThreads()
{
	if (m_modelScriptThreads.empty())
		return;

	for (auto& itr : m_modelScriptThreads)
		itr.join();

	m_modelScriptThreads.clear();
}

// Can be a little slow since we're reading text files, thus the threading.
// Why text files? The flare engine does so and I really can't be bothered converting them to just data.
void ContentMgr::loadModelScripts()
{
	blog(Logger::LOG_INFO, "loadModelScripts();");

	// For reloading case
	m_modelScript.clear();

	// Go through every .txt file in this directory and spawn a thread that parses the script.
	auto parseDirThread = [](const string& dir, const string& scriptPrefix)
	{
		// We'll be doing this work in its own thread, thus a function.
		auto parseScript = [](const string& filename, const string& scriptName)
		{
			vector<string> lines;
			Util::readLinesFromFile(filename, lines);
			auto script = make_shared<ModelScript>();

			if (script->parse(lines))
			{
				auto ptr = ContentMgr::instance();
				lock_guard<mutex> lock(ptr->m_mutex);
				sContentMgr->m_modelScript[scriptName] = script;
			}
			else
			{
				blog(Logger::LOG_ERROR, "Failed to load model script %s", scriptName.c_str());
			}
		};

		// In release, the time it takes to launch the threads makes it slower than it is to just do it all on the main thread
		// In debug, STL containers and string object parsing is really slow, slower than launching a thread for every .txt
		#ifdef _DEBUG
			vector<std::thread> workers;
		#endif

		vector<string> fileList;
		Util::getFileList(dir.c_str(), fileList);
		
		for (auto& filename : fileList)
		{
			// Format name, instead of "scripts\thing\scriptname.txt" we just get "scriptname" as a key.
			const auto len = dir.size() + 2;
			const string subStr = scriptPrefix + filename.substr(len, filename.size() - len - 4);

			#ifdef _DEBUG
				workers.push_back(std::thread(parseScript, filename, subStr));
			#else
				parseScript(filename, subStr);
			#endif
		}

		#ifdef _DEBUG
			for_each(workers.begin(), workers.end(), [](std::thread &t) { t.join(); });
		#endif
	};

	// Start loading
	m_modelScriptThreads.push_back(thread(parseDirThread, "scripts\\npc", ""));
	m_modelScriptThreads.push_back(thread(parseDirThread, "scripts\\player\\male", "player_male_"));
	m_modelScriptThreads.push_back(thread(parseDirThread, "scripts\\player\\female", "player_female_"));
	
	// Used by the server
	/*Database db(sConfig->getString("GameDb", "Name"), sConfig->getString("GameDb", "Path"));

	for (auto itr : m_modelScript)
	{
		for (int i = 0; i <= FlareAnimation::CastAlt; ++i)
		{
			if (auto anim = itr.second->getAnim(UnitDefines::AnimId(i)))
				db->query("INSERT INTO 'model_info' ('script_name', 'animation_id', 'duration_in_seconds') VALUES ('%s', '%d', '%f');", itr.first.c_str(), i, anim->durationInSeconds());
		}
	}*/
}

void ContentMgr::documentZips()
{
	m_zips.clear();

	vector<string> allFiles;
	Util::getFileList("content", allFiles);

	for (auto& str : allFiles)
	{
		if (str.size() < 5)
			continue;

		if (str[str.size() - 1] == 'p' &&
			str[str.size() - 2] == 'i' &&
			str[str.size() - 3] == 'z' &&
			str[str.size() - 4] == '.')
		{
			m_zips.push_back(str);
		}
	}

	vector<thread> threads;

	// Document what's in each zip
	auto doczip = [&](zip* z, const string& zname)
	{
		const zip_int64_t num_entries = zip_get_num_entries(z, 0);

		for (zip_int64_t i = 0; i < num_entries; i++)
		{
			string name = zip_get_name(z, i, 0);
			lock_guard<mutex> grd(m_mutex);
			m_zipContents[name] = zname;
		}

		ASSERT(zip_close(z) == 0);
	};	

	for (auto& zipname : m_zips)
	{
		int err = 0;

		if (zip* z = zip_open(zipname.c_str(), 0, &err))
			threads.push_back(thread(doczip, z, zipname));
	}

	for (auto& itr : threads)
		itr.join();
}

shared_ptr<sf::Texture> ContentMgr::getTexture(const string& filename)
{
	{
		lock_guard<mutex> grd(m_mutex);

		auto itr = m_resourcesImages.find(filename);

		if (itr != m_resourcesImages.end())
			return itr->second;
	}

	unsigned int size = 0;
	char* contents = nullptr;
	shared_ptr<sf::Texture> texture = nullptr;

	if (unzip(filename, contents, size))
	{
		texture = make_shared<sf::Texture>();

		if (m_smoothTextures.find(filename) != m_smoothTextures.end())
			texture->setSmooth(true);

		if (texture->loadFromMemory(contents, size))
		{
			// This texture might get loaded twice due to race condition
			// That's acceptable, though, because we don't want the main thread to wait (the mutex is only held for fast operations)
			// Just be sure the extra is discarded
			lock_guard<mutex> grd(m_mutex);			

			if (m_resourcesImages.find(filename) == m_resourcesImages.end())
				m_resourcesImages[filename] = texture;
		}
		else
		{
			blog(Logger::LOG_ERROR, "Unable to load texture %s from memory.", filename.c_str());
		}
	}

	if (contents != nullptr)
		delete[] contents;

	return texture;
}

shared_ptr<sf::SoundBuffer> ContentMgr::getSoundBuffer(const string& filename)
{
	auto itr = m_resourceSoundBuffers.find(filename);

	if (itr != m_resourceSoundBuffers.end())
		return itr->second;

	unsigned int size = 0;
	char* contents = nullptr;
	shared_ptr<sf::SoundBuffer> sbuf = nullptr;

	if (unzip(filename, contents, size))
	{
		sbuf = make_shared<sf::SoundBuffer>();

		if (sbuf->loadFromMemory(contents, size))
			m_resourceSoundBuffers[filename] = sbuf;
	}

	/*
	Remember that a music needs its source as long as it is played.
	A music file on your disk probably won't be deleted or moved while your application plays it,
	however things get more complicated when you play a music from a file in memory, or from a custom input stream:
	https://www.sfml-dev.org/tutorials/2.4/audio-sounds.php
	*/

	//if (contents != nullptr)
	//	delete[] contents;
	return sbuf;
}

shared_ptr<sf::Font> ContentMgr::getFont(const string& fontname)
{
	auto itr = m_resourceFonts.find(fontname);

	if (itr != m_resourceFonts.end())
		return itr->second;

	shared_ptr<sf::Font> f = make_shared<sf::Font>();

	if (f->loadFromFile(Assets::kFontDir + fontname))
	{
		m_resourceFonts[fontname] = f;
		return f;
	}

	return nullptr;
}

// Shaders are owned here (single source); loaded lazily on first use, path from the asset manifest.
sf::Shader& ContentMgr::getShader(Assets::ShaderId id)
{
	auto itr = m_shaders.find(id);

	if (itr != m_shaders.end())
		return itr->second;

	sf::Shader& shader = m_shaders[id];
	ASSERT(shader.loadFromFile(Assets::kShaderDir + Assets::shaderFile(id), sf::Shader::Fragment));
	return shader;
}

shared_ptr<SpriteScript> ContentMgr::ensureSpriteScript(const string& filename)
{
	shared_ptr<SpriteScript> result;

	if (result = getSpriteRenderScript(filename))
		return result;

	result = make_shared<SpriteScript>();
	m_spriteRenderScripts[filename] = result;
	return result;
}

shared_ptr<SpriteScript> ContentMgr::getSpriteRenderScript(const string& textureName) const
{
	auto itr = m_spriteRenderScripts.find(textureName);

	if (itr != m_spriteRenderScripts.end())
		return itr->second;

	return nullptr;
}
		
int ContentMgr::getPortraitOffset(const Sprite& spr) const
{
	auto itr = m_portraitOffsets.find(spr.getTextureName());

	if (itr != m_portraitOffsets.end())
	{
		// Must be zero and the offset starts from the center of the top box (portraits are tall rectangles, we're trying to center a box)
		int baseOffset = spr.getTexture()->getSize().x / 2;
		return max(0, itr->second - baseOffset);
	}

	return 0;
}

shared_ptr<Sprite> ContentMgr::spawnPortrait(const string& filename)
{
	auto result = spawnSprite(filename);

	if (result != nullptr && result->getTexture() != nullptr)
		const_cast<sf::Texture*>(result->getTexture())->setSmooth(true);
	else
		return nullptr;

	return result;
}
		
shared_ptr<Sprite> ContentMgr::spawnPlayerPortrait(int id, int gender)
{
	string spriteName = "portrait_";

	if (gender == PlayerDefines::Gender::Male)
	{
		if (id > CharacterDefines::Portraits::NumMale)
			id = 1;
		else if (id < 1)
			id = CharacterDefines::Portraits::NumMale;

		spriteName += "male (" + to_string(id) + ").png";
	}
	else if (gender == PlayerDefines::Gender::Female)
	{
		if (id > CharacterDefines::Portraits::NumFemale)
			id = 1;
		else if (id < 1)
			id = CharacterDefines::Portraits::NumFemale;

		spriteName += "female (" + to_string(id) + ").png";
	}

	return spawnPortrait(spriteName);
}

shared_ptr<Sprite> ContentMgr::spawnSprite(const string& filename)
{
	shared_ptr<sf::Texture> tex = getTexture(filename);

	// It's valid for a sprite object to have just a particle system and nothing else
	if (tex == nullptr && !isPsiExist(filename))
		blog(Logger::LOG_ERROR, "Unknown texture %s", filename.c_str());

	return make_shared<Sprite>(tex, filename);
}

shared_ptr<ButtonScript> ContentMgr::buttonScript(const string& scriptName) const
{
	auto itr = m_buttonScripts.find(scriptName);

	if (itr != m_buttonScripts.end())
		return itr->second;

	blog(Logger::LOG_ERROR, "Unable to find button script %s", scriptName.c_str());
	return nullptr;
}

shared_ptr<SpriteAnimation> ContentMgr::spawnSpriteAnimation(const string& name)
{
	auto itr = m_spriteAnimations.find(name);

	if (itr != m_spriteAnimations.end())
		return make_shared<SpriteAnimation>(itr->second);

	blog(Logger::LOG_ERROR, "Unable to find sprite animation %s", name.c_str());
	return nullptr;
}

unique_ptr<ParticleSystem> ContentMgr::spawnParticleSystem(const string& scriptName)
{	
	auto itr = m_psi.find(scriptName);

	if (itr == m_psi.end())
	{
		blog(Logger::LOG_ERROR, "Failed to spawn particle system %s", scriptName.c_str());
		return nullptr;
	}

	return move(make_unique<ParticleSystem>(*itr->second));
}

unique_ptr<ModelScript> ContentMgr::spawnModelScript(const string& scriptName)
{
	joinModelThreads();
	
	auto itr = m_modelScript.find(scriptName);

	if (itr != m_modelScript.end())
		return make_unique<ModelScript>(*itr->second);

	blog(Logger::LOG_ERROR, "Unable to find npc model script %s", scriptName.c_str());
	return nullptr;
}

shared_ptr<sf::Music> ContentMgr::getMusic(const string& filename)
{
	auto itr = m_resourcesMusics.find(filename);

	if (itr != m_resourcesMusics.end())
		return itr->second;

	unsigned int size = 0;
	char* contents = nullptr;
	shared_ptr<sf::Music> music = nullptr;

	if (unzip(filename, contents, size))
	{
		music = make_shared<sf::Music>();

		if (music->openFromMemory(contents, size))
			m_resourcesMusics[filename] = music;
	}

	/*
	Remember that a music needs its source as long as it is played.
	A music file on your disk probably won't be deleted or moved while your application plays it,
	however things get more complicated when you play a music from a file in memory, or from a custom input stream:
	https://www.sfml-dev.org/tutorials/2.4/audio-sounds.php
	*/

	//if (contents != nullptr)
	//	delete[] contents;

	return music;
}

void ContentMgr::loadButtonScripts()
{
	blog(Logger::LOG_INFO, "loadButtonScripts();");

	if (!m_buttonScripts.empty())
		m_buttonScripts.clear();

	vector<string> fileList;
	Util::getFileList("scripts\\buttons", fileList);

	for (auto& str : fileList)
	{
		if (str.find(".bscript") == string::npos)
			continue;

		vector<string> lines;
		Util::readLinesFromFile(str, lines);

		shared_ptr<ButtonScript> bscript = make_shared<ButtonScript>();

		if (bscript->parse(lines))
			m_buttonScripts[lines[0]] = bscript;
		else
			blog(Logger::LOG_ERROR, "%s invalid button script.", str.c_str());
	}
}

bool ContentMgr::unzip(const string& filename, char*& contents, unsigned int& size)
{
	ASSERT(contents == nullptr);

	// We've documented which zip has the file we're looking for
	auto itr = m_zipContents.find(filename);

	if (itr == m_zipContents.end())
		return false;

	const string& zipname = itr->second;

	int err = 0;
	zip* z = zip_open(zipname.c_str(), 0, &err);

	if (z == nullptr)
	{
		blog(Logger::LOG_ERROR, "Unable to open zip %s", zipname.c_str());
		return false;
	}

	struct zip_stat st;
	zip_stat_init(&st);
	zip_stat(z, filename.c_str(), 0, &st);

	if (zip_file* f = zip_fopen(z, filename.c_str(), 0))
	{
		size = (unsigned int)st.size;
		contents = new char[size];

		zip_fread(f, contents, size);
		zip_fclose(f);
		ASSERT(zip_close(z) == 0);
		return true;
	}

	ASSERT(zip_close(z) == 0);
	return false;
}

shared_ptr<SfSoundSafe> ContentMgr::spawnSound(const string& filename)
{
	if (shared_ptr<sf::SoundBuffer> sbuf = getSoundBuffer(filename))
		return make_shared<SfSoundSafe>(sbuf);

	return nullptr;
}

int ContentMgr::getSfxVolume() const
{
	return sConfig->getCache(Options::System_SfxVolumeBar);
}

int ContentMgr::getMusicVolume() const
{
	return sConfig->getCache(Options::System_MusicVolumeBar);
}

float ContentMgr::getAmbienceVolume() const
{
	return float(getSfxVolume()) / 10.f;
}

QueryResult& ContentMgr::editableDb(const string& tablename)
{
	return (QueryResult&)db(tablename);
}

QueryResult const& ContentMgr::db(const string& tablename)
{
	auto itr = m_clientdat.find(tablename);
	ASSERT(itr != m_clientdat.end());
	return *itr->second.get();
}

void ContentMgr::dumpTextures()
{
	lock_guard<mutex> grd(m_mutex);

	auto itr = m_resourcesImages.begin();

	while (itr != m_resourcesImages.end())
	{
		if (itr->second.use_count() == 1)
			itr = m_resourcesImages.erase(itr);
		else
			++itr;
	}
}