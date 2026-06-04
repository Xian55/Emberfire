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
		bool m_mouseable;

		RenderObject* m_owner;
};

