#ifndef CRTC_EVENT_H
#define CRTC_EVENT_H

#include "crtc.h"

namespace crtc {
	class Event : virtual public Reference {
		Event(const Event&) = delete;
		Event& operator=(const Event&) = delete;
		friend class Let<Event>;

	public:
		static Let<Event> New();

	protected:
		explicit Event();
		~Event() override;
	};
}

#endif