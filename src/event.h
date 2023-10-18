#ifndef CRTC_EVENT_H
#define CRTC_EVENT_H

#include "crtc.h"

namespace crtc {
	class Event {
		Event(const Event&) = delete;
		Event& operator=(const Event&) = delete;

	public:
		explicit Event();
		virtual ~Event();

		static std::shared_ptr<Event> New();
	};
}

#endif