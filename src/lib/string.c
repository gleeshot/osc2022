#include "string.h"

int strcmp(const char *s1, const char *s2)
{
	while (*s1 && (*s1 == *s2))
	{
		s1++;
		s2++;
	}
	return *(const unsigned char *)s1 - *(const unsigned char *)s2;
}

int strncmp(const char *cs, const char *ct, unsigned int count)
{
	unsigned char c1, c2;

	while (count)
	{
		c1 = *cs++;
		c2 = *ct++;
		if (c1 != c2)
			return c1 < c2 ? -1 : 1;
		if (!c1)
			break;
		count--;
	}
	return 0;
}

unsigned long hexStr2int(char *hex, int n)
{
	unsigned long val = 0;
	while (*hex && n > 0)
	{
		int byte = *hex++;
		if (byte >= '0' && byte <= '9')
		{
			byte = byte - '0';
		}
		else if (byte >= 'a' && byte <= 'f')
		{
			byte = byte - 'a' + 10;
		}
		else if (byte >= 'A' && byte <= 'F')
		{
			byte = byte - 'A' + 10;
		}
		val = (val << 4) | (byte & 0xF);
		n--;
	}
	return val;
}

unsigned int strlen(char *str)
{
	unsigned int len = 0;
	while (str[len] != '\0')
	{
		len++;
	}
	return len;
}