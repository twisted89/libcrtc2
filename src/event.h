#ifndef CRTC_EVENT_H
#define CRTC_EVENT_H

#include "crtc.h"

namespace crtc {
	class Event {
		Event(const Event&) = delete;
		Event& operator=(const Event&) = delete;

	public:
		static std::shared_ptr<Event> New();

	protected:
		explicit Event();
		~Event();
	};
}

#endif