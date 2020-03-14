#include "FgUtf8Utils.h"
#include <Windows.h>

CFgUtf8Utils::CFgUtf8Utils()
{
}


CFgUtf8Utils::~CFgUtf8Utils()
{
}

std::wstring CFgUtf8Utils::UTF8_To_UTF16(const char* utf8Text)
{  
	unsigned long len = ::MultiByteToWideChar(CP_UTF8, NULL, utf8Text, -1, NULL, NULL);
	if (len == 0)  
		return std::wstring(L"");  
	wchar_t *buffer = new wchar_t[len];  
	::MultiByteToWideChar(CP_UTF8, NULL, utf8Text, -1, buffer, len);

	std::wstring dest(buffer);
	delete[] buffer;  

	return dest;  
} 
