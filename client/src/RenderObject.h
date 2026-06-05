#pragma once

#include "MouseableNode.h"
#include "AnchoredPosition.h"

class RenderObjectHolder;

class RenderObject : public MouseableNode, public AnchoredPosition
{
	public:
		RenderObject(RenderObject* owner, const int id = 0);
		virtual ~RenderObject();

		void attemptInput();
		void attemptRender();

		void setMouseable(const bool b) { m_mouseable = b; }

		bool mayInput() const;
		bool isChildOf(RenderObject& obj);
		bool isHidden() const { return m_isHidden; }
		bool wasInputLastFrame() const;

		virtual void resetActivated() { }
		virtual void setHidden(const bool b) { m_isHidden = b; }
		// Force-hide overrides the object's own setHidden() bookkeeping: a window that
		// re-shows itself every frame (e.g. UnitFrame when it has a unit) stays suppressed.
		// Used by the Lua UI layer to retire a C++ window in favour of a Lua replacement.
		void setForceHidden(const bool b) { m_forceHidden = b; }
		bool isForceHidden() const { return m_forceHidden; }
		// Z-level within the parent holder: higher renders on top (WoW SetFrameLevel). Default 0; equal
		// levels keep creation order. The parent re-sorts when a child's level changes.
		void setZLevel(const int n) { m_zLevel = n; }
		int  getZLevel() const { return m_zLevel; }
		virtual void notifyCtxMenuClicked(const int id, const string& lineClicked) {}

		virtual bool isMousedOver(const bool useRealPosition = false) override;
		virtual bool childMayInput(RenderObject const& obj) const { return mayInput(); }
		virtual bool popActivated() { return false; }
		virtual bool popRightClicked() { return false; }
		virtual bool popLeftClicked() { return false; }
		virtual bool popKeyBound() { return false; }


		int getId() const { return m_id; }

		RenderObject* getOwner() const { return m_owner; }

	protected:
		virtual void input() = 0;
		virtual void render() = 0;

		void initOwner(RenderObject* owner);

	private:
		int m_id;
		int m_lastInputFrame{0};

		bool m_isHidden;
		bool m_forceHidden{false};
		bool m_mouseable;
		int  m_zLevel{0};

		RenderObject* m_owner;
};

