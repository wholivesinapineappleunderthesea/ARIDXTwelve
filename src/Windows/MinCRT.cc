#include <stddef.h>

extern "C"
{
	// memmove
	void* memmove(void* dest, const void* src, size_t count)
	{
		char* d = (char*)dest;
		const char* s = (const char*)src;
		if (d < s)
		{
			while (count--)
				*d++ = *s++;
		}
		else
		{
			const char* lasts = s + (count - 1);
			char* lastd = d + (count - 1);
			while (count--)
				*lastd-- = *lasts--;
		}
		return dest;
	}

	void* memset(void* s, int c, size_t n)
	{
		unsigned char* p = reinterpret_cast<unsigned char*>(s);
		while (n--)
			*p++ = (unsigned char)c;
		return s;
	}

	// _fltused
	int _fltused = 0x9875;

	// _RTC_CheckStackVars
	int _RTC_CheckStackVars(void)
	{
		return 1;
	}

	// _RTC_Shutdown
	int _RTC_Shutdown(void)
	{
		return 1;
	}

	// _RTC_InitBase
	int _RTC_InitBase(void)
	{
		return 1;
	}

	// _purecall
	int _purecall(void)
	{
		*(int*)0 = 0;
		return 0;
	}
}
