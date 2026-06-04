#pragma once

class ClientObject;
class SpellVisualKit;

class AuraAnimation
{
	public:
		AuraAnimation(const int auraKitId, ClientObject const& holder);
		~AuraAnimation();

		void update();
		void render();
		void stop();
		void resume();

		void setUseHolderPositionForVolume(const bool v) { m_holderPosForVolume = v; }

		bool finished() const;
		bool isStopped() const { return m_stopped; }

	private:
		bool m_finished;
		bool m_stopped;
		bool m_holderPosForVolume{false};

		int m_kitId;

		unique_ptr<SpellVisualKit> m_kit;
		
		ClientObject const& m_holder;
};

