#pragma once
#include <string>

class CFgUtf8Utils
{
public:
	CFgUtf8Utils();
	~CFgUtf8Utils();

	static std::wstring UTF8_To_UTF16(const char* utf8Text);
};

