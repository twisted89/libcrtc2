#include "crtc.h"

namespace crtc
{
	class String::Impl
	{
	public:
		~Impl()
		{
			if(text)
				free(text);
		}

		Impl() : text(nullptr), length(0) { }

#ifdef  _WIN32
		Impl(char const* const t) : text(_strdup(t)), length(strlen(t)) {}
#else
		Impl(char const* const t) : text(strdup(t)), length(strlen(t)) {}
#endif //  _WIN32

		

		Impl(const char* t, size_t len) : text(nullptr), length(len) {
			text = reinterpret_cast<char *>(malloc(length + 1));
			if (text) {
				memcpy(text, t, length);
				text[length] = 0;
			}
		}

		Impl* clone() const
		{
			return new Impl(text, length);
		}

		char* text;
		size_t length;
	};

	String::~String()
	{
		delete impl;
	}

	String::String() : impl(new Impl()) {}

	String::String(const char* text) : impl(new Impl(text)) {}

	String::String(const char* text, size_t length) : impl(new Impl(text, length)) {}

	String::String(const String& other) : impl(other.impl->clone()) {}

	String& String::swap(String& other)
	{
		std::swap(impl, other.impl);
		return *this;
	}

	size_t String::size() const
	{
		return impl->length;
	}

	String& String::operator=(String rhs)
	{
		rhs.swap(*this);
		return *this;
	}

	String& String::operator=(const char* text)
	{
		return *this = String(text);
	}
	
	const char* String::get() const
	{
		return impl->text;
	}

}