/*
 * MacroQuest2: The extension platform for EverQuest
 * Copyright (C) 2002-2019 MacroQuest Authors
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2, as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "MQ2Main.h"

#include <DbgHelp.h>

#ifdef _DEBUG
#define DBG_SPEW // enable DebugSpew messages in debug builds
#endif

//***************************************************************************
// Function:    DebugSpew
// Description: Outputs text to debugger, usage is same as printf ;)
//***************************************************************************

static void LogToFile(const char* szOutput)
{
	FILE* fOut = nullptr;
	char szFilename[MAX_PATH] = { 0 };

	sprintf_s(szFilename, "%s\\DebugSpew.log", gszLogPath);
	errno_t err = fopen_s(&fOut, szFilename, "at");

	if (err || !fOut)
		return;

#ifdef DBG_CHARNAME
	char Name[256] = "Unknown";
	if (CHARINFO* pCharInfo = GetCharInfo())
	{
		strcpy_s(Name, pCharInfo->Name);
	}
	fprintf(fOut, "%s - ", Name);
#endif

	fprintf(fOut, "%s\r\n", szOutput);
	fclose(fOut);
}

static void DebugSpewImpl(bool always, bool logToFile, const char* szFormat, va_list vaList)
{
	if (!always && gFilterDebug)
		return;

	// _vscprintf doesn't count // terminating '\0'
	int len = _vscprintf(szFormat, vaList) + 1;
	int headerlen = strlen(DebugHeader) + 1;
	size_t theLen = len + headerlen + 32;

	auto out = std::make_unique<char[]>(theLen);
	char* szOutput = out.get();

	strcpy_s(szOutput, theLen, DebugHeader " ");
	vsprintf_s(szOutput + headerlen, theLen - headerlen, szFormat, vaList);

	strcat_s(szOutput, theLen, "\n");
	OutputDebugString(szOutput);

	if (logToFile)
	{
		LogToFile(szOutput);
	}
}

void DebugSpew(const char* szFormat, ...)
{
	va_list vaList;
	va_start(vaList, szFormat);

	DebugSpewImpl(false, false, szFormat, vaList);
}

void DebugSpewAlways(const char* szFormat, ...)
{
	va_list vaList;
	va_start(vaList, szFormat);

	DebugSpewImpl(true, gSpewToFile, szFormat, vaList);
}

void DebugSpewAlwaysFile(const char* szFormat, ...)
{
	va_list vaList;
	va_start(vaList, szFormat);

	DebugSpewImpl(true, true, szFormat, vaList);
}

EQLIB_API void DebugSpewNoFile(const char* szFormat, ...)
{
#ifdef DBG_SPEW
	va_list vaList;
	va_start(vaList, szFormat);

	DebugSpewImpl(true, false, szFormat, vaList);
#endif
}

// Implemented in MQ2PluginHandler.cpp
void PluginsWriteChatColor(const char* Line, int Color, int Filter);

static void WriteChatColorMaybeDeferred(std::unique_ptr<char[]> Ptr, int Color, int Filter)
{
	if (IsMainThread())
	{
		PluginsWriteChatColor(Ptr.get(), Color, Filter);
	}

	// Queue it up to run on the main thread
	PostToMainThread(
		[Ptr = std::shared_ptr<char[]>{ std::move(Ptr) }, Color, Filter]()
	{
		PluginsWriteChatColor(Ptr.get(), Color, Filter);
	});
}

void WriteChatColor(const char* Line, int Color /* = USERCOLOR_DEFAULT */, int Filter /* = 0 */)
{
	// If we're alreadyon the main thread, avoid copying anything and just call
	// straight to PluginsWriteChatColor

	if (IsMainThread())
	{
		PluginsWriteChatColor(Line, Color, Filter);
		return;
	}

	// we're not on the main thread, we need to copy the string and queue up a function
	// to be executed on the main thread.
	size_t length = strlen(Line) + 1;
	std::shared_ptr<char[]> Ptr{ new char[length] };
	strcpy_s(Ptr.get(), length, Line);

	// Queue it up to run on the main thread
	PostToMainThread(
		[Ptr, Color, Filter]()
	{
		PluginsWriteChatColor(Ptr.get(), Color, Filter);
	});
}

void WriteChatf(const char* szFormat, ...)
{
	va_list vaList;
	va_start(vaList, szFormat);

	// _vscprintf doesn't count // terminating '\0'
	int len = _vscprintf(szFormat, vaList) + 1;

	auto out = std::make_unique<char[]>(len);
	char* szOutput = out.get();

	vsprintf_s(szOutput, len, szFormat, vaList);
	WriteChatColor(szOutput);
}

void WriteChatfSafe(const char* szFormat, ...)
{
	va_list vaList;
	va_start(vaList, szFormat);

	// _vscprintf doesn't count // terminating '\0'
	int len = _vscprintf(szFormat, vaList) + 1;

	auto out = std::make_unique<char[]>(len);
	char* szOutput = out.get();

	vsprintf_s(szOutput, len, szFormat, vaList);
	WriteChatColor(szOutput);
}

void WriteChatColorf(const char* szFormat, int color, ...)
{
	va_list vaList;
	va_start(vaList, color);

	// _vscprintf doesn't count // terminating '\0'
	int len = _vscprintf(szFormat, vaList) + 1;

	auto out = std::make_unique<char[]>(len);
	char* szOutput = out.get();

	vsprintf_s(szOutput, len, szFormat, vaList);
	WriteChatColor(szOutput, color);
}

//============================================================================

static void StrReplaceSection(char* szInsert, size_t InsertLen, DWORD Length, const char* szNewString)
{
	DWORD NewLength = (DWORD)strlen(szNewString);
	memmove(&szInsert[NewLength], &szInsert[Length], strlen(&szInsert[Length]) + 1);
	memcpy_s(szInsert, InsertLen - NewLength, szNewString, NewLength);
}

void ConvertCR(char* Text, size_t LineLen)
{
	// not super-efficient but this is only being called at initialization currently.
	while (char* Next = strstr(Text, "\\n"))
	{
		int len = (int)(Next - Text);
		StrReplaceSection(Next, LineLen - len, 2, "\n");
	}
}

void SyntaxError(const char* szFormat, ...)
{
	va_list vaList;
	va_start(vaList, szFormat);

	int len = _vscprintf(szFormat, vaList) + 1 + 32;

	auto out = std::make_unique<char[]>(len);
	char* szOutput = out.get();

	vsprintf_s(szOutput, len, szFormat, vaList);
	WriteChatColor(szOutput, CONCOLOR_YELLOW);
	strcpy_s(gszLastSyntaxError, szOutput);
}

void MacroError(const char* szFormat, ...)
{
	va_list vaList;
	va_start(vaList, szFormat);

	int len = _vscprintf(szFormat, vaList) + 1 + 32;

	auto out = std::make_unique<char[]>(len);
	char* szOutput = out.get();

	vsprintf_s(szOutput, len, szFormat, vaList);
	WriteChatColor(szOutput, CONCOLOR_RED);

	if (bAllErrorsLog) MacroLog(nullptr, "Macro Error");
	if (bAllErrorsLog) MacroLog(nullptr, szOutput);

	strcpy_s(gszLastNormalError, szOutput);

	if (gMacroBlock)
	{
		if (bAllErrorsDumpStack || bAllErrorsFatal)
			DumpStack(nullptr, nullptr);

		if (bAllErrorsFatal)
			EndMacro((SPAWNINFO*)pLocalPlayer, "");
	}
}

void FatalError(const char* szFormat, ...)
{
	va_list vaList;
	va_start(vaList, szFormat);

	int len = _vscprintf(szFormat, vaList) + 1 + 32;

	auto out = std::make_unique<char[]>(len);
	char* szOutput = out.get();

	vsprintf_s(szOutput, len, szFormat, vaList);
	WriteChatColor(szOutput, CONCOLOR_RED);
	strcpy_s(gszLastNormalError, szOutput);

	if (bAllErrorsLog) MacroLog(nullptr, "Fatal Error");
	if (bAllErrorsLog) MacroLog(nullptr, szOutput);

	if (gMacroBlock)
	{
		DumpStack(nullptr, nullptr);
		EndMacro((SPAWNINFO*)pLocalPlayer, "");
	}
}

void MQ2DataError(char* szFormat, ...)
{
	va_list vaList;
	va_start(vaList, szFormat);

	int len = _vscprintf(szFormat, vaList) + 1 + 32;

	auto out = std::make_unique<char[]>(len);
	char* szOutput = out.get();

	vsprintf_s(szOutput, len, szFormat, vaList);
	if (gFilterMQ2DataErrors)
		DebugSpew("%s", szOutput);
	else
		WriteChatColor(szOutput, CONCOLOR_RED);

	strcpy_s(gszLastMQ2DataError, szOutput);
	if (bAllErrorsLog) MacroLog(nullptr, "Data Error");
	if (bAllErrorsLog) MacroLog(nullptr, szOutput);

	if (gMacroBlock)
	{
		if (bAllErrorsDumpStack || bAllErrorsFatal)
			DumpStack(nullptr, nullptr);

		if (bAllErrorsFatal)
			EndMacro((SPAWNINFO*)pLocalPlayer, "");
	}
}

// ***************************************************************************
// Function:    GetNextArg
// Description: Returns a pointer to the next argument
// ***************************************************************************
char* GetNextArg(char* szLine, int dwNumber, bool CSV /* = false */, char Separator /* = 0 */)
{
	char* szNext = szLine;
	bool InQuotes = false;
	bool CustomSep = Separator != 0;

	while ((!CustomSep && szNext[0] == ' ')
		|| (!CustomSep && szNext[0] == '\t')
		|| (CustomSep && szNext[0] == Separator)
		|| (!CustomSep && CSV && szNext[0] == ','))
	{
		szNext++;
	}

	if (dwNumber < 1)
		return szNext;

	for (dwNumber; dwNumber > 0; dwNumber--)
	{
		while (((CustomSep || szNext[0] != ' ')
			&& (CustomSep || szNext[0] != '\t')
			&& (!CustomSep || szNext[0] != Separator)
			&& (CustomSep || !CSV || szNext[0] != ',')
			&& szNext[0] != 0)
			|| InQuotes)
		{
			if (szNext[0] == 0 && InQuotes)
			{
				DebugSpew("GetNextArg - No matching quote, returning empty string");
				return szNext;
			}

			if (szNext[0] == '"')
				InQuotes = !InQuotes;
			szNext++;
		}

		while ((!CustomSep && szNext[0] == ' ')
			|| (!CustomSep && szNext[0] == '\t')
			|| (CustomSep && szNext[0] == Separator)
			|| (!CustomSep && CSV && szNext[0] == ','))
		{
			szNext++;
		}
	}

	return szNext;
}

// ***************************************************************************
// Function:    GetArg
// Description: Returns a pointer to the current argument in szDest
// ***************************************************************************
char* GetArg(char* szDest, char* szSrc, int dwNumber, bool LeaveQuotes, bool ToParen, bool CSV, char Separator, bool AnyNonAlphaNum)
{
	if (!szSrc)
		return nullptr;

	bool CustomSep = false;
	bool InQuotes = false;

	char* szTemp = szSrc;
	ZeroMemory(szDest, MAX_STRING);

	if (Separator != 0) CustomSep = true;

	szTemp = GetNextArg(szTemp, dwNumber - 1, CSV, Separator);
	int i = 0;
	int j = 0;

	while ((
		(CustomSep || szTemp[i] != ' ')
		&& (CustomSep || szTemp[i] != '\t')
		&& (CustomSep || !CSV || szTemp[i] != ',')
		&& (!CustomSep || szTemp[i] != Separator)
		&& (!AnyNonAlphaNum || ((szTemp[i] >= '0' && szTemp[i] <= '9')
			|| (szTemp[i] >= 'a' && szTemp[i] <= 'z')
			|| (szTemp[i] >= 'A' && szTemp[i] <= 'Z')
			|| szTemp[i] == '_'))
		&& (szTemp[i] != 0)
		&& (!ToParen || szTemp[i] != ')'))
		|| InQuotes)
	{
		if (szTemp[i] == 0 && InQuotes)
		{
			DebugSpew("GetArg - No matching quote, returning entire string");
			DebugSpew("Source = %s", szSrc);
			DebugSpew("Dest = %s", szDest);
			return szDest;
		}

		if (szTemp[i] == '"')
		{
			InQuotes = !InQuotes;
			if (LeaveQuotes)
			{
				szDest[j] = szTemp[i];
				j++;
			}
		}
		else
		{
			szDest[j] = szTemp[i];
			j++;
		}
		i++;
	}

	if (ToParen && szTemp[i] == ')')
		szDest[j] = ')';

	return szDest;
}

// TODO:  Remove since the working directory is the EQ directory
char* GetEQPath(char* szBuffer, size_t len)
{
	GetModuleFileName(nullptr, szBuffer, MAX_STRING);

	char* pSearch = nullptr;
	_strlwr_s(szBuffer, len);

	if (pSearch = strstr(szBuffer, "\\wineq\\"))
		*pSearch = 0;
	else if (pSearch = strstr(szBuffer, "\\testeqgame.exe"))
		*pSearch = 0;
	else if (pSearch = strstr(szBuffer, "\\eqgame.exe"))
		*pSearch = 0;

	return szBuffer;
}

void ConvertItemTags(CXStr& cxstr, bool Tag)
{
	using fnConvertItemTags = void(*)(CXStr&, bool);

	// FIXME why asm?

	if (fnConvertItemTags func = (fnConvertItemTags)EQADDR_CONVERTITEMTAGS)
	{
		func(cxstr, Tag);
	}
#if 0
	__asm {
		push ecx;
		push eax;
		push [Tag];
		push [cxstr];
		call [EQADDR_CONVERTITEMTAGS];
		pop  ecx;
		pop  ecx;
		pop  eax;
		pop  ecx;
	};
#endif
}

#define InsertColor(text, color) sprintf(text,"<c \"#%06X\">", color); TotalColors++;
#define InsertColorSafe(text, len, color) sprintf_s(text, len, "<c \"#%06X\">", color); TotalColors++;
#define InsertStopColor(text)   sprintf(text, "</c>"); TotalColors--;
#define InsertStopColorSafe(text, len) sprintf_s(text, len, "</c>"); TotalColors--;

void StripMQChat(const char* in, char* out)
{
	int i = 0;
	int o = 0;
	while (in[i])
	{
		if (in[i] == '\a')
		{
			i++;
			if (in[i] == '-')
			{
				// skip 1 after -
				i++;
			}
			else if (in[i] == '#')
			{
				// skip 6 after #
				i += 6;
			}
		}
		else if (in[i] == '\n')
		{
		}
		else
			out[o++] = in[i];
		i++;
	}
	out[o] = 0;
}

static bool ReplaceSafely(char** out, size_t* pchar_out_string_position, char chr, size_t maxlen)
{
	if ((*pchar_out_string_position) + 1 > maxlen)
		return false;
	(*out)[(*pchar_out_string_position)++] = chr;
	return true;
}

DWORD MQToSTML(const char* in, char* out, size_t maxlen, uint32_t ColorOverride)
{
	//DebugSpew("MQToSTML(%s)",in);
	// 1234567890123
	// <c "#123456">
	//char szCmd[MAX_STRING] = { 0 };
	//strcpy_s(szCmd, out);

	int outlen = maxlen;
	if (maxlen > 14)
		maxlen -= 14; // make room for this: <c "#123456">

	size_t pchar_in_string_position = 0;
	size_t pchar_out_string_position = 0;
	bool bFirstColor = false;
	bool bNBSpace = false;
	ColorOverride &= 0xFFFFFF;
	uint32_t CurrentColor = ColorOverride;

	int TotalColors = 0; // this MUST be signed.

	pchar_out_string_position += InsertColorSafe(&out[pchar_out_string_position], outlen - pchar_out_string_position, CurrentColor);

	while (in[pchar_in_string_position] != 0 && pchar_out_string_position < maxlen)
	{
		if (in[pchar_in_string_position] == ' ')
		{
			if (bNBSpace)
			{
				if (!ReplaceSafely(&out, &pchar_out_string_position, '&', maxlen))
					break;
				if (!ReplaceSafely(&out, &pchar_out_string_position, 'N', maxlen))
					break;
				if (!ReplaceSafely(&out, &pchar_out_string_position, 'B', maxlen))
					break;
				if (!ReplaceSafely(&out, &pchar_out_string_position, 'S', maxlen))
					break;
				if (!ReplaceSafely(&out, &pchar_out_string_position, 'P', maxlen))
					break;
				if (!ReplaceSafely(&out, &pchar_out_string_position, ';', maxlen))
					break;
			}
			else
			{
				if (!ReplaceSafely(&out, &pchar_out_string_position, ' ', maxlen))
					break;
			}

			bNBSpace = true;
		}
		else
		{
			bNBSpace = false;

			switch (in[pchar_in_string_position])
			{
			case '\a':
				// HANDLE COLOR
				bFirstColor = true;
				pchar_in_string_position++;

				if (in[pchar_in_string_position] == 'x')
				{
					CurrentColor = -1;
					pchar_out_string_position += InsertStopColorSafe(&out[pchar_out_string_position], outlen - pchar_out_string_position);

					if (pchar_out_string_position >= maxlen)
						break;
				}
				else
				{
					if (in[pchar_in_string_position] == '#')
					{
						pchar_in_string_position++;
						char temp[7];
						for (int x = 0; x < 6; x++)
						{
							temp[x] = in[pchar_in_string_position++];
						}
						pchar_in_string_position--;
						temp[6] = 0;
						CurrentColor = -1;
						//pchar_out_string_position += sprintf_s(&out[pchar_out_string_position],outlen-pchar_out_string_position, "<c \"#%s\">", &temp[0]);
						pchar_out_string_position += sprintf_s(&out[pchar_out_string_position], outlen - pchar_out_string_position, "<c \"#%s\">", &temp[0]);
						TotalColors++;
						if (pchar_out_string_position >= maxlen)
							break;
					}
					else
					{
						bool Dark = false;

						if (in[pchar_in_string_position] == '-')
						{
							Dark = true;
							pchar_in_string_position++;
						}

						uint32_t LastColor = CurrentColor;
						switch (in[pchar_in_string_position])
						{
						case 'y': // yellow (green/red)
							if (Dark)
								CurrentColor = 0x999900;
							else
								CurrentColor = 0xFFFF00;
							break;
						case 'o': // orange (green/red)
							if (Dark)
								CurrentColor = 0x996600;
							else
								CurrentColor = 0xFF9900;
							break;
						case 'g': // green   (green)
							if (Dark)
								CurrentColor = 0x009900;
							else
								CurrentColor = 0x00FF00;
							break;
						case 'u': // blue   (blue)
							if (Dark)
								CurrentColor = 0x000099;
							else
								CurrentColor = 0x0000FF;
							break;
						case 'r': // red     (red)
							if (Dark)
								CurrentColor = 0x990000;
							else
								CurrentColor = 0xFF0000;
							break;
						case 't': // teal (blue/green)
							if (Dark)
								CurrentColor = 0x009999;
							else
								CurrentColor = 0x00FFFF;
							break;
						case 'b': // black   (none)
							CurrentColor = 0x000000;
							break;
						case 'm': // magenta (blue/red)
							if (Dark)
								CurrentColor = 0x990099;
							else
								CurrentColor = 0xFF00FF;
							break;
						case 'p': // purple (blue/red)
							if (Dark)
								CurrentColor = 0x660099;
							else
								CurrentColor = 0x9900FF;
							break;
						case 'w': // white   (all)
							if (Dark)
								CurrentColor = 0x999999;
							else
								CurrentColor = 0xFFFFFF;
							break;
						}

						if (CurrentColor != LastColor)
						{
							//pchar_out_string_position += InsertColor(&out[pchar_out_string_position], CurrentColor);
							pchar_out_string_position += InsertColorSafe(&out[pchar_out_string_position], outlen - pchar_out_string_position, CurrentColor);
							if (pchar_out_string_position >= maxlen)
								break;
						}
					}
				}
				break;

			case '&':
				if (!ReplaceSafely(&out, &pchar_out_string_position, '&', maxlen))
					break;
				if (!ReplaceSafely(&out, &pchar_out_string_position, 'A', maxlen))
					break;
				if (!ReplaceSafely(&out, &pchar_out_string_position, 'M', maxlen))
					break;
				if (!ReplaceSafely(&out, &pchar_out_string_position, 'P', maxlen))
					break;
				if (!ReplaceSafely(&out, &pchar_out_string_position, ';', maxlen))
					break;
				break;

			case '%':
				if (!ReplaceSafely(&out, &pchar_out_string_position, '&', maxlen))
					break;
				if (!ReplaceSafely(&out, &pchar_out_string_position, 'P', maxlen))
					break;
				if (!ReplaceSafely(&out, &pchar_out_string_position, 'C', maxlen))
					break;
				if (!ReplaceSafely(&out, &pchar_out_string_position, 'T', maxlen))
					break;
				if (!ReplaceSafely(&out, &pchar_out_string_position, ';', maxlen))
					break;
				break;

			case '<':
				if (!ReplaceSafely(&out, &pchar_out_string_position, '&', maxlen))
					break;
				if (!ReplaceSafely(&out, &pchar_out_string_position, 'L', maxlen))
					break;
				if (!ReplaceSafely(&out, &pchar_out_string_position, 'T', maxlen))
					break;
				if (!ReplaceSafely(&out, &pchar_out_string_position, ';', maxlen))
					break;
				break;

			case '>':
				if (!ReplaceSafely(&out, &pchar_out_string_position, '&', maxlen))
					break;
				if (!ReplaceSafely(&out, &pchar_out_string_position, 'G', maxlen))
					break;
				if (!ReplaceSafely(&out, &pchar_out_string_position, 'T', maxlen))
					break;
				if (!ReplaceSafely(&out, &pchar_out_string_position, ';', maxlen))
					break;
				break;

			case '"':
				if (!ReplaceSafely(&out, &pchar_out_string_position, '&', maxlen))
					break;
				if (!ReplaceSafely(&out, &pchar_out_string_position, 'Q', maxlen))
					break;
				if (!ReplaceSafely(&out, &pchar_out_string_position, 'U', maxlen))
					break;
				if (!ReplaceSafely(&out, &pchar_out_string_position, 'O', maxlen))
					break;
				if (!ReplaceSafely(&out, &pchar_out_string_position, 'T', maxlen))
					break;
				if (!ReplaceSafely(&out, &pchar_out_string_position, ';', maxlen))
					break;
				break;

			case '\n':
				if (!ReplaceSafely(&out, &pchar_out_string_position, '<', maxlen))
					break;
				if (!ReplaceSafely(&out, &pchar_out_string_position, 'B', maxlen))
					break;
				if (!ReplaceSafely(&out, &pchar_out_string_position, 'R', maxlen))
					break;
				if (!ReplaceSafely(&out, &pchar_out_string_position, '>', maxlen))
					break;
				break;

			default:
				out[pchar_out_string_position++] = in[pchar_in_string_position];
				break;
			}
		}

		if (pchar_out_string_position >= maxlen)
			break;
		else
			pchar_in_string_position++;
	}

	if (pchar_out_string_position > maxlen)
	{
		pchar_out_string_position = maxlen;
	}
	for (TotalColors; TotalColors > 0;)
	{
		pchar_out_string_position += InsertStopColorSafe(&out[pchar_out_string_position], outlen - pchar_out_string_position);
	}

	out[pchar_out_string_position++] = 0;
	return pchar_out_string_position;
}

const char* GetFilenameFromFullPath(const char* Filename)
{
	while (Filename && strstr(Filename, "\\"))
		Filename = strstr(Filename, "\\") + 1;

	return Filename;
}

bool CompareTimes(char* RealTime, char* ExpectedTime)
{
	// Match everything except seconds
	// Format is: WWW MMM DD hh:mm:ss YYYY
	//            0123456789012345678901234
	//                      1         2
	if (!_strnicmp(RealTime, ExpectedTime, 17)
		&& !_strnicmp(RealTime + 19, ExpectedTime + 19, 5))
	{
		return true;
	}

	return false;
}

void AddFilter(const char* szFilter, int Length, bool& pEnabled)
{
	MQFilter* New = new MQFilter(szFilter, Length, pEnabled);

	New->pNext = gpFilters;
	gpFilters = New;
}

void DefaultFilters()
{
	AddFilter("You have become better at ", 26, gFilterSkillsIncrease);
	AddFilter("You lacked the skills to fashion the items together.", -1, gFilterSkillsAll);
	AddFilter("You have fashioned the items together to create something new!", -1, gFilterSkillsAll);
	AddFilter("You have fashioned the items together to create an alternate product.", -1, gFilterSkillsAll);
	AddFilter("You can no longer advance your skill from making this item.", -1, gFilterSkillsAll);
	AddFilter("You no longer have a target.", -1, gFilterTarget);
	AddFilter("You give ", 9, gFilterMoney);
	AddFilter("You receive ", 12, gFilterMoney);
	AddFilter("You are encumbered", 17, gFilterEncumber);
	AddFilter("You are no longer encumbered", 27, gFilterEncumber);
	AddFilter("You are low on drink", 19, gFilterFood);
	AddFilter("You are low on food", 18, gFilterFood);
	AddFilter("You are out of drink", 19, gFilterFood);
	AddFilter("You are out of food", 18, gFilterFood);
	AddFilter("You and your mount are thirsty.", -1, gFilterFood);
	AddFilter("You and your mount are hungry.", -1, gFilterFood);
	AddFilter("You are hungry", 13, gFilterFood);
	AddFilter("You are thirsty", 14, gFilterFood);
	AddFilter("You take a bite out of", 22, gFilterFood);
	AddFilter("You take a bite of", 18, gFilterFood);
	AddFilter("You take a drink from", 21, gFilterFood);
	AddFilter("Ahhh. That was tasty.", -1, gFilterFood);
	AddFilter("Ahhh. That was refreshing.", -1, gFilterFood);
	AddFilter("Chomp, chomp, chomp...", 22, gFilterFood);
	AddFilter("Glug, glug, glug...", 19, gFilterFood);
	AddFilter("You could not possibly eat any more, you would explode!", -1, gFilterFood);
	AddFilter("You could not possibly drink any more, you would explode!", -1, gFilterFood);
	AddFilter("You could not possibly consume more alcohol or become more intoxicated!", -1, gFilterFood);
}

char* ConvertHotkeyNameToKeyName(char* szName, size_t Namelen)
{
	if (!_stricmp(szName, "EQUALSIGN"))
		strcpy_s(szName, Namelen, "=");

	if (!_stricmp(szName, "SEMICOLON"))
		strcpy_s(szName, Namelen, ";");

	if (!_stricmp(szName, "LEFTBRACKET"))
		strcpy_s(szName, Namelen, "[");

	return szName;
}

// ***************************************************************************
// Function:    GetFullZone
// Description: Returns a full zone name from a short name
// ***************************************************************************
const char* GetFullZone(int ZoneID)
{
	ZoneID &= 0x7FFF;

	if (!ppWorldData || (ppWorldData && !pWorldData))
		return nullptr;

	if (ZoneID >= MAX_ZONES)
		return "UNKNOWN_ZONE";

	ZONELIST* pZone = ((WORLDDATA*)pWorldData)->ZoneArray[ZoneID];
	if (pZone)
		return pZone->LongName;

	return "UNKNOWN_ZONE";
}

// ***************************************************************************
// Function:    GetShortZone
// Description: Returns a short zone name from a ZoneID
// ***************************************************************************
const char* GetShortZone(int ZoneID)
{
	ZoneID &= 0x7FFF;

	if (!ppWorldData || (ppWorldData && !pWorldData))
		return nullptr;

	if (ZoneID >= MAX_ZONES)
		return "UNKNOWN_ZONE";

	ZONELIST* pZone = ((WORLDDATA*)pWorldData)->ZoneArray[ZoneID];
	if (pZone)
		return pZone->ShortName;

	return "UNKNOWN_ZONE";
}

// ***************************************************************************
// Function:    GetZoneID
// Description: Returns a ZoneID from a short or long zone name
// ***************************************************************************
int GetZoneID(const char* ZoneShortName)
{
	if (!ppWorldData || (ppWorldData && !pWorldData))
		return -1;

	for (int nIndex = 0; nIndex < MAX_ZONES; nIndex++)
	{
		ZONELIST* pZone = ((WORLDDATA*)pWorldData)->ZoneArray[nIndex];
		if (pZone)
		{
			if (!_stricmp(pZone->ShortName, ZoneShortName))
			{
				return nIndex;
			}

			if (!_stricmp(pZone->LongName, ZoneShortName))
			{
				return nIndex;
			}
		}
	}
	return -1;
}

// ***************************************************************************
// Function:    GetGameTime
// Description: Returns Current Game Time
// ***************************************************************************
void GetGameTime(int* Hour, int* Minute, int* Night)
{
	if (!ppWorldData || (ppWorldData && !pWorldData))
		return;

	int eqHour = ((PWORLDDATA)pWorldData)->Hour - 1; // Midnight = 1 in EQ time
	int eqMinute = ((PWORLDDATA)pWorldData)->Minute;

	if (Hour)
		*Hour = eqHour;
	if (Minute)
		*Minute = eqMinute;
	if (Night)
		*Night = ((eqHour < 7) || (eqHour > 18));
}

// ***************************************************************************
// Function:    GetGameDate
// Description: Returns Current Game Time
// ***************************************************************************
void GetGameDate(int* Month, int* Day, int* Year)
{
	if (!ppWorldData || (ppWorldData && !pWorldData))
		return;

	if (Month)
		*Month = ((PWORLDDATA)pWorldData)->Month;
	if (Day)
		*Day = ((PWORLDDATA)pWorldData)->Day;
	if (Year)
		*Year = ((PWORLDDATA)pWorldData)->Year;
}

// TOOD: Convert to data table
int GetLanguageIDByName(const char* szName)
{
	if (!_stricmp(szName, "Common")) return 1;
	if (!_stricmp(szName, "Common Tongue")) return 1;
	if (!_stricmp(szName, "Barbarian")) return 2;
	if (!_stricmp(szName, "Erudian")) return 3;
	if (!_stricmp(szName, "Elvish")) return 4;
	if (!_stricmp(szName, "Dark Elvish")) return 5;
	if (!_stricmp(szName, "Dwarvish")) return 6;
	if (!_stricmp(szName, "Troll")) return 7;
	if (!_stricmp(szName, "Ogre")) return 8;
	if (!_stricmp(szName, "Gnomish")) return 9;
	if (!_stricmp(szName, "Halfling")) return 10;
	if (!_stricmp(szName, "Thieves Cant")) return 11;
	if (!_stricmp(szName, "Old Erudian")) return 12;
	if (!_stricmp(szName, "Elder Elvish")) return 13;
	if (!_stricmp(szName, "Froglok")) return 14;
	if (!_stricmp(szName, "Goblin")) return 15;
	if (!_stricmp(szName, "Gnoll")) return 16;
	if (!_stricmp(szName, "Combine Tongue")) return 17;
	if (!_stricmp(szName, "Elder Tier'Dal")) return 18;
	if (!_stricmp(szName, "Lizardman")) return 19;
	if (!_stricmp(szName, "Orcish")) return 20;
	if (!_stricmp(szName, "Faerie")) return 21;
	if (!_stricmp(szName, "Dragon")) return 22;
	if (!_stricmp(szName, "Elder Dragon")) return 23;
	if (!_stricmp(szName, "Dark Speech")) return 24;
	if (!_stricmp(szName, "Vah Shir")) return 25;
	return -1;
}

// TOOD: Convert to data table
int GetCurrencyIDByName(char* szName)
{
	if (!_stricmp(szName, "Doubloons")) return ALTCURRENCY_DOUBLOONS;
	if (!_stricmp(szName, "Orux")) return ALTCURRENCY_ORUX;
	if (!_stricmp(szName, "Phosphenes")) return ALTCURRENCY_PHOSPHENES;
	if (!_stricmp(szName, "Phosphites")) return ALTCURRENCY_PHOSPHITES;
	if (!_stricmp(szName, "Faycitum")) return ALTCURRENCY_FAYCITES;
	if (!_stricmp(szName, "Chronobines")) return ALTCURRENCY_CHRONOBINES;
	if (!_stricmp(szName, "Silver Tokens")) return ALTCURRENCY_SILVERTOKENS;
	if (!_stricmp(szName, "Gold Tokens")) return ALTCURRENCY_GOLDTOKENS;
	if (!_stricmp(szName, "McKenzie's Special Brew")) return ALTCURRENCY_MCKENZIE;
	if (!_stricmp(szName, "Bayle Marks")) return ALTCURRENCY_BAYLE;
	if (!_stricmp(szName, "Tokens of Reclamation")) return ALTCURRENCY_RECLAMATION;
	if (!_stricmp(szName, "Brellium Tokens")) return ALTCURRENCY_BRELLIUM;
	if (!_stricmp(szName, "Dream Motes")) return ALTCURRENCY_MOTES;
	if (!_stricmp(szName, "Rebellion Chits")) return ALTCURRENCY_REBELLIONCHITS;
	if (!_stricmp(szName, "Diamond Coins")) return ALTCURRENCY_DIAMONDCOINS;
	if (!_stricmp(szName, "Bronze Fiats")) return ALTCURRENCY_BRONZEFIATS;
	if (!_stricmp(szName, "Expedient Delivery Vouchers")) return ALTCURRENCY_VOUCHER;
	if (!_stricmp(szName, "Velium Shards")) return ALTCURRENCY_VELIUMSHARDS;
	if (!_stricmp(szName, "Crystallized Fear")) return ALTCURRENCY_CRYSTALLIZEDFEAR;
	if (!_stricmp(szName, "Shadowstones")) return ALTCURRENCY_SHADOWSTONES;
	if (!_stricmp(szName, "Dreadstones")) return ALTCURRENCY_DREADSTONES;
	if (!_stricmp(szName, "Marks of Valor")) return ALTCURRENCY_MARKSOFVALOR;
	if (!_stricmp(szName, "Medals of Heroism")) return ALTCURRENCY_MEDALSOFHEROISM;
	if (!_stricmp(szName, "Commemorative Coins")) return ALTCURRENCY_COMMEMORATIVE_COINS;
	if (!_stricmp(szName, "Fists of Bayle")) return ALTCURRENCY_FISTSOFBAYLE;
	if (!_stricmp(szName, "Nobles")) return ALTCURRENCY_NOBLES;
	if (!_stricmp(szName, "Arx Energy Crystals")) return ALTCURRENCY_ENERGYCRYSTALS;
	if (!_stricmp(szName, "Pieces of Eight")) return ALTCURRENCY_PIECESOFEIGHT;
	return -1;
}

// This wrapper is here to deal with older plugins and to preserve bacwards compatability with older clients (emu)
ALTABILITY* GetAAByIdWrapper(int nAbilityId, int playerLevel)
{
	return pAltAdvManager->GetAAById(nAbilityId, playerLevel);
}

SPELL* GetSpellByAAName(const char* szName)
{
	int level = -1;

	if (SPAWNINFO* pMe = (SPAWNINFO*)pLocalPlayer)
	{
		level = pMe->Level;
	}

	for (int nAbility = 0; nAbility < NUM_ALT_ABILITIES; nAbility++)
	{
		if (ALTABILITY* pAbility = GetAAByIdWrapper(nAbility, level))
		{
			if (pAbility->SpellID != -1)
			{
				if (const char* pName = pCDBStr->GetString(pAbility->nName, eAltAbilityName))
				{
					if (!_stricmp(szName, pName))
					{
						if (SPELL* psp = GetSpellByID(pAbility->SpellID))
						{
							return psp;
						}
					}
				}
			}
		}
	}

	return nullptr;
}

int GetDeityTeamByID(int DeityID)
{
	switch (DeityID) {
	case DEITY_ErollisiMarr:
	case DEITY_MithanielMarr:
	case DEITY_RodcetNife:
	case DEITY_Quellious:
	case DEITY_Tunare:
		return 1;
	case DEITY_BrellSerilis:
	case DEITY_Bristlebane:
	case DEITY_Karana:
	case DEITY_Prexus:
	case DEITY_SolusekRo:
	case DEITY_TheTribunal:
	case DEITY_Veeshan:
		return 2;
	case DEITY_Bertoxxulous:
	case DEITY_CazicThule:
	case DEITY_Innoruuk:
	case DEITY_RallosZek:
		return 3;
	default:
		return 0;
	}
}

const char* GetGuildByID(int64_t GuildID)
{
	if (GuildID == 0 || GuildID == -1)
		return nullptr;

	if (const char* thename = pGuild->GetGuildName(GuildID))
	{
		if (!_stricmp(thename, "Unknown Guild"))
			return nullptr;

		return thename;
	}

	return nullptr;
}

int64_t GetGuildIDByName(char* szGuild)
{
	return pGuild->GetGuildIndex(szGuild);
}

const char* GetLightForSpawn(SPAWNINFO* pSpawn)
{
	uint8_t Light = pSpawn->Light;

	if (Light > LIGHT_COUNT)
		Light = 0;

	return szLights[Light];
}

// ***************************************************************************
// Function:    DistanceToSpawn3D
// Description: Return the distance between two spawns, including Z
// ***************************************************************************
float DistanceToSpawn3D(SPAWNINFO* pChar, SPAWNINFO* pSpawn)
{
	float X = pChar->X - pSpawn->X;
	float Y = pChar->Y - pSpawn->Y;
	float Z = pChar->Z - pSpawn->Z;
	return sqrtf(X * X + Y * Y + Z * Z);
}

// ***************************************************************************
// Function:    DistanceToSpawn
// Description: Return the distance between two spawns
// ***************************************************************************
float EstimatedDistanceToSpawn(SPAWNINFO* pChar, SPAWNINFO* pSpawn)
{
	float RDistance = DistanceToSpawn(pChar, pSpawn);
	float X = pChar->X - (pSpawn->X + pSpawn->SpeedX * RDistance);
	float Y = pChar->Y - (pSpawn->Y + pSpawn->SpeedY * RDistance);
	return sqrtf(X * X + Y * Y);
}

// ***************************************************************************
// Function:    ConColor
// Description: Returns the con color for a spawn's level
// ***************************************************************************
int ConColor(SPAWNINFO* pSpawn)
{
	SPAWNINFO* pChar = (SPAWNINFO*)pLocalPlayer;
	if (!pChar)
		return CONCOLOR_WHITE; // its you

	switch (pCharData->GetConLevel((PlayerClient*)pSpawn))
	{
	case 0:
	case 1:
		return CONCOLOR_GREY;
	case 2:
		return CONCOLOR_GREEN;
	case 3:
		return CONCOLOR_LIGHTBLUE;
	case 4:
		return CONCOLOR_BLUE;
	case 5:
		return CONCOLOR_WHITE;
	case 6:
		return CONCOLOR_YELLOW;
	case 7:
		return CONCOLOR_RED;
	default:
		return COLOR_PURPLE;
	}
}

CONTENTS* GetEnviroContainer()
{
	if (!pContainerMgr)
		return nullptr;

	if (!pContainerMgr->pWorldContainer.pObject)
		return nullptr;

	return pContainerMgr->pWorldContainer.pObject;
}

CContainerWnd* FindContainerForContents(CONTENTS* pContents)
{
	if (!pContainerMgr)
		return nullptr;

	for (int j = 0; j < MAX_CONTAINERS; j++)
	{
		if (pContainerMgr->pContainerWnds[j] && pContainerMgr->pContainerWnds[j]->pContents == pContents)
			return pContainerMgr->pContainerWnds[j];
	}

	return nullptr;
}

// ***************************************************************************
// FindSpeed(SPAWNINFO*) - Used to find the speed of a Spawn taking a mount into
//                               consideration.
// ***************************************************************************
float FindSpeed(SPAWNINFO* pSpawn)
{
	SPAWNINFO* pMount = nullptr;
	float fRunSpeed = 0;
	pMount = FindMount(pSpawn);

	if (pMount)
		if (!fRunSpeed)
			fRunSpeed = pMount->SpeedRun * 10000 / 70;

	return fRunSpeed;
}

void GetItemLinkHash(CONTENTS* Item, char* Buffer, size_t BufferSize)
{
	((EQ_Item*)Item)->CreateItemTagString(Buffer, BufferSize, true);
}

bool GetItemLink(CONTENTS* Item, char* Buffer, size_t BufferSize, bool Clickable)
{
	char hash[MAX_STRING] = { 0 };
	bool retVal = false;

	GetItemLinkHash(Item, hash);

	int len = strlen(hash);
	if (len > 0)
	{
		if (Clickable)
		{
			sprintf_s(Buffer, BufferSize, "%c0%s%s%c", 0x12, hash, GetItemFromContents(Item)->Name, 0x12);
		}
		else
		{
			sprintf_s(Buffer, BufferSize, "0%s%s", hash, GetItemFromContents(Item)->Name);
		}

		retVal = true;
	}

	return retVal;
}

const char* GetLoginName()
{
	if (__LoginName)
	{
		return (char*)__LoginName;
	}

	return nullptr;
}

void STMLToPlainText(char* in, char* out)
{
	uint32_t pchar_in_string_position = 0;
	uint32_t pchar_out_string_position = 0;
	uint32_t pchar_amper_string_position = 0;
	char Amper[2048] = { 0 };

	while (in[pchar_in_string_position] != 0)
	{
		switch (in[pchar_in_string_position])
		{
		case '<':
			while (in[pchar_in_string_position] != '>')
				pchar_in_string_position++;
			pchar_in_string_position++;
			break;

		case '&':
			pchar_in_string_position++;
			pchar_amper_string_position = 0;
			ZeroMemory(Amper, 2048);
			while (in[pchar_in_string_position] != ';')
			{
				Amper[pchar_amper_string_position++] = in[pchar_in_string_position++];
			}

			pchar_in_string_position++;

			if (!_stricmp(Amper, "nbsp"))
			{
				out[pchar_out_string_position++] = ' ';
			}
			else if (!_stricmp(Amper, "amp"))
			{
				out[pchar_out_string_position++] = '&';
			}
			else if (!_stricmp(Amper, "gt"))
			{
				out[pchar_out_string_position++] = '>';
			}
			else if (!_stricmp(Amper, "lt"))
			{
				out[pchar_out_string_position++] = '<';
			}
			else if (!_stricmp(Amper, "quot"))
			{
				out[pchar_out_string_position++] = '\"';
			}
			else if (!_stricmp(Amper, "pct"))
			{
				out[pchar_out_string_position++] = '%';
			}
			else
			{
				out[pchar_out_string_position++] = '?';
			}
			break;

		default:
			out[pchar_out_string_position++] = in[pchar_in_string_position++];
		}
	}

	out[pchar_out_string_position++] = 0;
}

void ClearSearchItem(MQItemSearch& SearchItem)
{
	SearchItem = MQItemSearch();
}

#define MaskSet(n) (SearchItem.FlagMask[(SearchItemFlag)n])
#define Flag(n) (SearchItem.Flag[(SearchItemFlag)n])
#define RequireFlag(flag,value) { if (MaskSet(flag) && Flag(flag) != (char)((value)!=0)) return false;}

bool ItemMatchesSearch(MQItemSearch& SearchItem, CONTENTS* pContents)
{
	ITEMINFO* pItem = GetItemFromContents(pContents);

	if (SearchItem.ID && pItem->ItemNumber != SearchItem.ID)
		return false;

	RequireFlag(Lore, pItem->Lore);
	RequireFlag(NoRent, pItem->NoRent);
	RequireFlag(NoDrop, pItem->NoDrop);
	RequireFlag(Magic, pItem->Magic);
	RequireFlag(Pack, pItem->Type == ITEMTYPE_PACK);
	RequireFlag(Book, pItem->Type == ITEMTYPE_BOOK);
	RequireFlag(Combinable, pItem->ItemType == 17);
	RequireFlag(Summoned, pItem->Summoned);
	RequireFlag(Instrument, pItem->InstrumentType);
	RequireFlag(Weapon, pItem->Damage && pItem->Delay);
	RequireFlag(Normal, pItem->Type == ITEMTYPE_NORMAL);

	if (SearchItem.szName[0] && ci_find_substr(pItem->Name, SearchItem.szName))
		return false;

	return true;
}

bool SearchThroughItems(MQItemSearch& SearchItem, CONTENTS** pResult, DWORD* nResult)
{
	// TODO
#define DoResult(pContents, nresult) { \
	if (pResult)                       \
		*pResult = pContents;          \
	if (nResult)                       \
		*nResult = nresult;            \
	return true;                       \
}

	if (PcProfile* pProfile = GetPcProfile())
	{
		if (pProfile->pInventoryArray)
		{
			if (MaskSet(Worn) && Flag(Worn))
			{
				// iterate through worn items
				for (int N = 0; N < NUM_WORN_ITEMS; N++)
				{
					if (CONTENTS* pContents = pProfile->pInventoryArray->InventoryArray[N])
					{
						if (ItemMatchesSearch(SearchItem, pContents)) {
							DoResult(pContents, N);
						}
					}
				}
			}

			if (MaskSet(Inventory) && Flag(Inventory))
			{
				// iterate through inventory slots before in-pack slots
				for (int nPack = 0; nPack < NUM_INV_BAG_SLOTS; nPack++)
				{
					if (CONTENTS* pContents = pProfile->pInventoryArray->Inventory.Pack[nPack])
					{
						if (ItemMatchesSearch(SearchItem, pContents))
							DoResult(pContents, nPack + 21);
					}
				}

				for (int nPack = 0; nPack < NUM_INV_BAG_SLOTS; nPack++)
				{
					if (CONTENTS* pContents = pProfile->pInventoryArray->Inventory.Pack[nPack])
					{
						if (GetItemFromContents(pContents)->Type == ITEMTYPE_PACK && pContents->Contents.ContainedItems.Capacity)
						{
							for (int nItem = 0; nItem < GetItemFromContents(pContents)->Slots; nItem++)
							{
								if (CONTENTS* pItem = pContents->GetContent(nItem))
								{
									if (ItemMatchesSearch(SearchItem, pItem))
										DoResult(pItem, nPack * 100 + nItem);
								}
							}
						}
					}
				}
			}
		}
	}

	// TODO
	return false;
}
#undef DoResult
#undef RequireFlag
#undef Flag
#undef MaskSet

void ClearSearchSpawn(MQSpawnSearch* pSearchSpawn)
{
	if (!pSearchSpawn) return;

	*pSearchSpawn = MQSpawnSearch();

	if (pCharSpawn)
		pSearchSpawn->zLoc = ((SPAWNINFO*)pCharSpawn)->Z;
	else if (pLocalPlayer)
		pSearchSpawn->zLoc = ((SPAWNINFO*)pLocalPlayer)->Z;
}

// ***************************************************************************
// Function:    DistanceToPoint
// Description: Return the distance between a spawn and the specified point
// ***************************************************************************
float DistanceToPoint(SPAWNINFO* pSpawn, float xLoc, float yLoc)
{
	float X = pSpawn->X - xLoc;
	float Y = pSpawn->Y - yLoc;
	return sqrtf(X * X + Y * Y);
}

// ***************************************************************************
// Function:    Distance3DToPoint
// Description: Return the distance between a spawn and the specified point
// ***************************************************************************
float Distance3DToPoint(SPAWNINFO* pSpawn, float xLoc, float yLoc, float zLoc)
{
	float dX = pSpawn->X - xLoc;
	float dY = pSpawn->Y - yLoc;
	float dZ = pSpawn->Z - zLoc;
	return sqrtf(dX * dX + dY * dY + dZ * dZ);
}

void DisplayOverlayText(const char* szText, int dwColor, uint32_t dwTransparency, uint32_t msFadeIn, uint32_t msFadeOut, uint32_t msHold)
{
	CBroadcast* pBC = GetTextOverlay();
	if (!pBC)
	{
		WriteChatColor(szText, dwColor);
		return;
	}

	uint32_t dwAlpha = (uint32_t)(dwTransparency * 255 / 100);
	if (dwAlpha > 255) dwAlpha = 255;

	((CTextOverlay*)pBC)->DisplayText(
		szText,
		dwColor,
		10, // Always 10 in eqgame.exe,
			// Doesn't seem to affect anything
			// (tried 0,1,10,20,100,500)
		dwAlpha,
		msFadeIn,
		msFadeOut,
		msHold);
}

void CustomPopup(char* szPopText, bool bPopOutput)
{
	int iArgNum = 1;
	int iMsgColor = CONCOLOR_LIGHTBLUE;
	int iMsgTime = 3000;
	char szCurArg[MAX_STRING] = { 0 };
	char szPopupMsg[MAX_STRING] = { 0 };
	char szErrorCust[MAX_STRING] = "\awUsage: /popcustom [\agcolor\ax] [\agdisplaytime\ax(in seconds)] [\agmessage\ax]";
	char szErrorEcho[MAX_STRING] = "\awUsage: /popupecho [\agcolor\ax] [\agdisplaytime\ax(in seconds)] [\agmessage\ax]";

	GetArg(szCurArg, szPopText, iArgNum++);
	if (!*szCurArg)
	{
		if (bPopOutput)
		{
			WriteChatf("%s", szErrorEcho);
		}
		else
		{
			WriteChatf("%s", szErrorCust);
		}
		return;
	}
	else
	{
		if (isdigit(szCurArg[0]))
		{
			iMsgColor = atoi(szCurArg);
			GetArg(szCurArg, szPopText, iArgNum++);
			if (isdigit(szCurArg[0]))
			{
				iMsgTime = atoi(szCurArg) * 1000;
				sprintf_s(szPopupMsg, "%s", GetNextArg(szPopText, 2, false, 0));
			}
			else
			{
				sprintf_s(szPopupMsg, "%s", GetNextArg(szPopText, 1, false, 0));
			}
		}
		else
		{
			strcpy_s(szPopupMsg, szPopText);
		}
	}

	DisplayOverlayText(szPopupMsg, iMsgColor, 100, 500, 500, iMsgTime);
	if (bPopOutput)
		WriteChatf("\ayPopup\aw:: %s", szPopupMsg);
}

bool ParseKeyCombo(const char* text, KeyCombo& Dest)
{
	KeyCombo Ret;
	if (!_stricmp(text, "clear"))
	{
		Dest = Ret;
		return true;
	}

	char Copy[MAX_STRING];
	strcpy_s(Copy, text);
	char* token1 = nullptr;
	char* next_token1 = nullptr;

	token1 = strtok_s(Copy, "+ ", &next_token1);
	while (token1 != nullptr)
	{
		if (token1 != nullptr)
		{
			if (!_stricmp(token1, "alt"))
				Ret.Data[0] = 1;
			else if (!_stricmp(token1, "ctrl"))
				Ret.Data[1] = 1;
			else if (!_stricmp(token1, "shift"))
				Ret.Data[2] = 1;
			else
			{
				for (int i = 0; gDiKeyID[i].Id; i++)
				{
					if (!_stricmp(token1, gDiKeyID[i].szName))
					{
						Ret.Data[3] = (char)gDiKeyID[i].Id;
						break;
					}
				}
			}
			token1 = strtok_s(nullptr, "+ ", &next_token1);
		}
	}
	if (Ret.Data[3])
	{
		Dest = Ret;
		return true;
	}
	return false;
}

char* DescribeKeyCombo(KeyCombo& Combo, char* szDest, size_t BufferSize)
{
	int pos = 0;
	szDest[0] = 0;

	if (Combo.Data[2])
	{
		strcpy_s(&szDest[pos], BufferSize - pos, "shift");
		pos += 5;
	}

	if (Combo.Data[1])
	{
		if (pos)
		{
			szDest[pos] = '+';
			pos++;
		}
		strcpy_s(&szDest[pos], BufferSize - pos, "ctrl");
		pos += 4;
	}

	if (Combo.Data[0])
	{
		if (pos)
		{
			szDest[pos] = '+';
			pos++;
		}
		strcpy_s(&szDest[pos], BufferSize - pos, "alt");
		pos += 3;
	}

	if (pos)
	{
		szDest[pos] = '+';
		pos++;
	}

	if (Combo.Data[3])
	{
		strcpy_s(&szDest[pos], BufferSize - pos, gDiKeyName[Combo.Data[3]]);
	}
	else
	{
		strcpy_s(&szDest[pos], BufferSize - pos, "clear");
	}

	return &szDest[0];
}

bool LoadCfgFile(const char* Filename, bool Delayed)
{
	FILE* file = nullptr;
	errno_t err = 0;

	char szFilename[MAX_STRING] = { 0 };
	strcpy_s(szFilename, Filename);
	if (!strchr(szFilename, '.'))
		strcat_s(szFilename, ".cfg");

	char szFull[MAX_STRING] = { 0 };
#define TryFile(name)  \
    {\
	if((err = fopen_s(&file,name,"rt"))==0)\
    goto havecfgfile;\
    }
	sprintf_s(szFull, "%s\\Configs\\%s", gszINIPath, szFilename);
	TryFile(szFull);
	sprintf_s(szFull, "%s\\%s", gszINIPath, szFilename);
	TryFile(szFull);
	TryFile(szFilename);
	TryFile(Filename);
#undef TryFile
	return false;
havecfgfile:
	char szBuffer[MAX_STRING] = { 0 };
	while (fgets(szBuffer, MAX_STRING, file))
	{
		char* Next_Token1 = 0;
		char* Cmd = strtok_s(szBuffer, "\r\n", &Next_Token1);
		if (Cmd && Cmd[0] && Cmd[0] != ';')
		{
			HideDoCommand(((SPAWNINFO*)pLocalPlayer), Cmd, Delayed);
		}
	}
	fclose(file);
	return true;
}

int FindInvSlotForContents(CONTENTS* pContents)
{
	int LastMatch = -1;

	// screw the old style InvSlot numbers
	// return the index into the INVSLOTMGR array
	DebugSpew("FindInvSlotForContents(0x%08X) (0x%08X)", pContents, GetItemFromContents(pContents));

	for (int index = 0; index < MAX_INV_SLOTS; index++)
	{
		CONTENTS* pC = nullptr;

		if (pInvSlotMgr->SlotArray[index])
		{
			CInvSlot* pCIS = pInvSlotMgr->SlotArray[index];
			pCIS->GetItemBase(&pC);

			if (pC)
			{
				DebugSpew("pInvSlotMgr->SlotArray[%d] Contents==0x%08X", index, pC);

				if (pC == pContents)
				{
					CInvSlot* pInvSlot = pInvSlotMgr->SlotArray[index];

					if (pInvSlot->pInvSlotWnd)
					{
						DebugSpew("%d slot %d wnd %d %d %d", index,
							pInvSlot->Index,
							pInvSlot->pInvSlotWnd->ItemLocation.GetLocation(),
							pInvSlot->pInvSlotWnd->ItemLocation.GetIndex().GetSlot(0),
							pInvSlot->pInvSlotWnd->ItemLocation.GetIndex().GetSlot(1));
					}

					if (pInvSlot->pInvSlotWnd
						&& pInvSlot->pInvSlotWnd->ItemLocation.GetLocation() == eItemContainerPossessions)
					{
						return pInvSlot->Index;
					}

					if (pInvSlot->pInvSlotWnd && pInvSlot->pInvSlotWnd->ItemLocation.GetIndex().GetSlot(1) != -1)
					{
						return pInvSlot->Index;
					}

					if (pInvSlot->pInvSlotWnd
						&& pInvSlot->pInvSlotWnd->ItemLocation.GetLocation() == eItemContainerCorpse)
					{
						// loot window items should not be anywhere else
						return pInvSlot->Index;
					}

					LastMatch = index;
				}
			}
		}
	}

	// return specific window type if needed
	if (LastMatch != -1 && pInvSlotMgr->SlotArray[LastMatch]->pInvSlotWnd->ItemLocation.GetLocation() == 9999)
		return  pInvSlotMgr->SlotArray[LastMatch]->Index;

	return -1;
}

int LastFoundInvSlot = -1;

int FindInvSlot(const char* Name, bool Exact)
{
	char szTemp[MAX_STRING] = { 0 };

	for (int nSlot = 0; nSlot < MAX_INV_SLOTS; nSlot++)
	{
		if (pInvSlotMgr->SlotArray[nSlot])
		{
			CInvSlot* x = pInvSlotMgr->SlotArray[nSlot];
			CONTENTS* y = nullptr;

			if (x)
			{
				x->GetItemBase(&y);
			}

			if (y)
			{
				ITEMINFO* pItem = GetItemFromContents(y);

				if (!Exact)
				{
					if (ci_find_substr(pItem->Name, Name) != -1)
					{
						if (pInvSlotMgr->SlotArray[nSlot]->pInvSlotWnd)
						{
							LastFoundInvSlot = nSlot;
							return pInvSlotMgr->SlotArray[nSlot]->Index;
						}

						// let it try to find it in an open slot if this fails
					}
				}
				else if (ci_equals(pItem->Name, Name))
				{
					if (pInvSlotMgr->SlotArray[nSlot]->pInvSlotWnd)
					{
						LastFoundInvSlot = nSlot;
						return pInvSlotMgr->SlotArray[nSlot]->Index;
					}

					// let it try to find it in an open slot if this fails
				}

			}
		}
	}

	LastFoundInvSlot = -1;
	return -1;
}

int FindNextInvSlot(const char* pName, bool Exact)
{
	char szTemp[MAX_STRING] = { 0 };
	char Name[MAX_STRING] = { 0 };
	strcpy_s(Name, pName);
	_strlwr_s(Name);

#if 0 // FIXME
	PEQINVSLOTMGR pInvMgr = (PEQINVSLOTMGR)pInvSlotMgr;
	for (int N = LastFoundInvSlot + 1; N < MAX_INV_SLOTS; N++)
	{
		if (pInvMgr->SlotArray[N])
		{
			if (pInvMgr->SlotArray[N]->ppContents && *pInvMgr->SlotArray[N]->ppContents)
			{
				if (!Exact)
				{
					__strlwr_s(strcpy_s(szTemp, (*pInvMgr->SlotArray[N]->ppContents)->Item->Name));
					if (strstr(szTemp, Name))
					{
						if (pInvMgr->SlotArray[N]->pInvSlotWnd)
						{
							LastFoundInvSlot = N;
							return pInvMgr->SlotArray[N]->pInvSlotWnd->InvSlot;
						}
						// let it try to find it in an open slot if this fails
					}
				}
				else if (!_stricmp(Name, (*pInvMgr->SlotArray[N]->ppContents)->Item->Name))
				{
					if (pInvMgr->SlotArray[N]->pInvSlotWnd)
					{
						LastFoundInvSlot = N;
						return pInvMgr->SlotArray[N]->pInvSlotWnd->InvSlot;
					}
					// let it try to find it in an open slot if this fails
				}

			}
		}
	}
#endif
	LastFoundInvSlot = -1;
	return -1;
}

enum eCalcOp
{
	CO_NUMBER = 0,
	CO_OPENPARENS = 1,
	CO_CLOSEPARENS = 2,
	CO_ADD = 3,
	CO_SUBTRACT = 4,
	CO_MULTIPLY = 5,
	CO_DIVIDE = 6,
	CO_IDIVIDE = 7,
	CO_LAND = 8,
	CO_AND = 9,
	CO_LOR = 10,
	CO_OR = 11,
	CO_XOR = 12,
	CO_EQUAL = 13,
	CO_NOTEQUAL = 14,
	CO_GREATER = 15,
	CO_NOTGREATER = 16,
	CO_LESS = 17,
	CO_NOTLESS = 18,
	CO_MODULUS = 19,
	CO_POWER = 20,
	CO_LNOT = 21,
	CO_NOT = 22,
	CO_SHL = 23,
	CO_SHR = 24,
	CO_NEGATE = 25,
	CO_TOTAL = 26,
};

int CalcOpPrecedence[CO_TOTAL] =
{
	0,
	0,
	0,
	9,    // add
	9,    // subtract
	10,   // multiply
	10,   // divide
	10,   // integer divide
	2,    // logical and
	5,    // bitwise and
	1,    // logical or
	3,    // bitwise or
	4,    // bitwise xor
	6,    // equal
	6,    // not equal
	7,    // greater
	7,    // not greater
	7,    // less
	7,    // not less
	10,   // modulus
	11,   // power
	12,   // logical not
	12,   // bitwise not
	8,    // shl
	8,    // shr
	12,   // negate
};

struct CalcOp
{
	eCalcOp Op;
	double Value;
};

bool EvaluateRPN(CalcOp* pList, int Size, double& Result)
{
	if (!Size)
		return false;

	std::unique_ptr<double[]> stackPtr = std::make_unique<double[]>(Size / 2 + 2);
	double* pStack = stackPtr.get();

	int nStack = 0;

#define StackEmpty()           (nStack==0)
#define StackTop()             (pStack[nStack])
#define StackSetTop(do_assign) {pStack[nStack]##do_assign;}
#define StackPush(val)         {nStack++;pStack[nStack]=val;}
#define StackPop()             {if (!nStack) {FatalError("Illegal arithmetic in calculation"); return 0;}; nStack--;}

#define BinaryIntOp(op)        {int RightSide=(int)StackTop();StackPop();StackSetTop(=(double)(((int)StackTop())##op##RightSide));}
#define BinaryOp(op)           {double RightSide=StackTop();StackPop();StackSetTop(=StackTop()##op##RightSide);}
#define BinaryAssign(op)       {double RightSide=StackTop();StackPop();StackSetTop(##op##=RightSide);}

#define UnaryIntOp(op)         {StackSetTop(=op##((int)StackTop()));}
#define UnaryOp(op)            {StackSetTop(=op##(StackTop()));}

	for (int i = 0; i < Size; i++)
	{
		switch (pList[i].Op)
		{
		case CO_NUMBER:
			StackPush(pList[i].Value);
			break;
		case CO_ADD:
			BinaryAssign(+);
			break;
		case CO_MULTIPLY:
			BinaryAssign(*);
			break;
		case CO_SUBTRACT:
			BinaryAssign(-);
			break;
		case CO_NEGATE:
			UnaryOp(-);
			break;
		case CO_DIVIDE:
			if (StackTop())
			{
				BinaryAssign(/ );
			}
			else
			{
				//printf("Divide by zero error\n");
				FatalError("Divide by zero in calculation");
				return false;
			}
			break;

		case CO_IDIVIDE://TODO: SPECIAL HANDLING
		{
			int Right = (int)StackTop();
			if (Right)
			{
				StackPop();
				int Left = (int)StackTop();
				Left /= Right;
				StackSetTop(= Left);
			}
			else
			{
				//printf("Integer divide by zero error\n");
				FatalError("Divide by zero in calculation");
				return false;
			}
		}
		break;

		case CO_MODULUS://TODO: SPECIAL HANDLING
		{
			int Right = (int)StackTop();
			if (Right)
			{
				StackPop();
				int Left = (int)StackTop();
				Left %= Right;
				StackSetTop(= Left);
			}
			else
			{
				//printf("Modulus by zero error\n");
				FatalError("Modulus by zero in calculation");
				return false;
			}
		}
		break;

		case CO_LAND:
			BinaryOp(&&);
			break;
		case CO_LOR:
			BinaryOp(|| );
			break;
		case CO_EQUAL:
			BinaryOp(== );
			break;
		case CO_NOTEQUAL:
			BinaryOp(!= );
			break;
		case CO_GREATER:
			BinaryOp(> );
			break;
		case CO_NOTGREATER:
			BinaryOp(<= );
			break;
		case CO_LESS:
			BinaryOp(< );
			break;
		case CO_NOTLESS:
			BinaryOp(>= );
			break;
		case CO_SHL:
			BinaryIntOp(<< );
			break;
		case CO_SHR:
			BinaryIntOp(>> );
			break;
		case CO_AND:
			BinaryIntOp(&);
			break;
		case CO_OR:
			BinaryIntOp(| );
			break;
		case CO_XOR:
			BinaryIntOp(^);
			break;
		case CO_LNOT:
			UnaryIntOp(!);
			break;
		case CO_NOT:
			UnaryIntOp(~);
			break;
		case CO_POWER:
		{
			double RightSide = StackTop();
			StackPop();
			StackSetTop(= pow(StackTop(), RightSide));
		}
		break;
		}
	}

	Result = StackTop();

#undef StackEmpty
#undef StackTop
#undef StackPush
#undef StackPop

	return true;
}

bool FastCalculate(char* szFormula, double& Result)
{
	//DebugSpew("FastCalculate(%s)",szFormula);
	if (!szFormula || !szFormula[0])
		return false;

	int Length = (int)strlen(szFormula);
	int MaxOps = (Length + 1);

	std::unique_ptr<CalcOp[]> OpsList = std::make_unique<CalcOp[]>(MaxOps);
	CalcOp* pOpList = OpsList.get();
	memset(pOpList, 0, sizeof(CalcOp) * MaxOps);

	std::unique_ptr<eCalcOp[]> Stack = std::make_unique<eCalcOp[]>(MaxOps);
	eCalcOp* pStack = Stack.get();
	memset(pStack, 0, sizeof(eCalcOp) * MaxOps);

	int nOps = 0;
	int nStack = 0;
	char* pEnd = szFormula + Length;
	char CurrentToken[MAX_STRING] = { 0 };
	char* pToken = &CurrentToken[0];

#define OpToList(op)         { pOpList[nOps].Op = op; nOps++; }
#define ValueToList(val)     { pOpList[nOps].Value = val; nOps++; }
#define StackEmpty()         (nStack == 0)
#define StackTop()           (pStack[nStack])
#define StackPush(op)        { nStack++; pStack[nStack] = op; }
#define StackPop()           { if (!nStack) { FatalError("Illegal arithmetic in calculation"); return 0; } nStack--;}
#define HasPrecedence(a,b)   ( CalcOpPrecedence[a] >= CalcOpPrecedence[b])
#define MoveStack(op) {                                                                        \
	while (!StackEmpty() && StackTop() != CO_OPENPARENS && HasPrecedence(StackTop(), op)) {    \
		OpToList(StackTop());                                                                  \
		StackPop();                                                                            \
	}                                                                                          \
}
#define FinishString()       { if (pToken != &CurrentToken[0]) { *pToken = 0; ValueToList(atof(CurrentToken)); pToken = &CurrentToken[0]; *pToken=0; }}
#define NewOp(op)            { FinishString(); MoveStack(op); StackPush(op); }
#define NextChar(ch)         { *pToken = ch; pToken++; }

	bool WasParen = false;
	for (char* pCur = szFormula; pCur < pEnd; pCur++)
	{
		switch (*pCur)
		{
		case ' ':
			continue;
		case '(':
			FinishString();
			StackPush(CO_OPENPARENS);
			break;
		case ')':
			FinishString();
			while (StackTop() != CO_OPENPARENS)
			{
				OpToList(StackTop());
				StackPop();
			}
			StackPop();
			WasParen = true;
			continue;
		case '+':
			if (pCur[1] != '+')
				NewOp(CO_ADD);
			break;
		case '-':
			if (pCur[1] == '-')
			{
				pCur++;
				NewOp(CO_ADD);
			}
			else
			{
				if (CurrentToken[0] || WasParen)
				{
					NewOp(CO_SUBTRACT);
				}
				else
					NewOp(CO_NEGATE);
			}
			break;
		case '*':
			NewOp(CO_MULTIPLY);
			break;
		case '\\':
			NewOp(CO_IDIVIDE);
			break;
		case '/':
			NewOp(CO_DIVIDE);
			break;
		case '|':
			if (pCur[1] == '|')
			{
				// Logical OR
				++pCur;
				NewOp(CO_LOR);
			}
			else
			{
				// Bitwise OR
				NewOp(CO_OR);
			}
			break;
		case '%':
			NewOp(CO_MODULUS);
			break;
		case '~':
			NewOp(CO_NOT);
			break;
		case '&':
			if (pCur[1] == '&')
			{
				// Logical AND
				++pCur;
				NewOp(CO_LAND);
			}
			else
			{
				// Bitwise AND
				NewOp(CO_AND);
			}
			break;
		case '^':
			if (pCur[1] == '^')
			{
				// XOR
				++pCur;
				NewOp(CO_XOR);
			}
			else
			{
				// POWER
				NewOp(CO_POWER);
			}
			break;
		case '!':
			if (pCur[1] == '=')
			{
				++pCur;
				NewOp(CO_NOTEQUAL);
			}
			else
			{
				NewOp(CO_LNOT);
			}
			break;
		case '=':
			if (pCur[1] == '=')
			{
				++pCur;
				NewOp(CO_EQUAL);
			}
			else
			{
				//printf("Unparsable: '%c'\n",*pCur);
				// error
				return false;
			}
			break;
		case '<':
			if (pCur[1] == '=')
			{
				++pCur;
				NewOp(CO_NOTGREATER);
			}
			else if (pCur[1] == '<')
			{
				++pCur;
				NewOp(CO_SHL);
			}
			else
			{
				NewOp(CO_LESS);
			}
			break;
		case '>':
			if (pCur[1] == '=')
			{
				++pCur;
				NewOp(CO_NOTLESS);
			}
			else if (pCur[1] == '>')
			{
				++pCur;
				NewOp(CO_SHR);
			}
			else
			{
				NewOp(CO_GREATER);
			}
			break;
		case '.':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
		case '0':
			NextChar(*pCur);
			break;
		default:
		{
			//printf("Unparsable: '%c'\n",*pCur);
			FatalError("Unparsable in Calculation: '%c'", *pCur);
			// unparsable
			return false;
		}
		break;
		}
		WasParen = false;
	}
	FinishString();

	while (!StackEmpty())
	{
		OpToList(StackTop());
		StackPop();
	}

	return EvaluateRPN(pOpList, nOps, Result);
}

bool Calculate(const char* szFormula, double& Result)
{
	char Buffer[MAX_STRING] = { 0 };
	strcpy_s(Buffer, szFormula);
	_strupr_s(Buffer);

	while (char* pNull = strstr(Buffer, "NULL"))
	{
		pNull[0] = '0';
		pNull[1] = '.';
		pNull[2] = '0';
		pNull[3] = '0';
	}

	while (char* pTrue = strstr(Buffer, "TRUE"))
	{
		pTrue[0] = '1';
		pTrue[1] = '.';
		pTrue[2] = '0';
		pTrue[3] = '0';
	}

	while (char* pFalse = strstr(Buffer, "FALSE"))
	{
		pFalse[0] = '0';
		pFalse[1] = '.';
		pFalse[2] = '0';
		pFalse[3] = '0';
		pFalse[4] = '0';
	}

	bool Ret;
	Benchmark(bmCalculate, Ret = FastCalculate(Buffer, Result));
	return Ret;
}

bool PlayerHasAAAbility(int AAIndex)
{
	for (int i = 0; i < AA_CHAR_MAX_REAL; i++)
	{
		if (pPCData->GetAlternateAbilityId(i) == AAIndex)
			return true;
	}
	return false;
}

#if 0
const char* GetAANameByIndex(int AAIndex)
{
	for (int nAbility = 0; nAbility < NUM_ALT_ABILITIES_ARRAY; nAbility++)
	{
		if (((PALTADVMGR)pAltAdvManager)->AltAbilities->AltAbilityList->Abilities[nAbility])
		{
			if (ALTABILITY* pAbility = ((PALTADVMGR)pAltAdvManager)->AltAbilities->AltAbilityList->Abilities[nAbility]->Ability)
			{
				if (pAbility->Index == AAIndex)
				{
					return pStringTable->getString(pAbility->nName, 0);
				}
			}
		}
	}
	return "AA Not Found";
}
#endif

int GetAAIndexByName(const char* AAName)
{
	int level = -1;
	if (SPAWNINFO* pMe = (SPAWNINFO*)pLocalPlayer)
	{
		level = pMe->Level;
	}

	// check bought aa's first
	for (int nAbility = 0; nAbility < AA_CHAR_MAX_REAL; nAbility++)
	{
		if (ALTABILITY* pAbility = GetAAByIdWrapper(pPCData->GetAlternateAbilityId(nAbility), level))
		{
			if (const char* pName = pCDBStr->GetString(pAbility->nName, eAltAbilityName))
			{
				if (!_stricmp(AAName, pName))
				{
					return pAbility->Index;
				}
			}
		}
	}

	// not found? fine lets check them all then...
	for (int nAbility = 0; nAbility < NUM_ALT_ABILITIES; nAbility++)
	{
		if (ALTABILITY* pAbility = GetAAByIdWrapper(nAbility, level))
		{
			if (const char* pName = pCDBStr->GetString(pAbility->nName, eAltAbilityName))
			{
				if (!_stricmp(AAName, pName))
				{
					return pAbility->Index;
				}
			}
		}
	}

	return 0;
}

int GetAAIndexByID(int ID)
{
	// check our bought aa's first
	for (int nAbility = 0; nAbility < AA_CHAR_MAX_REAL; nAbility++)
	{
		if (ALTABILITY* pAbility = GetAAByIdWrapper(pPCData->GetAlternateAbilityId(nAbility)))
		{
			if (pAbility->ID == ID)
			{
				return pAbility->Index;
			}
		}
	}

	// didnt find it? fine we go through them all then...
	for (int nAbility = 0; nAbility < NUM_ALT_ABILITIES; nAbility++)
	{
		if (ALTABILITY* pAbility = GetAAByIdWrapper(nAbility))
		{
			if (pAbility->ID == ID)
			{
				return pAbility->Index;
			}
		}
	}

	return 0;
}

bool IsPCNear(SPAWNINFO* pSpawn, float Radius)
{
	SPAWNINFO* pClose = nullptr;
	if (ppSpawnManager && pSpawnList)
	{
		pClose = (SPAWNINFO*)pSpawnList;
	}

	while (pClose)
	{
		if (!IsInGroup(pClose) && (pClose->Type == SPAWN_PLAYER))
		{
			if ((pClose != pSpawn) && (Distance3DToSpawn(pClose, pSpawn) < Radius))
				return true;
		}
		pClose = pClose->pNext;
	}
	return false;
}

bool IsInGroup(SPAWNINFO* pSpawn, bool bCorpse)
{
	CHARINFO* pChar = GetCharInfo();
	if (!pChar->pGroupInfo)
		return false;
	if (pSpawn == pChar->pSpawn)
		return true;

	for (int i = 1; i < 6; i++)
	{
		GROUPMEMBER* pMember = pChar->pGroupInfo->pMember[i];

		if (pMember)
		{
			if (!bCorpse)
			{
				if (!_stricmp(pMember->Name.c_str(), pSpawn->Name))
				{
					return true;
				}
			}
			else
			{
				char szSearch[256] = { 0 };
				strcpy_s(szSearch, pMember->Name.c_str());
				strcat_s(szSearch, "'s corpse");

				if (!_strnicmp(pSpawn->Name, szSearch, strlen(szSearch)))
				{
					return true;
				}
			}
		}
	}

	return false;
}

bool IsInRaid(SPAWNINFO* pSpawn, bool bCorpse)
{

	if (pSpawn == GetCharInfo()->pSpawn)
		return true;

	size_t l = strlen(pSpawn->Name);

	for (int i = 0; i < MAX_RAID_SIZE; i++)
	{
		if (!bCorpse)
		{
			if (!_strnicmp(pRaid->RaidMember[i].Name, pSpawn->Name, l + 1)
				&& pRaid->RaidMember[i].nClass == pSpawn->mActorClient.Class)
			{
				return true;
			}
		}
		else
		{
			char szSearch[256] = { 0 };
			strcpy_s(szSearch, pRaid->RaidMember[i].Name);
			strcat_s(szSearch, "'s corpse");

			size_t l = strlen(szSearch);
			if (!_strnicmp(szSearch, pSpawn->Name, l)
				&& pRaid->RaidMember[i].nClass == pSpawn->mActorClient.Class)
			{
				return true;
			}
		}
	}

	return false;
}

bool IsInFellowship(SPAWNINFO* pSpawn, bool bCorpse)
{
	if (CHARINFO* pChar = GetCharInfo())
	{
		if (!pChar->pSpawn)
			return false;

		FELLOWSHIPINFO Fellowship = (FELLOWSHIPINFO)pChar->pSpawn->Fellowship;

		for (int i = 0; i < Fellowship.Members; i++)
		{
			if (!bCorpse)
			{
				if (!_stricmp(Fellowship.FellowshipMember[i].Name, pSpawn->Name))
				{
					return true;
				}
			}
			else
			{
				char szSearch[256] = { 0 };
				strcpy_s(szSearch, Fellowship.FellowshipMember[i].Name);
				strcat_s(szSearch, "'s corpse");

				if (!_strnicmp(szSearch, pSpawn->Name, strlen(szSearch))
					&& Fellowship.FellowshipMember[i].Class == pSpawn->mActorClient.Class)
				{
					return true;
				}
			}
		}
	}

	return false;
}

bool IsNamed(SPAWNINFO* pSpawn)
{
	if (pSpawn)
	{
		char szTemp[MAX_STRING] = { 0 };

		if (GetSpawnType(pSpawn) != NPC)
			return false;
		if (!IsTargetable(pSpawn))
			return false;
		if (pSpawn->mActorClient.Class >= 20 && pSpawn->mActorClient.Class <= 35)  // NPC GMs
			return false;
		if (pSpawn->mActorClient.Class == 40)  // NPC bankers
			return false;
		if (pSpawn->mActorClient.Class == 41 || pSpawn->mActorClient.Class == 70)  // NPC/Quest/TBS merchants
			return false;
		if (pSpawn->mActorClient.Class == 60 || pSpawn->mActorClient.Class == 61)  //Ldon Merchants/Recruiters
			return false;
		if (pSpawn->mActorClient.Class == 62)  // Destructible Objects
			return false;
		if (pSpawn->mActorClient.Class == 63 || pSpawn->mActorClient.Class == 64 || pSpawn->mActorClient.Class == 74)  // Tribute Master/Guild Tribute Master/Personal Tribute Master
			return false;
		if (pSpawn->mActorClient.Class == 66)  // Guild Banker
			return false;
		if (pSpawn->mActorClient.Class == 67 || pSpawn->mActorClient.Class == 68)  //Don Merchants (Norrath's Keepers/Dark Reign)
			return false;
		if (pSpawn->mActorClient.Class == 69)  // Fellowship Registrar
			return false;
		if (pSpawn->mActorClient.Class == 71)  // Mercenary Liason
			return false;

		strcpy_s(szTemp, pSpawn->Name);
		char* Next_Token1 = nullptr;

		if (char* Cmd = strtok_s(szTemp, " ", &Next_Token1))
		{
			// Checking for mobs that have 'A' or 'An' as their first name
			if (Cmd[0] == 'A')
			{
				if (Cmd[1] == '_')
					return false;
				else if (Cmd[1] == 'n')
					if (Cmd[2] == '_')
						return false;
			}

			if (!gUseNewNamedTest)
			{
				if (!_strnicmp(Cmd, "Guard", 5)
					|| !_strnicmp(Cmd, "Defender", 8)
					|| !_strnicmp(Cmd, "Soulbinder", 10)
					|| !_strnicmp(Cmd, "Aura", 4)
					|| !_strnicmp(Cmd, "Sage", 4)
					//|| !_strnicmp(szTemp,"High_Priest", 11)
					|| !_strnicmp(Cmd, "Ward", 4)
					//|| !_strnicmp(szTemp,"Shroudkeeper", 12)
					|| !_strnicmp(Cmd, "Eye of", 6)
					|| !_strnicmp(Cmd, "Imperial_Crypt", 14)
					|| !_strnicmp(Cmd, "Diaku", 5))
				{
					return false;
				}
			}

			if (Cmd[0] == '#' && (!gUseNewNamedTest || (gUseNewNamedTest && !pSpawn->Lastname[0])))
				return true;

			if (isupper(Cmd[0]) && (!gUseNewNamedTest || (gUseNewNamedTest && !pSpawn->Lastname[0])))
				return true;
		}
	}

	return false;
}

char* FormatSearchSpawn(char* Buffer, size_t BufferSize, MQSpawnSearch* pSearchSpawn)
{
	if (!Buffer)
		return nullptr;

	char szTemp[MAX_STRING] = { 0 };

	if (!pSearchSpawn)
	{
		strcpy_s(Buffer, BufferSize, "None");
		return Buffer;
	}

	const char* pszSpawnType = nullptr;
	switch (pSearchSpawn->SpawnType)
	{
	case NONE:
	default:
		pszSpawnType = "any";
		break;
	case PC:
		pszSpawnType = "pc";
		break;
	case MOUNT:
		pszSpawnType = "mount";
		break;
	case PET:
		pszSpawnType = "pet";
		break;
	case PCPET:
		pszSpawnType = "pcpet";
		break;
	case NPCPET:
		pszSpawnType = "npcpet";
		break;
	case XTARHATER:
		pszSpawnType = "xtarhater";
		break;
	case NPC:
		pszSpawnType = "npc";
		break;
	case CORPSE:
		pszSpawnType = "corpse";
		break;
	case TRIGGER:
		pszSpawnType = "trigger";
		break;
	case TRAP:
		pszSpawnType = "trap";
		break;
	case CHEST:
		pszSpawnType = "chest";
		break;
	case TIMER:
		pszSpawnType = "timer";
		break;
	case UNTARGETABLE:
		pszSpawnType = "untargetable";
		break;
	case MERCENARY:
		pszSpawnType = "mercenary";
		break;
	case FLYER:
		pszSpawnType = "flyer";
		break;
	}

	sprintf_s(Buffer, BufferSize, "(%d-%d) %s", pSearchSpawn->MinLevel, pSearchSpawn->MaxLevel, pszSpawnType);

	if (pSearchSpawn->szName[0] != 0)
	{
		if (pSearchSpawn->bExactName)
		{
			sprintf_s(szTemp, " whose name exactly matches %s", pSearchSpawn->szName);
		}
		else
		{
			sprintf_s(szTemp, " whose name contains %s", pSearchSpawn->szName);
		}
		strcat_s(Buffer, BufferSize, szTemp);
	}

	if (pSearchSpawn->szRace[0] != 0)
	{
		sprintf_s(szTemp, " Race:%s", pSearchSpawn->szRace);
		strcat_s(Buffer, BufferSize, szTemp);
	}

	if (pSearchSpawn->szClass[0] != 0)
	{
		sprintf_s(szTemp, " Class:%s", pSearchSpawn->szClass);
		strcat_s(Buffer, BufferSize, szTemp);
	}

	if (pSearchSpawn->szBodyType[0] != 0)
	{
		sprintf_s(szTemp, " Body:%s", pSearchSpawn->szBodyType);
		strcat_s(Buffer, BufferSize, szTemp);
	}

	if (pSearchSpawn->GuildID != -1 && pSearchSpawn->GuildID != 0)
	{
		const char* szGuild = GetGuildByID(pSearchSpawn->GuildID);
		sprintf_s(szTemp, " Guild:%s", szGuild ? szGuild : "Unknown");
		strcat_s(Buffer, BufferSize, szTemp);
	}

	if (pSearchSpawn->bKnownLocation)
	{
		sprintf_s(szTemp, " at %1.2f,%1.2f", pSearchSpawn->yLoc, pSearchSpawn->xLoc);
		strcat_s(Buffer, BufferSize, szTemp);
	}

	if (pSearchSpawn->FRadius < 10000.0f)
	{
		sprintf_s(szTemp, " Radius:%1.2f", pSearchSpawn->FRadius);
		strcat_s(Buffer, BufferSize, szTemp);
	}

	if (pSearchSpawn->ZRadius < 10000.0f)
	{
		sprintf_s(szTemp, " Z:%1.2f", pSearchSpawn->ZRadius);
		strcat_s(Buffer, BufferSize, szTemp);
	}

	if (pSearchSpawn->Radius > 0.0f)
	{
		sprintf_s(szTemp, " NoPC:%1.2f", pSearchSpawn->Radius);
		strcat_s(Buffer, BufferSize, szTemp);
	}

	if (pSearchSpawn->SpawnID)
	{
		sprintf_s(szTemp, " ID:%d", pSearchSpawn->SpawnID);
		strcat_s(Buffer, BufferSize, szTemp);
	}

	if (pSearchSpawn->NotID)
	{
		sprintf_s(szTemp, " NotID:%d", pSearchSpawn->NotID);
		strcat_s(Buffer, BufferSize, szTemp);
	}

	if (pSearchSpawn->bAlert)
	{
		sprintf_s(szTemp, " Alert:%d", pSearchSpawn->AlertList);
		strcat_s(Buffer, BufferSize, szTemp);
	}

	if (pSearchSpawn->bNoAlert)
	{
		sprintf_s(szTemp, " NoAlert:%d", pSearchSpawn->NoAlertList);
		strcat_s(Buffer, BufferSize, szTemp);
	}

	if (pSearchSpawn->bNearAlert)
	{
		sprintf_s(szTemp, " NearAlert:%d", pSearchSpawn->NearAlertList);
		strcat_s(Buffer, BufferSize, szTemp);
	}

	if (pSearchSpawn->bNotNearAlert)
	{
		sprintf_s(szTemp, " NotNearAlert:%d", pSearchSpawn->NotNearAlertList);
		strcat_s(Buffer, BufferSize, szTemp);
	}

	if (pSearchSpawn->bGM && pSearchSpawn->SpawnType != NPC)
		strcat_s(Buffer, BufferSize, " GM");
	if (pSearchSpawn->bTrader)
		strcat_s(Buffer, BufferSize, " Trader");
	if (pSearchSpawn->bXTarHater)
		strcat_s(Buffer, BufferSize, " XTarHater");
	if (pSearchSpawn->bLFG)
		strcat_s(Buffer, BufferSize, " LFG");

	if (pSearchSpawn->bLight)
	{
		strcat_s(Buffer, BufferSize, " Light");
		if (pSearchSpawn->szLight[0])
		{
			strcat_s(Buffer, BufferSize, ":");
			strcat_s(Buffer, BufferSize, pSearchSpawn->szLight);
		}
	}

	if (pSearchSpawn->bLoS)
		strcat_s(Buffer, BufferSize, " LoS");

	return Buffer;
}

SPAWNINFO* NthNearestSpawn(MQSpawnSearch* pSearchSpawn, int Nth, SPAWNINFO* pOrigin, bool IncludeOrigin)
{
	if (!pSearchSpawn || !Nth || !pOrigin)
		return nullptr;

	std::vector<std::unique_ptr<MQRank>> spawnSet;
	SPAWNINFO* pSpawn = (SPAWNINFO*)pSpawnList;

	while (pSpawn)
	{
		if (IncludeOrigin || pSpawn != pOrigin)
		{
			if (SpawnMatchesSearch(pSearchSpawn, pOrigin, pSpawn))
			{
				// matches search, add to our set
				auto pNewRank = std::make_unique<MQRank>();
				pNewRank->VarPtr.Ptr = pSpawn;
				pNewRank->Value.Float = GetDistance3D(pOrigin->X, pOrigin->Y, pOrigin->Z, pSpawn->X, pSpawn->Y, pSpawn->Z);

				spawnSet.push_back(std::move(pNewRank));
			}
		}

		pSpawn = pSpawn->pNext;
	}

	if (Nth > static_cast<int>(spawnSet.size()))
	{
		return nullptr;
	}

	// sort our list
	std::sort(std::begin(spawnSet), std::end(spawnSet),
		[](const auto& a, const auto& b) { return a->Value.Float < b->Value.Float; });

	// get our Nth nearest
	return (SPAWNINFO*)spawnSet[Nth - 1]->VarPtr.Ptr;
}

int CountMatchingSpawns(MQSpawnSearch* pSearchSpawn, SPAWNINFO* pOrigin, bool IncludeOrigin)
{
	if (!pSearchSpawn || !pOrigin)
		return 0;

	int TotalMatching = 0;
	SPAWNINFO* pSpawn = (SPAWNINFO*)pSpawnList;

	if (IncludeOrigin)
	{
		while (pSpawn)
		{
			if (SpawnMatchesSearch(pSearchSpawn, pOrigin, pSpawn))
			{
				TotalMatching++;
			}
			pSpawn = pSpawn->pNext;
		}
	}
	else
	{
		while (pSpawn)
		{
			if (pSpawn != pOrigin && SpawnMatchesSearch(pSearchSpawn, pOrigin, pSpawn))
			{
				// matches search, add to our set
				TotalMatching++;
			}
			pSpawn = pSpawn->pNext;
		}
	}
	return TotalMatching;
}

SPAWNINFO* SearchThroughSpawns(MQSpawnSearch* pSearchSpawn, SPAWNINFO* pChar)
{
	SPAWNINFO* pFromSpawn = nullptr;

	if (pSearchSpawn->FromSpawnID > 0 && (pSearchSpawn->bTargNext || pSearchSpawn->bTargPrev))
	{
		pFromSpawn = (SPAWNINFO*)GetSpawnByID(pSearchSpawn->FromSpawnID);
		if (!pFromSpawn) return nullptr;
		for (int index = 0; index < 3000; index++)
		{
			if (EQP_DistArray[index].VarPtr.Ptr == pFromSpawn)
			{
				if (pSearchSpawn->bTargPrev)
				{
					index--;
					for (; index >= 0; index--)
					{
						if (EQP_DistArray[index].VarPtr.Ptr
							&& SpawnMatchesSearch(pSearchSpawn, pFromSpawn, (SPAWNINFO*)EQP_DistArray[index].VarPtr.Ptr))
						{
							return (SPAWNINFO*)EQP_DistArray[index].VarPtr.Ptr;
						}
					}
				}
				else
				{
					index++;
					for (; index < 3000; index++)
					{
						if (EQP_DistArray[index].VarPtr.Ptr
							&& SpawnMatchesSearch(pSearchSpawn, pFromSpawn, (SPAWNINFO*)EQP_DistArray[index].VarPtr.Ptr))
						{
							return (SPAWNINFO*)EQP_DistArray[index].VarPtr.Ptr;
						}
					}
				}
				return nullptr;
			}
		}
	}

	return NthNearestSpawn(pSearchSpawn, 1, pChar, true);
}

bool SearchSpawnMatchesSearchSpawn(MQSpawnSearch* pSearchSpawn1, MQSpawnSearch* pSearchSpawn2)
{
	if (pSearchSpawn1->AlertList != pSearchSpawn2->AlertList)
		return false;
	if (pSearchSpawn1->SpawnType != pSearchSpawn2->SpawnType)
		return false;
	if (pSearchSpawn1->FRadius != pSearchSpawn2->FRadius)
		return false;
	if (pSearchSpawn1->FromSpawnID != pSearchSpawn2->FromSpawnID)
		return false;
	if (pSearchSpawn1->GuildID != pSearchSpawn2->GuildID)
		return false;
	if (pSearchSpawn1->MaxLevel != pSearchSpawn2->MaxLevel)
		return false;
	if (pSearchSpawn1->MinLevel != pSearchSpawn2->MinLevel)
		return false;
	if (pSearchSpawn1->NearAlertList != pSearchSpawn2->NearAlertList)
		return false;
	if (pSearchSpawn1->NoAlertList != pSearchSpawn2->NoAlertList)
		return false;
	if (pSearchSpawn1->NotID != pSearchSpawn2->NotID)
		return false;
	if (pSearchSpawn1->NotNearAlertList != pSearchSpawn2->NotNearAlertList)
		return false;
	if (pSearchSpawn1->Radius != pSearchSpawn2->Radius)
		return false;
	if (pSearchSpawn1->SortBy != pSearchSpawn2->SortBy)
		return false;
	if (pSearchSpawn1->SpawnID != pSearchSpawn2->SpawnID)
		return false;
	if (_stricmp(pSearchSpawn1->szBodyType, pSearchSpawn2->szBodyType))
		return false;
	if (_stricmp(pSearchSpawn1->szClass, pSearchSpawn2->szClass))
		return false;
	if (_stricmp(pSearchSpawn1->szLight, pSearchSpawn2->szLight))
		return false;
	if (_stricmp(pSearchSpawn1->szName, pSearchSpawn2->szName))
		return false;
	if (_stricmp(pSearchSpawn1->szRace, pSearchSpawn2->szRace))
		return false;
	if (pSearchSpawn1->xLoc != pSearchSpawn2->xLoc)
		return false;
	if (pSearchSpawn1->yLoc != pSearchSpawn2->yLoc)
		return false;
	if (pSearchSpawn1->ZRadius != pSearchSpawn2->ZRadius)
		return false;
	if (pSearchSpawn1->bAlert != pSearchSpawn2->bAlert)
		return false;
	if (pSearchSpawn1->bAura != pSearchSpawn2->bAura)
		return false;
	if (pSearchSpawn1->bBanner != pSearchSpawn2->bBanner)
		return false;
	if (pSearchSpawn1->bCampfire != pSearchSpawn2->bCampfire)
		return false;
	if (pSearchSpawn1->bDps != pSearchSpawn2->bDps)
		return false;
	if (pSearchSpawn1->bExactName != pSearchSpawn2->bExactName)
		return false;
	if (pSearchSpawn1->bGM != pSearchSpawn2->bGM)
		return false;
	if (pSearchSpawn1->bGroup != pSearchSpawn2->bGroup)
		return false;
	if (pSearchSpawn1->bFellowship != pSearchSpawn2->bFellowship)
		return false;
	if (pSearchSpawn1->bKnight != pSearchSpawn2->bKnight)
		return false;
	if (pSearchSpawn1->bKnownLocation != pSearchSpawn2->bKnownLocation)
		return false;
	if (pSearchSpawn1->bLFG != pSearchSpawn2->bLFG)
		return false;
	if (pSearchSpawn1->bLight != pSearchSpawn2->bLight)
		return false;
	if (pSearchSpawn1->bLoS != pSearchSpawn2->bLoS)
		return false;
	if (pSearchSpawn1->bMerchant != pSearchSpawn2->bMerchant)
		return false;
	if (pSearchSpawn1->bBanker != pSearchSpawn2->bBanker)
		return false;
	if (pSearchSpawn1->bNamed != pSearchSpawn2->bNamed)
		return false;
	if (pSearchSpawn1->bNearAlert != pSearchSpawn2->bNearAlert)
		return false;
	if (pSearchSpawn1->bNoAlert != pSearchSpawn2->bNoAlert)
		return false;
	if (pSearchSpawn1->bNoGroup != pSearchSpawn2->bNoGroup)
		return false;
	if (pSearchSpawn1->bNoGuild != pSearchSpawn2->bNoGuild)
		return false;
	if (pSearchSpawn1->bNoPet != pSearchSpawn2->bNoPet)
		return false;
	if (pSearchSpawn1->bNotNearAlert != pSearchSpawn2->bNotNearAlert)
		return false;
	if (pSearchSpawn1->bRaid != pSearchSpawn2->bRaid)
		return false;
	if (pSearchSpawn1->bSlower != pSearchSpawn2->bSlower)
		return false;
	if (pSearchSpawn1->bSpawnID != pSearchSpawn2->bSpawnID)
		return false;
	if (pSearchSpawn1->bTank != pSearchSpawn2->bTank)
		return false;
	if (pSearchSpawn1->bTargetable != pSearchSpawn2->bTargetable)
		return false;
	if (pSearchSpawn1->bTargNext != pSearchSpawn2->bTargNext)
		return false;
	if (pSearchSpawn1->bTargPrev != pSearchSpawn2->bTargPrev)
		return false;
	if (pSearchSpawn1->bTrader != pSearchSpawn2->bTrader)
		return false;
	if (pSearchSpawn1->bTributeMaster != pSearchSpawn2->bTributeMaster)
		return false;
	if (pSearchSpawn1->bXTarHater != pSearchSpawn2->bXTarHater)
		return false;

	return true;
}

bool SpawnMatchesSearch(MQSpawnSearch* pSearchSpawn, SPAWNINFO* pChar, SPAWNINFO* pSpawn)
{
	eSpawnType SpawnType = GetSpawnType(pSpawn);

	if (SpawnType == PET && (pSearchSpawn->SpawnType == PCPET || pSearchSpawn->SpawnType == NPCPET))
	{
		if (SPAWNINFO* pTheMaster = (SPAWNINFO*)GetSpawnByID(pSpawn->MasterID))
		{
			if (pTheMaster->Type == SPAWN_NPC)
			{
				SpawnType = NPCPET;
			}
			else if (pTheMaster->Type == SPAWN_PLAYER)
			{
				SpawnType = PCPET;
			}
		}
	}

	if (pSearchSpawn->SpawnType != SpawnType && pSearchSpawn->SpawnType != NONE)
	{
		if (pSearchSpawn->SpawnType == NPCCORPSE)
		{
			if (SpawnType != CORPSE || pSpawn->Deity)
			{
				return false;
			}
		}
		else if (pSearchSpawn->SpawnType == PCCORPSE)
		{
			if (SpawnType != CORPSE || !pSpawn->Deity)
			{
				return false;
			}
		}
		else if (pSearchSpawn->SpawnType == NPC && SpawnType == UNTARGETABLE)
		{
			return false;
		}

		// if the search type is not npc or the mob type is UNT, continue?
		// stupid /who

		if (pSearchSpawn->SpawnType != NPC || SpawnType != UNTARGETABLE)
			return false;
	}

	if (pSearchSpawn->MinLevel && pSpawn->Level < pSearchSpawn->MinLevel)
		return false;
	if (pSearchSpawn->MaxLevel && pSpawn->Level > pSearchSpawn->MaxLevel)
		return false;
	if (pSearchSpawn->NotID == pSpawn->SpawnID)
		return false;
	if (pSearchSpawn->bSpawnID && pSearchSpawn->SpawnID != pSpawn->SpawnID)
		return false;
	if (pSearchSpawn->GuildID != -1 && pSearchSpawn->GuildID != pSpawn->GuildID)
		return false;

	if (pSearchSpawn->bGM && pSearchSpawn->SpawnType != NPC)
	{
		if (!pSpawn->GM)
			return false;
	}

	if (pSearchSpawn->bGM && pSearchSpawn->SpawnType == NPC)
	{
		if (pSpawn->mActorClient.Class < 20 || pSpawn->mActorClient.Class > 35)
			return false;
	}

	if (pSearchSpawn->bNamed && !IsNamed(pSpawn))
		return false;
	if (pSearchSpawn->bMerchant && pSpawn->mActorClient.Class != 41)
		return false;
	if (pSearchSpawn->bBanker && pSpawn->mActorClient.Class != 40)
		return false;
	if (pSearchSpawn->bTributeMaster && pSpawn->mActorClient.Class != 63)
		return false;
	if (pSearchSpawn->bNoGuild && (pSpawn->GuildID != -1 && pSpawn->GuildID != 0))
		return false;

	if (pSearchSpawn->bKnight && pSearchSpawn->SpawnType != NPC)
	{
		if (pSpawn->mActorClient.Class != Paladin
			&& pSpawn->mActorClient.Class != Shadowknight)
		{
			return false;
		}
	}

	if (pSearchSpawn->bTank && pSearchSpawn->SpawnType != NPC)
	{
		if (pSpawn->mActorClient.Class != Paladin
			&& pSpawn->mActorClient.Class != Shadowknight
			&& pSpawn->mActorClient.Class != Warrior)
		{
			return false;
		}
	}

	if (pSearchSpawn->bHealer && pSearchSpawn->SpawnType != NPC)
	{
		if (pSpawn->mActorClient.Class != Cleric
			&& pSpawn->mActorClient.Class != Druid)
		{
			return false;
		}
	}

	if (pSearchSpawn->bDps && pSearchSpawn->SpawnType != NPC)
	{
		if (pSpawn->mActorClient.Class != Ranger
			&& pSpawn->mActorClient.Class != Rogue
			&& pSpawn->mActorClient.Class != Wizard
			&& pSpawn->mActorClient.Class != Berserker)
		{
			return false;
		}
	}

	if (pSearchSpawn->bSlower && pSearchSpawn->SpawnType != NPC)
	{
		if (pSpawn->mActorClient.Class != Shaman
			&& pSpawn->mActorClient.Class != Enchanter
			&& pSpawn->mActorClient.Class != Beastlord)
		{
			return false;
		}
	}

	if (pSearchSpawn->bLFG && !pSpawn->LFG)
		return false;
	if (pSearchSpawn->bTrader && !pSpawn->Trader)
		return false;

	if (pSearchSpawn->bXTarHater)
	{
		bool foundhater = false;

		if (CHARINFO* pmyChar = GetCharInfo())
		{
			if (ExtendedTargetList* xtm = pmyChar->pXTargetMgr)
			{
				if (xtm->XTargetSlots.Count)
				{
					for (int i = 0; i < pmyChar->pXTargetMgr->XTargetSlots.Count; i++)
					{
						XTARGETSLOT xts = xtm->XTargetSlots[i];

						if (xts.xTargetType == XTARGET_AUTO_HATER && xts.XTargetSlotStatus && xts.SpawnID)
						{
							if (SPAWNINFO* pxtarSpawn = (SPAWNINFO*)GetSpawnByID(xts.SpawnID))
							{
								if (pxtarSpawn->SpawnID == pSpawn->SpawnID)
								{
									foundhater = true;
								}
							}
						}
					}
				}
			}
		}

		if (!foundhater)
		{
			return false;
		}
	}

	if (pSearchSpawn->bGroup)
	{
		bool ingrp = false;
		if (pSearchSpawn->SpawnType == PCCORPSE || pSpawn->Type == SPAWN_CORPSE)
		{
			ingrp = IsInGroup(pSpawn, true);
		}
		else
		{
			ingrp = IsInGroup(pSpawn);
		}

		if (!ingrp)
			return false;
	}

	if (pSearchSpawn->bFellowship)
	{
		bool infellowship = false;
		if (pSearchSpawn->SpawnType == PCCORPSE || pSpawn->Type == SPAWN_CORPSE)
		{
			infellowship = IsInFellowship(pSpawn, true);
		}
		else
		{
			infellowship = IsInFellowship(pSpawn);
		}

		if (!infellowship)
			return false;
	}

	if (pSearchSpawn->bNoGroup && IsInGroup(pSpawn))
		return false;

	if (pSearchSpawn->bRaid)
	{
		bool ingrp = false;
		if (pSearchSpawn->SpawnType == PCCORPSE || pSpawn->Type == SPAWN_CORPSE)
		{
			ingrp = IsInRaid(pSpawn, true);
		}
		else
		{
			ingrp = IsInRaid(pSpawn);
		}
		if (!ingrp)
			return false;
	}

	if (pSearchSpawn->bKnownLocation)
	{
		if ((pSearchSpawn->xLoc != pSpawn->X || pSearchSpawn->yLoc != pSpawn->Y))
		{
			if (pSearchSpawn->FRadius < 10000.0f
				&& Distance3DToPoint(pSpawn, pSearchSpawn->xLoc, pSearchSpawn->yLoc, pSearchSpawn->zLoc)>pSearchSpawn->FRadius)
			{
				return false;
			}
		}
	}
	else if (pSearchSpawn->FRadius < 10000.0f && Distance3DToSpawn(pChar, pSpawn)>pSearchSpawn->FRadius)
	{
		return false;
	}

	if (pSearchSpawn->Radius > 0.0f && IsPCNear(pSpawn, pSearchSpawn->Radius))
		return false;
	if (gZFilter < 10000.0f && ((pSpawn->Z > pSearchSpawn->zLoc + gZFilter) || (pSpawn->Z < pSearchSpawn->zLoc - gZFilter)))
		return false;
	if (pSearchSpawn->ZRadius < 10000.0f && (pSpawn->Z > pSearchSpawn->zLoc + pSearchSpawn->ZRadius || pSpawn->Z < pSearchSpawn->zLoc - pSearchSpawn->ZRadius))
		return false;
	if (pSearchSpawn->bLight)
	{
		const char* pLight = GetLightForSpawn(pSpawn);
		if (!_stricmp(pLight, "NONE"))
			return false;
		if (pSearchSpawn->szLight[0] && _stricmp(pLight, pSearchSpawn->szLight))
			return false;
	}
	if ((pSearchSpawn->bAlert) && CAlerts.AlertExist(pSearchSpawn->AlertList))
	{
		if (!IsAlert(pChar, pSpawn, pSearchSpawn->AlertList))
			return false;
	}
	if ((pSearchSpawn->bNoAlert) && CAlerts.AlertExist(pSearchSpawn->NoAlertList))
	{
		if (IsAlert(pChar, pSpawn, pSearchSpawn->NoAlertList))
			return false;
	}
	if (pSearchSpawn->bNotNearAlert && GetClosestAlert(pSpawn, pSearchSpawn->NotNearAlertList))
		return false;
	if (pSearchSpawn->bNearAlert && !GetClosestAlert(pSpawn, pSearchSpawn->NearAlertList))
		return false;
	if (pSearchSpawn->szClass[0] && _stricmp(pSearchSpawn->szClass, GetClassDesc(pSpawn->mActorClient.Class)))
		return false;
	if (pSearchSpawn->szBodyType[0] && _stricmp(pSearchSpawn->szBodyType, GetBodyTypeDesc(GetBodyType(pSpawn))))
		return false;
	if (pSearchSpawn->szRace[0] && _stricmp(pSearchSpawn->szRace, pEverQuest->GetRaceDesc(pSpawn->mActorClient.Race)))
		return false;
	if (pSearchSpawn->bLoS && !pCharSpawn->CanSee(*(PlayerClient*)pSpawn))
		return false;
	if (pSearchSpawn->bTargetable && !IsTargetable(pSpawn))
		return false;
	if (pSearchSpawn->PlayerState && !(pSpawn->PlayerState & pSearchSpawn->PlayerState)) // if player state isn't 0 and we have that bit set
		return false;
	if (pSearchSpawn->szName[0] && pSpawn->Name[0])
	{
		char szName[MAX_STRING] = { 0 };
		char szSearchName[MAX_STRING] = { 0 };
		strcpy_s(szName, pSpawn->Name);
		_strlwr_s(szName);
		strcpy_s(szSearchName, pSearchSpawn->szName);
		_strlwr_s(szSearchName);

		if (!strstr(szName, szSearchName) && !strstr(CleanupName(szName, sizeof(szName), false), szSearchName))
			return false;
		if (pSearchSpawn->bExactName && _stricmp(CleanupName(szName, sizeof(szName), false, !gbExactSearchCleanNames), pSearchSpawn->szName))
			return false;
	}
	return true;
}

char* ParseSearchSpawnArgs(char* szArg, char* szRest, MQSpawnSearch* pSearchSpawn)
{
	if (szArg && pSearchSpawn)
	{
		if (!_stricmp(szArg, "pc"))
		{
			pSearchSpawn->SpawnType = PC;
		}
		else if (!_stricmp(szArg, "npc"))
		{
			pSearchSpawn->SpawnType = NPC;
		}
		else if (!_stricmp(szArg, "mount"))
		{
			pSearchSpawn->SpawnType = MOUNT;
		}
		else if (!_stricmp(szArg, "pet"))
		{
			pSearchSpawn->SpawnType = PET;
		}
		else if (!_stricmp(szArg, "pcpet"))
		{
			pSearchSpawn->SpawnType = PCPET;
		}
		else if (!_stricmp(szArg, "npcpet"))
		{
			pSearchSpawn->SpawnType = NPCPET;
		}
		else if (!_stricmp(szArg, "xtarhater"))
		{
			pSearchSpawn->bXTarHater = true;
		}
		else if (!_stricmp(szArg, "nopet"))
		{
			pSearchSpawn->bNoPet = true;
		}
		else if (!_stricmp(szArg, "corpse"))
		{
			pSearchSpawn->SpawnType = CORPSE;
		}
		else if (!_stricmp(szArg, "npccorpse"))
		{
			pSearchSpawn->SpawnType = NPCCORPSE;
		}
		else if (!_stricmp(szArg, "pccorpse"))
		{
			pSearchSpawn->SpawnType = PCCORPSE;
		}
		else if (!_stricmp(szArg, "trigger"))
		{
			pSearchSpawn->SpawnType = TRIGGER;
		}
		else if (!_stricmp(szArg, "untargetable"))
		{
			pSearchSpawn->SpawnType = UNTARGETABLE;
		}
		else if (!_stricmp(szArg, "trap"))
		{
			pSearchSpawn->SpawnType = TRAP;
		}
		else if (!_stricmp(szArg, "chest"))
		{
			pSearchSpawn->SpawnType = CHEST;
		}
		else if (!_stricmp(szArg, "timer"))
		{
			pSearchSpawn->SpawnType = TIMER;
		}
		else if (!_stricmp(szArg, "aura"))
		{
			pSearchSpawn->SpawnType = AURA;
		}
		else if (!_stricmp(szArg, "object"))
		{
			pSearchSpawn->SpawnType = OBJECT;
		}
		else if (!_stricmp(szArg, "banner"))
		{
			pSearchSpawn->SpawnType = BANNER;
		}
		else if (!_stricmp(szArg, "campfire"))
		{
			pSearchSpawn->SpawnType = CAMPFIRE;
		}
		else if (!_stricmp(szArg, "mercenary"))
		{
			pSearchSpawn->SpawnType = MERCENARY;
		}
		else if (!_stricmp(szArg, "flyer"))
		{
			pSearchSpawn->SpawnType = FLYER;
		}
		else if (!_stricmp(szArg, "any"))
		{
			pSearchSpawn->SpawnType = NONE;
		}
		else if (!_stricmp(szArg, "next"))
		{
			pSearchSpawn->bTargNext = true;
		}
		else if (!_stricmp(szArg, "prev"))
		{
			pSearchSpawn->bTargPrev = true;
		}
		else if (!_stricmp(szArg, "lfg"))
{
			pSearchSpawn->bLFG = true;
		}
		else if (!_stricmp(szArg, "gm"))
{
			pSearchSpawn->bGM = true;
		}
		else if (!_stricmp(szArg, "group"))
{
			pSearchSpawn->bGroup = true;
		}
		else if (!_stricmp(szArg, "fellowship"))
		{
			pSearchSpawn->bFellowship = true;
		}
		else if (!_stricmp(szArg, "nogroup"))
		{
			pSearchSpawn->bNoGroup = true;
		}
		else if (!_stricmp(szArg, "raid"))
		{
			pSearchSpawn->bRaid = true;
		}
		else if (!_stricmp(szArg, "noguild"))
		{
			pSearchSpawn->bNoGuild = true;
		}
		else if (!_stricmp(szArg, "trader"))
{
			pSearchSpawn->bTrader = true;
		}
		else if (!_stricmp(szArg, "named"))
{
			pSearchSpawn->bNamed = true;
		}
		else if (!_stricmp(szArg, "merchant"))
		{
			pSearchSpawn->bMerchant = true;
		}
		else if (!_stricmp(szArg, "banker"))
		{
			pSearchSpawn->bBanker = true;
		}
		else if (!_stricmp(szArg, "tribute"))
		{
			pSearchSpawn->bTributeMaster = true;
		}
		else if (!_stricmp(szArg, "knight"))
		{
			pSearchSpawn->bKnight = true;
		}
		else if (!_stricmp(szArg, "tank"))
		{
			pSearchSpawn->bTank = true;
		}
		else if (!_stricmp(szArg, "healer"))
		{
			pSearchSpawn->bHealer = true;
		}
		else if (!_stricmp(szArg, "dps"))
		{
			pSearchSpawn->bDps = true;
		}
		else if (!_stricmp(szArg, "slower"))
		{
			pSearchSpawn->bSlower = true;
		}
		else if (!_stricmp(szArg, "los"))
		{
			pSearchSpawn->bLoS = true;
		}
		else if (!_stricmp(szArg, "targetable"))
		{
			pSearchSpawn->bTargetable = true;
		}
		else if (!_stricmp(szArg, "range"))
		{
			GetArg(szArg, szRest, 1);
			pSearchSpawn->MinLevel = atoi(szArg);
			GetArg(szArg, szRest, 2);
			pSearchSpawn->MaxLevel = atoi(szArg);
			szRest = GetNextArg(szRest, 2);
		}
		else if (!_stricmp(szArg, "loc"))
		{
			pSearchSpawn->bKnownLocation = true;
			GetArg(szArg, szRest, 1);
			pSearchSpawn->xLoc = (float)atof(szArg);
			GetArg(szArg, szRest, 2);
			pSearchSpawn->yLoc = (float)atof(szArg);
			GetArg(szArg, szRest, 3);
			pSearchSpawn->zLoc = (float)atof(szArg);
			if (pSearchSpawn->zLoc == 0.0)
			{
				pSearchSpawn->zLoc = ((SPAWNINFO*)pCharSpawn)->Z;
				szRest = GetNextArg(szRest, 2);
			}
			else
			{
				szRest = GetNextArg(szRest, 3);
			}
		}
		else if (!_stricmp(szArg, "id"))
		{
			GetArg(szArg, szRest, 1);
			pSearchSpawn->bSpawnID = true;
			pSearchSpawn->SpawnID = atoi(szArg);
			szRest = GetNextArg(szRest, 1);
		}
		else if (!_stricmp(szArg, "radius"))
		{
			GetArg(szArg, szRest, 1);
			pSearchSpawn->FRadius = atof(szArg);
			szRest = GetNextArg(szRest, 1);
		}
		else if (!_stricmp(szArg, "body"))
		{
			GetArg(szArg, szRest, 1);
			strcpy_s(pSearchSpawn->szBodyType, szArg);
			szRest = GetNextArg(szRest, 1);
		}
		else if (!_stricmp(szArg, "class"))
		{
			GetArg(szArg, szRest, 1);
			strcpy_s(pSearchSpawn->szClass, szArg);
			szRest = GetNextArg(szRest, 1);
		}
		else if (!_stricmp(szArg, "race"))
		{
			GetArg(szArg, szRest, 1);
			strcpy_s(pSearchSpawn->szRace, szArg);
			szRest = GetNextArg(szRest, 1);
		}
		else if (!_stricmp(szArg, "light"))
		{
			GetArg(szArg, szRest, 1);
			int Light = -1;
			if (szArg[0] != 0)
			{
				for (int i = 0; i < LIGHT_COUNT; i++)
				{
					if (!_stricmp(szLights[i], szArg))
						Light = i;
				}
			}

			if (Light != -1)
			{
				strcpy_s(pSearchSpawn->szLight, szLights[Light]);
				szRest = GetNextArg(szRest, 1);
			}
			else
			{
				pSearchSpawn->szLight[0] = 0;
			}
			pSearchSpawn->bLight = true;
		}
		else if (!_stricmp(szArg, "guild"))
		{
			pSearchSpawn->GuildID = GetCharInfo()->GuildID;
		}
		else if (!_stricmp(szArg, "guildname"))
		{
			int64_t GuildID = -1;
			GetArg(szArg, szRest, 1);
			if (szArg[0] != 0)
				GuildID = GetGuildIDByName(szArg);
			if (GuildID != -1 && GuildID != 0)
			{
				pSearchSpawn->GuildID = GuildID;
				szRest = GetNextArg(szRest, 1);
			}
		}
		else if (!_stricmp(szArg, "alert"))
		{
			GetArg(szArg, szRest, 1);
			pSearchSpawn->AlertList = atoi(szArg);
			szRest = GetNextArg(szRest, 1);
			pSearchSpawn->bAlert = true;
		}
		else if (!_stricmp(szArg, "noalert"))
		{
			GetArg(szArg, szRest, 1);
			pSearchSpawn->NoAlertList = atoi(szArg);
			szRest = GetNextArg(szRest, 1);
			pSearchSpawn->bNoAlert = true;
		}
		else if (!_stricmp(szArg, "notnearalert"))
		{
			GetArg(szArg, szRest, 1);
			pSearchSpawn->NotNearAlertList = atoi(szArg);
			szRest = GetNextArg(szRest, 1);
			pSearchSpawn->bNotNearAlert = true;
		}
		else if (!_stricmp(szArg, "nearalert"))
		{
			GetArg(szArg, szRest, 1);
			pSearchSpawn->NearAlertList = atoi(szArg);
			szRest = GetNextArg(szRest, 1);
			pSearchSpawn->bNearAlert = true;
		}
		else if (!_stricmp(szArg, "zradius"))
		{
			GetArg(szArg, szRest, 1);
			pSearchSpawn->ZRadius = atof(szArg);
			szRest = GetNextArg(szRest, 1);
		}
		else if (!_stricmp(szArg, "notid"))
		{
			GetArg(szArg, szRest, 1);
			pSearchSpawn->NotID = atoi(szArg);
			szRest = GetNextArg(szRest, 1);
		}
		else if (!_stricmp(szArg, "nopcnear"))
		{
			GetArg(szArg, szRest, 1);
			if ((szArg[0] == 0) || (0.0f == (pSearchSpawn->Radius = (float)atof(szArg))))
			{
				pSearchSpawn->Radius = 200.0f;
			}
			else
			{
				szRest = GetNextArg(szRest, 1);
			}
		}
		else if (!_stricmp(szArg, "playerstate"))
		{
			GetArg(szArg, szRest, 1);
			pSearchSpawn->PlayerState |= atoi(szArg); // This allows us to pass multiple playerstate args
			szRest = GetNextArg(szRest, 1);
		}
		else if (IsNumber(szArg))
		{
			pSearchSpawn->MinLevel = atoi(szArg);
			pSearchSpawn->MaxLevel = pSearchSpawn->MinLevel;
		}
		else
		{
			for (size_t index = 1; index < lengthof(ClassInfo) - 1; index++)
			{
				if (!_stricmp(szArg, ClassInfo[index].Name) || !_stricmp(szArg, ClassInfo[index].ShortName))
				{
					strcpy_s(pSearchSpawn->szClass, pEverQuest->GetClassDesc(index));
					return szRest;
				}
			}

			if (pSearchSpawn->szName[0])
			{
				// multiple word name
				strcat_s(pSearchSpawn->szName, " ");
				strcat_s(pSearchSpawn->szName, szArg);
			}
			else
			{
				if (szArg[0] == '=')
				{
					pSearchSpawn->bExactName = true;
					szArg++;
				}
				strcpy_s(pSearchSpawn->szName, szArg);
			}
		}
	}

	return szRest;
}

void ParseSearchSpawn(const char* Buffer, MQSpawnSearch* pSearchSpawn)
{
	bRunNextCommand = true;

	char szLLine[MAX_STRING] = { 0 };
	strcpy_s(szLLine, Buffer);
	_strlwr_s(szLLine);
	char* szFilter = szLLine;

	char szArg[MAX_STRING] = { 0 };

	while (true)
	{
		GetArg(szArg, szFilter, 1);

		szFilter = GetNextArg(szFilter, 1);
		if (szArg[0] == 0)
		{
			break;
		}

		szFilter = ParseSearchSpawnArgs(szArg, szFilter, pSearchSpawn);
	}
}

bool GetClosestAlert(SPAWNINFO* pChar, uint32_t id)
{
	if (!ppSpawnManager)
		return false;
	if (!pSpawnList)
		return false;

	SPAWNINFO* pSpawn = nullptr;
	SPAWNINFO* pClosest = nullptr;

	float ClosestDistance = 50000.0f;

	char szName[MAX_STRING] = { 0 };

	std::vector<MQSpawnSearch> search;
	if (CAlerts.GetAlert(id, search))
	{
		for (auto& s : search)
		{
			if (pSpawn = SearchThroughSpawns(&s, pChar))
			{
				if (Distance3DToSpawn(pChar, pSpawn) < ClosestDistance)
				{
					pClosest = pSpawn;
				}
			}
		}
	}

	return pClosest != nullptr;
}

bool IsAlert(SPAWNINFO* pChar, SPAWNINFO* pSpawn, uint32_t id)
{
	char szName[MAX_STRING] = { 0 };

	MQSpawnSearch SearchSpawn;

	std::vector<MQSpawnSearch> alerts;
	if (CAlerts.GetAlert(id, alerts))
	{
		for (auto& search : alerts)
		{
			if (search.SpawnID > 0 && search.SpawnID != pSpawn->SpawnID)
				continue;

			// FIXME
			memcpy(&SearchSpawn, &search, sizeof(MQSpawnSearch));
			SearchSpawn.SpawnID = pSpawn->SpawnID;

			// if this spawn matches, it's true. This is an implied logical or
			if (SpawnMatchesSearch(&SearchSpawn, pChar, pSpawn))
				return true;
		}
	}

	return false;
}

// FIXME: This function is broken, and doesn't actually check against the CAlerts list.
bool CheckAlertForRecursion(MQSpawnSearch* pSearchSpawn, uint32_t id)
{
	if (gbIgnoreAlertRecursion)
		return false;

	if (!pSearchSpawn)
		return false;

	std::vector<MQSpawnSearch> ss;
	if (CAlerts.GetAlert(id, ss))
	{
		for (auto i = ss.begin(); i != ss.end(); i++)
		{
			if (pSearchSpawn->bAlert)
			{
				if (pSearchSpawn->AlertList == id)
				{
					return true;
				}

				if (CheckAlertForRecursion(pSearchSpawn, pSearchSpawn->AlertList))
				{
					return true;
				}
			}

			if (pSearchSpawn->bNoAlert)
			{
				if (pSearchSpawn->NoAlertList == id)
				{
					return true;
				}

				if (CheckAlertForRecursion(pSearchSpawn, pSearchSpawn->NoAlertList))
				{
					return true;
				}
			}

			if (pSearchSpawn->bNearAlert)
			{
				if (pSearchSpawn->NearAlertList == id)
				{
					return true;
				}

				if (CheckAlertForRecursion(pSearchSpawn, pSearchSpawn->NearAlertList))
				{
					return true;
				}
			}

			if (pSearchSpawn->bNotNearAlert)
			{
				if (pSearchSpawn->NotNearAlertList == id)
				{
					return true;
				}

				if (CheckAlertForRecursion(pSearchSpawn, pSearchSpawn->NotNearAlertList))
				{
					return true;
				}
			}
		}
	}

	return false;
}
// ***************************************************************************
// Function:    CleanupName
// Description: Cleans up NPC names
//              an_iksar_marauder23 = iksar marauder, an
// ***************************************************************************
char* CleanupName(char* szName, size_t BufferSize, bool Article, bool ForWhoList)
{
	char szTemp[MAX_STRING] = { 0 };
	int j = 0;

	for (size_t i = 0; i < strlen(szName); i++)
	{
		switch (szName[i])
		{
		case '_':
			szTemp[j++] = ' ';
			break;
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			break;
		case '#':
			if (!ForWhoList)
				break;
		default:
			szTemp[j++] = szName[i];
		}
	}

	strcpy_s(szName, BufferSize, szTemp);

	if (!Article) return szName;

	if (!_strnicmp(szName, "a ", 2))
	{
		sprintf_s(szTemp, "%s, a", szName + 2);
		strcpy_s(szName, BufferSize, szTemp);
	}
	else if (!_strnicmp(szName, "an ", 3))
	{
		sprintf_s(szTemp, "%s, an", szName + 3);
		strcpy_s(szName, BufferSize, szTemp);
	}
	else if (!_strnicmp(szName, "the ", 4))
	{
		sprintf_s(szTemp, "%s, the", szName + 4);
		strcpy_s(szName, BufferSize, szTemp);
	}

	return szName;
}

// ***************************************************************************
// Function:    SuperWhoDisplay
// Description: Displays our SuperWho / SuperWhoTarget
// ***************************************************************************
void SuperWhoDisplay(SPAWNINFO* pSpawn, DWORD Color)
{
	char szName[MAX_STRING] = { 0 };
	char szMsg[MAX_STRING] = { 0 };
	char szMsgL[MAX_STRING] = { 0 };
	char szTemp[MAX_STRING] = { 0 };

	strcpy_s(szName, pSpawn->DisplayedName);

	if (pSpawn->Type == SPAWN_PLAYER)
	{
		if (gFilterSWho.Lastname && strlen(pSpawn->Lastname) > 0)
		{
			strcat_s(szName, " ");
			strcat_s(szName, pSpawn->Lastname);
		}

		if (gFilterSWho.Guild && pSpawn->GuildID != -1 && pSpawn->GuildID != 0)
		{
			strcat_s(szName, " <");
			const char* szGuild = GetGuildByID(pSpawn->GuildID);
			strcat_s(szName, szGuild ? szGuild : "Unknown Guild");
			strcat_s(szName, ">");
		}
	}
	else
	{
		if (gFilterSWho.Lastname && strlen(pSpawn->Lastname) > 0)
		{
			strcat_s(szName, " (");
			strcat_s(szName, pSpawn->Lastname);
			strcat_s(szName, ")");
		}
	}

	char GM[MAX_STRING] = { 0 };

	if (gFilterSWho.GM && pSpawn->GM)
	{
		if (pSpawn->Level >= 50)
		{
			strcpy_s(GM, "\ay*GM*\ax");
		}
		else if (pSpawn->Level == 20)
		{
			strcpy_s(GM, "\a-y*Guide Applicant*\ax");
		}
		else
		{
			strcpy_s(GM, "\a-y*Guide*\ax");
		}
	}

	szMsg[0] = '\a';
	szMsg[2] = 0;

	if (Color || gFilterSWho.ConColor)
	{
		switch (ConColor(pSpawn))
		{
		case CONCOLOR_WHITE:
			szMsg[1] = 'w';
			break;
		case CONCOLOR_YELLOW:
			szMsg[1] = 'y';
			break;
		case CONCOLOR_RED:
			szMsg[1] = 'r';
			break;
		case CONCOLOR_BLUE:
			szMsg[1] = 'u';
			break;
		case CONCOLOR_LIGHTBLUE:
			szMsg[1] = 't';
			break;
		case CONCOLOR_GREEN:
			szMsg[1] = 'g';
			break;
		case CONCOLOR_GREY:
			szMsg[1] = '-';
			szMsg[2] = 'w';
			break;
		default:
			szMsg[1] = 'm';
			break;
		}
	}
	else
	{
		szMsg[1] = 'w';
	}

	if (gFilterSWho.GM)
		strcat_s(szMsg, GM);

	if (gFilterSWho.Level || gFilterSWho.Race || gFilterSWho.Body || gFilterSWho.Class)
	{
		strcat_s(szMsg, "\a-u[\ax");

		if (gFilterSWho.Level)
		{
			_itoa_s(pSpawn->Level, szTemp, 10);
			strcat_s(szMsg, szTemp);
			strcat_s(szMsg, " ");
		}

		if (gFilterSWho.Race)
		{
			strcat_s(szMsg, pEverQuest->GetRaceDesc(pSpawn->mActorClient.Race));
			strcat_s(szMsg, " ");
		}

		if (gFilterSWho.Body)
		{
			strcat_s(szMsg, GetBodyTypeDesc(GetBodyType(pSpawn)));
			strcat_s(szMsg, " ");
		}

		if (gFilterSWho.Class)
		{
			strcat_s(szMsg, GetClassDesc(pSpawn->mActorClient.Class));
			strcat_s(szMsg, " ");
		}

		szMsg[strlen(szMsg) - 1] = 0;
		strcat_s(szMsg, "\a-u]\ax");
	}

	strcat_s(szMsg, " ");
	strcat_s(szMsg, szName);

	if (pSpawn->Type == SPAWN_PLAYER)
	{
		if (gFilterSWho.Anon && pSpawn->Anon > 0)
		{
			if (pSpawn->Anon == 2)
			{
				strcat_s(szMsg, " \ag*RP*\ax");
			}
			else {
				strcat_s(szMsg, " \ag*Anon*\ax");
			}
		}

		if (gFilterSWho.LD && pSpawn->Linkdead) strcat_s(szMsg, " \ag<LD>\ax");
		if (gFilterSWho.Sneak && pSpawn->Sneak) strcat_s(szMsg, " \ag<Sneak>\ax");
		if (gFilterSWho.AFK && pSpawn->AFK) strcat_s(szMsg, " \ag<AFK>\ax");
		if (gFilterSWho.LFG && pSpawn->LFG) strcat_s(szMsg, " \ag<LFG>\ax");
		if (gFilterSWho.Trader && pSpawn->Trader) strcat_s(szMsg, " \ag<Trader>\ax");
	}
	else if (gFilterSWho.NPCTag && pSpawn->Type == SPAWN_NPC)
	{
		if (pSpawn->MasterID != 0)
		{
			strcat_s(szMsg, " <PET>");
		}
		else
		{
			strcat_s(szMsg, " <NPC>");
		}
	}

	if (gFilterSWho.Light)
	{
		const char* szLight = GetLightForSpawn(pSpawn);
		if (_stricmp(szLight, "NONE"))
		{
			strcat_s(szMsg, " (");
			strcat_s(szMsg, szLight);
			strcat_s(szMsg, ")");
		}
	}

	strcpy_s(szMsgL, szMsg);

	if (gFilterSWho.Distance)
	{
		int Angle = static_cast<int>((atan2f(GetCharInfo()->pSpawn->X - pSpawn->X, GetCharInfo()->pSpawn->Y - pSpawn->Y) * 180.0f / PI + 360.0f) / 22.5f + 0.5f) % 16;
		sprintf_s(szTemp, " \a-u(\ax%1.2f %s\a-u,\ax %1.2fZ\a-u)\ax", GetDistance(GetCharInfo()->pSpawn, pSpawn), szHeadingShort[Angle], pSpawn->Z - GetCharInfo()->pSpawn->Z);
		strcat_s(szMsg, szTemp);
	}

	if (gFilterSWho.SpawnID)
	{
		strcat_s(szMsg, " \a-u(\axID:");
		_itoa_s(pSpawn->SpawnID, szTemp, 10);
		strcat_s(szMsg, szTemp);
		strcat_s(szMsg, "\a-u)\ax");
	}

	if (gFilterSWho.Holding && (pSpawn->Equipment.Primary.ID || pSpawn->Equipment.Offhand.ID))
	{
		strcat_s(szMsg, " \a-u(\ax");

		if (pSpawn->Equipment.Primary.ID)
		{
			_itoa_s(pSpawn->Equipment.Primary.ID, szTemp, 10);
			strcat_s(szMsg, "Pri: ");
			strcat_s(szMsg, szTemp);
			if (pSpawn->Equipment.Offhand.ID)
				strcat_s(szMsg, " ");
		}

		if (pSpawn->Equipment.Offhand.ID)
		{
			_itoa_s(pSpawn->Equipment.Offhand.ID, szTemp, 10);
			strcat_s(szMsg, "Off: ");
			strcat_s(szMsg, szTemp);
		}

		strcat_s(szMsg, "\a-u)\ax");
	}

	switch (GetSpawnType(pSpawn))
	{
	case CHEST:
		strcat_s(szMsg, " \ar*CHEST*\ax");
		break;
	case TRAP:
		strcat_s(szMsg, " \ar*TRAP*\ax");
		break;
	case TRIGGER:
		strcat_s(szMsg, " \ar*TRIGGER*\ax");
		break;
	case TIMER:
		strcat_s(szMsg, " \ar*TIMER*\ax");
		break;
	case UNTARGETABLE:
		strcat_s(szMsg, " \ar*UNTARGETABLE*\ax");
		break;
	}

	WriteChatColor(szMsg, USERCOLOR_WHO);
}

struct SuperWhoSortPredicate
{
	SuperWhoSortPredicate(SearchSortBy sortBy, SPAWNINFO* pSeachOrigin)
		: m_sortBy(sortBy)
		, m_pOrigin(pSeachOrigin)
	{
	}

	bool operator()(SPAWNINFO* SpawnA, SPAWNINFO* SpawnB)
	{
		switch (m_sortBy)
		{
		case SearchSortBy::Level:
			return SpawnA->Level < SpawnB->Level;

		case SearchSortBy::Name:
			return _stricmp(SpawnA->DisplayedName, SpawnB->DisplayedName) < 0;

		case SearchSortBy::Race:
			return _stricmp(pEverQuest->GetRaceDesc(SpawnA->mActorClient.Race), pEverQuest->GetRaceDesc(SpawnB->mActorClient.Race)) < 0;

		case SearchSortBy::Class:
			return _stricmp(GetClassDesc(SpawnA->mActorClient.Class), GetClassDesc(SpawnB->mActorClient.Class)) < 0;

		case SearchSortBy::Distance:
			return GetDistanceSquared(m_pOrigin, SpawnA) < GetDistanceSquared(m_pOrigin, SpawnB);

		case SearchSortBy::Guild:
		{
			char szGuild1[256] = { "" };
			char szGuild2[256] = { "" };
			const char* pDest1 = GetGuildByID(SpawnA->GuildID);
			const char* pDest2 = GetGuildByID(SpawnB->GuildID);

			if (pDest1)
			{
				strcpy_s(szGuild1, pDest1);
			}

			if (pDest2)
			{
				strcpy_s(szGuild2, pDest2);
			}

			return _stricmp(szGuild1, szGuild2) < 0;
		}

		case SearchSortBy::Id:
		default:
			return SpawnA->SpawnID < SpawnB->SpawnID;
		}
	}

private:
	SearchSortBy m_sortBy;
	SPAWNINFO* m_pOrigin;
};

void SuperWhoDisplay(SPAWNINFO* pChar, MQSpawnSearch* pSearchSpawn, DWORD Color)
{
	if (!pSearchSpawn)
		return;

	std::vector<SPAWNINFO*> SpawnSet;

	SPAWNINFO* pSpawn = (SPAWNINFO*)pSpawnList;
	SPAWNINFO* pOrigin = nullptr;

	if (pSearchSpawn->FromSpawnID)
		pOrigin = (SPAWNINFO*)GetSpawnByID(pSearchSpawn->FromSpawnID);
	if (!pOrigin)
		pOrigin = pChar;

	while (pSpawn)
	{
		if (SpawnMatchesSearch(pSearchSpawn, pOrigin, pSpawn))
		{
			// matches search, add to our set
			SpawnSet.push_back(pSpawn);
		}

		pSpawn = pSpawn->pNext;
	}

	if (!SpawnSet.empty())
	{
		if (SpawnSet.size() > 1)
		{
			// sort our list
			std::sort(std::begin(SpawnSet), std::end(SpawnSet),
				SuperWhoSortPredicate{ pSearchSpawn->SortBy, pOrigin });
		}

		WriteChatColor("List of matching spawns", USERCOLOR_WHO);
		WriteChatColor("--------------------------------", USERCOLOR_WHO);

		for (SPAWNINFO* spawn : SpawnSet)
		{
			SuperWhoDisplay(spawn, Color);
		}

		char* pszSpawnType = nullptr;
		switch (pSearchSpawn->SpawnType)
		{
		case NONE:
		default:
			pszSpawnType = "any";
			break;
		case PC:
			pszSpawnType = "pc";
			break;
		case MOUNT:
			pszSpawnType = "mount";
			break;
		case PET:
			pszSpawnType = "pet";
			break;
		case PCPET:
			pszSpawnType = "pcpet";
			break;
		case NPCPET:
			pszSpawnType = "npcpet";
			break;
		case XTARHATER:
			pszSpawnType = "xtarhater";
			break;
		case NPC:
			pszSpawnType = "npc";
			break;
		case CORPSE:
			pszSpawnType = "corpse";
			break;
		case TRIGGER:
			pszSpawnType = "trigger";
			break;
		case TRAP:
			pszSpawnType = "trap";
			break;
		case CHEST:
			pszSpawnType = "chest";
			break;
		case TIMER:
			pszSpawnType = "timer";
			break;
		case UNTARGETABLE:
			pszSpawnType = "untargetable";
			break;
		case MERCENARY:
			pszSpawnType = "mercenary";
			break;
		case FLYER:
			pszSpawnType = "flyer";
			break;
		}

		if (CHARINFO* pCharinf = GetCharInfo())
		{
			size_t count = SpawnSet.size();

			WriteChatf("There %s \ag%d\ax %s%s in %s.",
				(count == 1) ? "is" : "are", count, pszSpawnType, (count == 1) ? "" : "s", GetFullZone(pCharinf->zoneId));
		}
	}
	else
	{
		WriteChatColor("List of matching spawns", USERCOLOR_WHO);
		WriteChatColor("--------------------------------", USERCOLOR_WHO);

		char szMsg[MAX_STRING] = { 0 };
		WriteChatColorf("%s was not found.", USERCOLOR_WHO, FormatSearchSpawn(szMsg, sizeof(szMsg), pSearchSpawn));
	}
}

float StateHeightMultiplier(DWORD StandState)
{
	switch (StandState) {
	case STANDSTATE_BIND:
	case STANDSTATE_DUCK:
		return 0.5f;
	case STANDSTATE_SIT:
		return 0.3f;
	case STANDSTATE_FEIGN:
	case STANDSTATE_DEAD:
		return 0.1f;
	case STANDSTATE_STAND:
	default:
		return 0.9f;
	}
}

int FindSpellListByName(const char* szName)
{
	for (int Index = 0; Index < NUM_SPELL_SETS; Index++)
	{
		if (!_stricmp(pSpellSets[Index].Name, szName))
			return Index;
	}

	return -1;
}

char* GetFriendlyNameForGroundItem(PGROUNDITEM pItem, char* szName, size_t BufferSize)
{
	szName[0] = 0;
	if (!pItem)
		return &szName[0];

	int Item = atoi(pItem->Name + 2);
	ACTORDEFENTRY* ptr = ActorDefList;
	while (ptr->Def)
	{
		if (ptr->Def == Item
			&& (ptr->ZoneID && (ptr->ZoneID < 0 || ptr->ZoneID == (pItem->ZoneID & 0x7FFF))))
		{
			sprintf_s(szName, BufferSize, "%s", ptr->Name);
			return &szName[0];
		}
		ptr++;

	}

	sprintf_s(szName, BufferSize, "Drop%05d/%d", Item, pItem->DropID);
	return szName;
}

void WriteFilterNames()
{
	char szBuffer[MAX_STRING] = { 0 };
	int filternumber = 1;

	MQFilter* pFilter = gpFilters;
	WritePrivateProfileSection("Filter Names", szBuffer, gszINIFilename);

	while (pFilter)
	{
		if (pFilter->pEnabled == &gFilterCustom)
		{
			sprintf_s(szBuffer, "Filter%d", filternumber++);
			WritePrivateProfileString("Filter Names", szBuffer, pFilter->FilterText, gszINIFilename);
		}
		pFilter = pFilter->pNext;
	}
}

bool GetShortBuffID(SPELLBUFF* pBuff, int& nID)
{
	PcProfile* pProfile = GetPcProfile();

	int index = (pBuff - &pProfile->ShortBuff[0]);
	if (index < NUM_SHORT_BUFFS)
	{
		nID = index + 1;
		return true;
	}

	return false;
}

bool GetBuffID(SPELLBUFF* pBuff, int& nID)
{
	PcProfile* pProfile = GetPcProfile();

	int index = (pBuff - &pProfile->Buff[0]);
	if (index < NUM_LONG_BUFFS)
	{
		nID = index + 1;
		return true;
	}
	return false;
}

#define IS_SET(flag, bit)   ((flag) & (bit))

const char* GetLDoNTheme(int LDTheme)
{
	if (LDTheme == 31) return "All";
	if (IS_SET(LDTheme, LDON_DG)) return "Deepest Guk";
	if (IS_SET(LDTheme, LDON_MIR)) return "Miragul's";
	if (IS_SET(LDTheme, LDON_MIS)) return "Mistmoore";
	if (IS_SET(LDTheme, LDON_RUJ)) return "Rujarkian";
	if (IS_SET(LDTheme, LDON_TAK)) return "Takish";
	return "Unknown";
}

uint32_t GetItemTimer(CONTENTS* pItem)
{
	uint32_t Timer = pPCData->GetItemRecastTimer((EQ_Item*)& pItem, eActivatableSpell);

	if (Timer < GetFastTime())
		return 0;

	return Timer - GetFastTime();
}

CONTENTS* GetItemContentsBySlotID(int dwSlotID)
{
	int InvSlot = -1;
	int SubSlot = -1;

	if (dwSlotID >= 0 && dwSlotID < NUM_INV_SLOTS)
		InvSlot = dwSlotID;
	else if (dwSlotID >= 262 && dwSlotID < 342)
	{
		InvSlot = BAG_SLOT_START + (dwSlotID - 262) / 10;
		SubSlot = (dwSlotID - 262) % 10;
	}

	if (InvSlot >= 0 && InvSlot < NUM_INV_SLOTS)
	{
		if (PcProfile* pProfile = GetPcProfile())
		{
			if (pProfile->pInventoryArray)
			{
				if (CONTENTS* iSlot = pProfile->pInventoryArray->InventoryArray[InvSlot])
				{
					if (SubSlot < 0)
						return iSlot;

					if (iSlot->Contents.ContainedItems.pItems)
					{
						if (CONTENTS* sSlot = iSlot->GetContent(SubSlot))
						{
							return sSlot;
						}
					}
				}
			}
		}
	}
	return nullptr;
}

CONTENTS* GetItemContentsByName(const char* ItemName)
{
	if (PcProfile* pProfile = GetPcProfile())
	{
		if (pProfile->pInventoryArray && pProfile->pInventoryArray->InventoryArray)
		{
			for (int nSlot = 0; nSlot < NUM_INV_SLOTS; nSlot++)
			{
				if (CONTENTS* pItem = pProfile->pInventoryArray->InventoryArray[nSlot])
				{
					if (!_stricmp(ItemName, GetItemFromContents(pItem)->Name))
					{
						return pItem;
					}
				}
			}

			for (int nPack = 0; nPack < NUM_INV_BAG_SLOTS; nPack++)
			{
				if (CONTENTS* pPack = pProfile->pInventoryArray->Inventory.Pack[nPack])
				{
					if (GetItemFromContents(pPack)->Type == ITEMTYPE_PACK && pPack->Contents.ContainedItems.pItems)
					{
						for (int nItem = 0; nItem < GetItemFromContents(pPack)->Slots; nItem++)
						{
							if (CONTENTS* pItem = pPack->GetContent(nItem))
							{
								if (!_stricmp(ItemName, GetItemFromContents(pItem)->Name))
								{
									return pItem;
								}
							}
						}
					}
				}
			}
		}
	}

	return nullptr;
}

CXWnd* GetParentWnd(CXWnd* pWnd)
{
	while (pWnd)
	{
		if (!pWnd->GetParentWindow())
			return pWnd;

		pWnd = pWnd->GetParentWindow();
	}

	return nullptr;
}

bool LoH_HT_Ready()
{
	unsigned int i = ((SPAWNINFO*)pLocalPlayer)->SpellGemETA[InnateETA];
	unsigned int j = i - ((CDISPLAY*)pDisplay)->TimeStamp;
	return i < j;
}

int GetSkillIDFromName(const char* name)
{
	for (int i = 0; i < NUM_SKILLS; i++)
	{
		if (SKILL* pSkill = pSkillMgr->pSkill[i])
		{
			if (!_stricmp(name, pStringTable->getString(pSkill->nName)))
				return i;
		}
	}

	return 0;
}

bool InHoverState()
{
	if (GetCharInfo() && GetCharInfo()->Stunned == 3)
		return true;

	return false;
}

int GetGameState()
{
	if (!ppEverQuest || !pEverQuest)
	{
		return -1;
	}

	return ((EVERQUEST*)pEverQuest)->GameState;
}

int GetWorldState()
{
	if (!ppEverQuest || !pEverQuest)
	{
		return -1;
	}
	return ((EVERQUEST*)pEverQuest)->WorldState;
}

// ***************************************************************************
// Function:    LargerEffectTest
// Description: Return boolean true if the spell effect is to be ignored
//              for stacking purposes
// ***************************************************************************
bool LargerEffectTest(SPELL* aSpell, SPELL* bSpell, int i, bool bTriggeredEffectCheck)
{
	int aAttrib = GetSpellNumEffects(aSpell) > i ? GetSpellAttrib(aSpell, i) : 254;
	int bAttrib = GetSpellNumEffects(bSpell) > i ? GetSpellAttrib(bSpell, i) : 254;
	if (aAttrib == bAttrib)			// verify they are the same, we can do fewer checks this way
//		&& (aAttrib == 1			// Ac Mod
//			|| aAttrib == 2			// ATK*
//			|| aAttrib == 15		// Mana*
//			|| aAttrib == 55		// Add Effect: Absorb Damage
//			|| aAttrib == 69		// Max HP Mod
//			|| aAttrib == 79		// HP Mod
//			|| aAttrib == 114		// Aggro Multiplier
//			|| aAttrib == 127		// Spell Haste
//			|| aAttrib == 162))		// Mitigate Melee Damage*
									// We don't need to check NumEffects again since it wouldn't reach here if it would be too big
		return (abs(GetSpellBase(aSpell, i)) >= abs(GetSpellBase(bSpell, i)) || (bTriggeredEffectCheck && (aSpell->SpellGroup == bSpell->SpellGroup)));
	return false;
}

// ***************************************************************************
// Function:    TriggeringEffectSpell
// Description: Return boolean true if the spell effect is to be ignored
//              for stacking purposes
// ***************************************************************************
bool TriggeringEffectSpell(SPELL* aSpell, int i)
{
	int aAttrib = GetSpellNumEffects(aSpell) > i ? GetSpellAttrib(aSpell, i) : 254;
	return (aAttrib == 85	// Add Proc
		|| aAttrib == 374 	// Trigger Spell
		|| aAttrib == 419);	// Contact_Ability_2
}

// ***************************************************************************
// Function:    SpellEffectTest
// Description: Return boolean true if the spell effect is to be ignored
//              for stacking purposes
// ***************************************************************************
bool SpellEffectTest(SPELL* aSpell, SPELL* bSpell, int i, bool bIgnoreTriggeringEffects, bool bTriggeredEffectCheck = false)
{
	int aAttrib = GetSpellNumEffects(aSpell) > i ? GetSpellAttrib(aSpell, i) : 254;
	int bAttrib = GetSpellNumEffects(bSpell) > i ? GetSpellAttrib(bSpell, i) : 254;
	return ((aAttrib == 57 || bAttrib == 57)		// Levitate
		|| (aAttrib == 134 || bAttrib == 134)		// Limit: Max Level
		|| (aAttrib == 135 || bAttrib == 135)		// Limit: Resist
		|| (aAttrib == 136 || bAttrib == 136)		// Limit: Target
		|| (aAttrib == 137 || bAttrib == 137)		// Limit: Effect
		|| (aAttrib == 138 || bAttrib == 138)		// Limit: SpellType
		|| (aAttrib == 139 || bAttrib == 139)		// Limit: Spell
		|| (aAttrib == 140 || bAttrib == 140)		// Limit: Min Duraction
		|| (aAttrib == 141 || bAttrib == 141)		// Limit: Instant
		|| (aAttrib == 142 || bAttrib == 142)		// Limit: Min Level
		|| (aAttrib == 143 || bAttrib == 143)		// Limit: Min Cast Time
		|| (aAttrib == 144 || bAttrib == 144)		// Limit: Max Cast Time
		|| (aAttrib == 254 || bAttrib == 254)		// Placeholder
		|| (aAttrib == 311 || bAttrib == 311)		// Limit: Combat Skills not Allowed
		|| (aAttrib == 339 || bAttrib == 339)		// Trigger DoT on cast
		|| (aAttrib == 340 || bAttrib == 340)		// Trigger DD on cast
		|| (aAttrib == 348 || bAttrib == 348)		// Limit: Min Mana
//		|| (aAttrib == 374 || bAttrib == 374)		// Add Effect: xxx
		|| (aAttrib == 385 || bAttrib == 385)		// Limit: SpellGroup
		|| (aAttrib == 391 || bAttrib == 391)		// Limit: Max Mana
		|| (aAttrib == 403 || bAttrib == 403)		// Limit: SpellClass
		|| (aAttrib == 404 || bAttrib == 404)		// Limit: SpellSubclass
		|| (aAttrib == 411 || bAttrib == 411)		// Limit: PlayerClass
		|| (aAttrib == 412 || bAttrib == 412)		// Limit: Race
		|| (aAttrib == 414 || bAttrib == 414)		// Limit: CastingSkill
		|| (aAttrib == 422 || bAttrib == 422)		// Limit: Use Min
		|| (aAttrib == 423 || bAttrib == 423)		// Limit: Use Type
		|| (aAttrib == 428 || bAttrib == 428)		// Skill_Proc_Modifier
		|| (LargerEffectTest(aSpell, bSpell, i, bTriggeredEffectCheck))	// Ignore if the new effect is greater than the old effect
		|| (bIgnoreTriggeringEffects && (TriggeringEffectSpell(aSpell, i) || TriggeringEffectSpell(bSpell, i)))		// Ignore triggering effects validation
		|| ((aSpell->SpellType == 1 || aSpell->SpellType == 2) && (bSpell->SpellType == 1 || bSpell->SpellType == 2) && !(aSpell->DurationWindow == bSpell->DurationWindow)));
}

// ***************************************************************************
// Function:    BuffStackTest
// Description: Return boolean true if the two spells will stack
// Usage:       Used by ${Spell[xxx].Stacks}, ${Spell[xxx].StacksPet},
//                ${Spell[xxx].WillStack[yyy]}, ${Spell[xxx].StacksWith[yyy]}
// Author:      Pinkfloydx33
// ***************************************************************************
bool BuffStackTest(SPELL* aSpell, SPELL* bSpell, bool bIgnoreTriggeringEffects, bool bTriggeredEffectCheck)
{
	SPAWNINFO* pSpawn = (SPAWNINFO*)pLocalPlayer;
	if (!pSpawn || !pSpawn->GetPcClient())
		return true;
	if (GetGameState() != GAMESTATE_INGAME)
		return true;
	if (gZoning)
		return true;
	if (!aSpell || !bSpell)
		return false;
	if (IsBadReadPtr((void*)aSpell, 4))
		return false;
	if (IsBadReadPtr((void*)bSpell, 4))
		return false;
	if (aSpell->ID == bSpell->ID)
		return true;

	if (gStackingDebug)
	{
		char szStackingDebug[MAX_STRING] = { 0 };
		sprintf_s(szStackingDebug, "aSpell->Name=%s(%d) bSpell->Name=%s(%d)",
			aSpell->Name, aSpell->ID, bSpell->Name, bSpell->ID);

		DebugSpewAlwaysFile("%s", szStackingDebug);

		if (gStackingDebug == -1)
			WriteChatColor(szStackingDebug, USERCOLOR_CHAT_CHANNEL);
	}

	// We need to loop over the largest of the two, this may seem silly but one could have stacking command blocks
	// which we will always need to check.
	int effects = std::max(GetSpellNumEffects(aSpell), GetSpellNumEffects(bSpell));
	for (int i = 0; i < effects; i++)
	{
		// Compare 1st Buff to 2nd. If Attrib[i]==254 its a place holder. If it is 10 it
		// can be 1 of 3 things: PH(Base=0), CHA(Base>0), Lure(Base=-6). If it is Lure or
		// Placeholder, exclude it so slots don't match up. Now Check to see if the slots
		// have equal attribute values. If the do, they don't stack.

		int aAttrib = 254, bAttrib = 254; // Default to placeholder ...
		int aBase = 0, bBase = 0, aBase2 = 0, bBase2 = 0;

		if (GetSpellNumEffects(aSpell) > i)
		{
			aAttrib = GetSpellAttrib(aSpell, i);
			aBase = GetSpellBase(aSpell, i);
			aBase2 = GetSpellBase2(aSpell, i);
		}

		if (GetSpellNumEffects(bSpell) > i)
		{
			bAttrib = GetSpellAttrib(bSpell, i);
			bBase = GetSpellBase(bSpell, i);
			bBase2 = GetSpellBase2(bSpell, i);
		}

		if (gStackingDebug)
		{
			char szStackingDebug[MAX_STRING] = { 0 };
			sprintf_s(szStackingDebug, "Slot %d: bSpell->Attrib=%d, bSpell->Base=%d, bSpell->TargetType=%d, aSpell->Attrib=%d, aSpell->Base=%d, aSpell->TargetType=%d", i, bAttrib, bBase, bSpell->TargetType, aAttrib, aBase, aSpell->TargetType);
			DebugSpewAlwaysFile("%s", szStackingDebug);

			if (gStackingDebug == -1)
				WriteChatColor(szStackingDebug, USERCOLOR_CHAT_CHANNEL);
		}

		bool bTriggerA = TriggeringEffectSpell(aSpell, i);
		bool bTriggerB = TriggeringEffectSpell(bSpell, i);
		if (bTriggerA || bTriggerB)
		{
			SPELL* pRetSpellA = GetSpellByID(bTriggerA ? (aAttrib == 374 ? aBase2 : aBase) : aSpell->ID);
			SPELL* pRetSpellB = GetSpellByID(bTriggerB ? (bAttrib == 374 ? bBase2 : bBase) : bSpell->ID);

			if (!pRetSpellA || !pRetSpellB)
			{
				if (gStackingDebug)
				{
					char szStackingDebug[MAX_STRING] = { 0 };
					sprintf_s(szStackingDebug, "BuffStackTest ERROR: aSpell[%d]:%s%s, bSpell[%d]:%s%s",
						aSpell->ID, aSpell->Name, pRetSpellA ? "" : "is null", bSpell->ID, bSpell->Name, pRetSpellB ? "" : "is null");
					DebugSpewAlwaysFile("%s", szStackingDebug);

					if (gStackingDebug == -1)
						WriteChatColor(szStackingDebug, USERCOLOR_CHAT_CHANNEL);
				}
			}

			if (!((bTriggerA && (aSpell->ID == pRetSpellA->ID)) || (bTriggerB && (bSpell->ID == pRetSpellB->ID))))
			{
				if (!BuffStackTest(pRetSpellA, pRetSpellB, bIgnoreTriggeringEffects, true))
				{
					if (gStackingDebug)
					{
						DebugSpewAlwaysFile("returning false #1");
						if (gStackingDebug == -1)
							WriteChatColor("returning false #1", USERCOLOR_CHAT_CHANNEL);
					}
					return false;
				}
			}
		}

		if (bAttrib == aAttrib && !SpellEffectTest(aSpell, bSpell, i, bIgnoreTriggeringEffects, bTriggeredEffectCheck))
		{
			if (gStackingDebug)
			{
				DebugSpewAlwaysFile("Inside IF");
				if (gStackingDebug == -1)
					WriteChatColor("Inside IF", USERCOLOR_CHAT_CHANNEL);
			}

			if (!((bAttrib == 10 && (bBase == -6 || bBase == 0))
				|| (aAttrib == 10 && (aBase == -6 || aBase == 0))
				|| (bAttrib == 79 && bBase > 0 && bSpell->TargetType == 6)
				|| (aAttrib == 79 && aBase > 0 && aSpell->TargetType == 6)
				|| (bAttrib == 0 && bBase < 0)
				|| (aAttrib == 0 && aBase < 0)
				|| (bAttrib == 148 || bAttrib == 149)
				|| (aAttrib == 148 || aAttrib == 149)))
			{
				if (gStackingDebug)
				{
					DebugSpewAlwaysFile("returning false #2");
					if (gStackingDebug == -1)
						WriteChatColor("returning false #2", USERCOLOR_CHAT_CHANNEL);
				}
				return false;
			}
		}

		// Check to see if second buffs blocks first buff:
		// 148: Stacking: Block new spell if slot %d is effect
		// 149: Stacking: Overwrite existing spell if slot %d is effect
		if (bAttrib == 148 || bAttrib == 149)
		{
			// in this branch we know bSpell has enough slots
			int tmpSlot = (bAttrib == 148 ? bBase2 - 1 : GetSpellCalc(bSpell, i) - 200 - 1);
			int tmpAttrib = bBase;

			if (GetSpellNumEffects(aSpell) > tmpSlot)
			{
				// verify aSpell has that slot
				if (gStackingDebug)
				{
					char szStackingDebug[MAX_STRING] = { 0 };
					snprintf(szStackingDebug, sizeof(szStackingDebug), "aSpell->Attrib[%d]=%d, aSpell->Base[%d]=%d, tmpAttrib=%d, tmpVal=%d", tmpSlot, GetSpellAttrib(aSpell, tmpSlot), tmpSlot, GetSpellBase(aSpell, tmpSlot), tmpAttrib, abs(GetSpellMax(bSpell, i)));
					DebugSpewAlwaysFile("%s", szStackingDebug);
					if (gStackingDebug == -1)
						WriteChatColor(szStackingDebug, USERCOLOR_CHAT_CHANNEL);
				}

				if (GetSpellMax(bSpell, i) > 0)
				{
					int tmpVal = abs(GetSpellMax(bSpell, i));
					if (GetSpellAttrib(aSpell, tmpSlot) == tmpAttrib && GetSpellBase(aSpell, tmpSlot) < tmpVal)
					{
						if (gStackingDebug)
						{
							DebugSpewAlwaysFile("returning false #3");
							if (gStackingDebug == -1)
								WriteChatColor("returning false #3", USERCOLOR_CHAT_CHANNEL);
						}
						return false;
					}
				}
				else if (GetSpellAttrib(aSpell, tmpSlot) == tmpAttrib)
				{
					if (gStackingDebug)
					{
						DebugSpewAlwaysFile("returning false #4");
						if (gStackingDebug == -1)
							WriteChatColor("returning false #4", USERCOLOR_CHAT_CHANNEL);
					}
					return false;
				}
			}
		}

		/*
		//Now Check to see if the first buff blocks second buff. This is necessary
		//because only some spells carry the Block Slot. Ex. Brells and Spiritual
		//Vigor don't stack Brells has 1 slot total, for HP. Vigor has 4 slots, 2
		//of which block Brells.
		if (aAttrib == 148 || aAttrib == 149) {
			// in this branch we know aSpell has enough slots
			int tmpSlot = (aAttrib == 148 ? aBase2 - 1 : GetSpellCalc(aSpell, i) - 200 - 1);
			int tmpAttrib = aBase;
			if (GetSpellNumEffects(bSpell) > tmpSlot) { // verify bSpell has that slot
				if (gStackingDebug) {
					char szStackingDebug[MAX_STRING] = { 0 };
					snprintf(szStackingDebug, sizeof(szStackingDebug), "bSpell->Attrib[%d]=%d, bSpell->Base[%d]=%d, tmpAttrib=%d, tmpVal=%d", tmpSlot, GetSpellAttrib(bSpell, tmpSlot), tmpSlot, GetSpellBase(bSpell, tmpSlot), tmpAttrib, abs(GetSpellMax(aSpell, i)));
					DebugSpewAlwaysFile("%s", szStackingDebug);
					if (gStackingDebug == -1)
						WriteChatColor(szStackingDebug, USERCOLOR_CHAT_CHANNEL);
				}
				if (GetSpellMax(aSpell, i) > 0) {
					int tmpVal = abs(GetSpellMax(aSpell, i));
					if (GetSpellAttrib(bSpell, tmpSlot) == tmpAttrib && GetSpellBase(bSpell, tmpSlot) < tmpVal) {
						if (gStackingDebug) {
							DebugSpewAlwaysFile("returning false #5");
							if (gStackingDebug == -1)
								WriteChatColor("returning false #5", USERCOLOR_CHAT_CHANNEL);
						}
						return false;
					}
				}
				else if (GetSpellAttrib(bSpell, tmpSlot) == tmpAttrib) {
					if (gStackingDebug) {
						DebugSpewAlwaysFile("returning false #6");
						if (gStackingDebug == -1)
							WriteChatColor("returning false #6", USERCOLOR_CHAT_CHANNEL);
					}
					return false;
				}
			}
		}
		*/
	}

	if (gStackingDebug)
	{
		DebugSpewAlwaysFile("returning true");
		if (gStackingDebug == -1)
			WriteChatColor("returning true", USERCOLOR_CHAT_CHANNEL);
	}
	return true;
}

float GetMeleeRange(PlayerClient* pSpawn1, PlayerClient* pSpawn2)
{
	if (pSpawn1 && pSpawn2)
	{
		float f = ((SPAWNINFO*)pSpawn1)->GetMeleeRangeVar1 * ((SPAWNINFO*)pSpawn1)->MeleeRadius;
		float g = ((SPAWNINFO*)pSpawn2)->GetMeleeRangeVar1 * ((SPAWNINFO*)pSpawn2)->MeleeRadius;

		float h = abs(((SPAWNINFO*)pSpawn1)->AvatarHeight - ((SPAWNINFO*)pSpawn2)->AvatarHeight);

		f = (f + g) * 0.75f;

		if (f < 14.0f)
			f = 14.0f;

		g = f + 2 + h;

		if (g > 75.0f)
			return 75.0f;
		else
			return g;
	}
	return 14.0f;
}

bool IsValidSpellIndex(int index)
{
	if ((index < 1) || (index > TOTAL_SPELL_COUNT))
		return false;
	return true;
}

inline bool IsValidSpellSlot(int nGem)
{
	return nGem >= 0 && nGem < 16;
}

// testing new cooldown code... -eqmule work in progress
uint32_t GetSpellGemTimer2(int nGem)
{
	bool bValidSlot = IsValidSpellSlot(nGem);

	if (bValidSlot)
	{
		int memspell = GetMemorizedSpell(nGem);
		if (SPELL* pSpell = GetSpellByID(memspell))
		{
			int ReuseTimerIndex = pSpell->ReuseTimerIndex;
			unsigned int linkedtimer = ((PcZoneClient*)pPCData)->GetLinkedSpellReuseTimer(ReuseTimerIndex);

			__time32_t RecastTime = ReuseTimerIndex > 0 && ReuseTimerIndex < 25 ? linkedtimer : 0;
			unsigned int RecastDuration = 0;
			unsigned int LinkedDuration = 0;
			unsigned int gemeta = ((SPAWNINFO*)pLocalPlayer)->SpellGemETA[nGem];
			DWORD now = ((CDISPLAY*)pDisplay)->TimeStamp;
			if (gemeta > now)
			{
				RecastDuration = gemeta - now;
			}
			__time32_t fasttime = (__time32_t)GetFastTime();
			if (RecastTime > fasttime)
			{
				LinkedDuration = (RecastTime - fasttime) * 1000;
			}
			CSpellGemWnd* gem = pCastSpellWnd->SpellSlots[nGem];
			unsigned int Timer = std::max(RecastDuration, LinkedDuration);
			unsigned int timeremaining = gem->GetCoolDownTimeRemaining();
			unsigned int totaldur = gem->GetCoolDownTotalDuration();

			bool TimerChanged = !(abs(long(Timer - timeremaining)) < 1000);
			if (Timer > 0 && (totaldur == 0 || TimerChanged))
			{
				int TotalDuration = Timer;
				if (RecastDuration > LinkedDuration)
				{
					VePointer<CONTENTS> pFocusItem;
					int ReuseMod = pCharData->GetFocusReuseMod((EQ_Spell*)pSpell, pFocusItem);
					TotalDuration = pSpell->RecastTime - ReuseMod;
				}
				//do stuff
				return TotalDuration;
			}
			return Timer;
		}
	}

	return 0;
}

// todo: check manually
uint32_t GetSpellGemTimer(int nGem)
{
	return GetSpellGemTimer2(nGem);
}

uint32_t GetSpellBuffTimer(int SpellID)
{
	for (int nBuff = 0; nBuff < NUM_LONG_BUFFS; nBuff++)
	{
		if (pBuffWnd->BuffId[nBuff] == SpellID)
		{
			if (pBuffWnd->BuffTimer[nBuff]) {
				return pBuffWnd->BuffTimer[nBuff];
			}
		}
	}

	// look, this probably is an oversight by the eqdevs
	// but the struct only have 0x2a BuffTimers so...
	// even though there could be 0x37 shortbuffs
	// we can only check up to 0x2a...
	for (int nBuff = 0; nBuff < NUM_LONG_BUFFS; nBuff++)
	{
		if (pSongWnd->BuffId[nBuff] == SpellID)
		{
			if (pSongWnd->BuffTimer[nBuff])
			{
				return pSongWnd->BuffTimer[nBuff];
			}
		}
	}
	return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Functions that were built into commands and people used DoCommand to execute                  //

void AttackRanged(PlayerClient* pRangedTarget)
{
	if (pRangedTarget && gbRangedAttackReady)
	{
		pLocalPlayer->DoAttack(InvSlot_Range, 0, pRangedTarget);
		gbRangedAttackReady = 0;
	}
}

void UseAbility(const char* sAbility)
{
	char szBuffer[MAX_STRING] = { 0 };
	strcpy_s(szBuffer, sAbility);

	if (!cmdDoAbility)
	{
		CMDLIST* pCmdListOrig = (CMDLIST*)EQADDR_CMDLIST;

		for (int i = 0; pCmdListOrig[i].fAddress != nullptr; i++)
		{
			if (ci_equals(pCmdListOrig[i].szName, "/doability"))
			{
				cmdDoAbility = (fEQCommand)pCmdListOrig[i].fAddress;
			}
		}
	}

	if (!cmdDoAbility)
		return;

	if (atoi(szBuffer) || !EQADDR_DOABILITYLIST)
	{
		cmdDoAbility((SPAWNINFO*)pLocalPlayer, szBuffer);
		return;
	}

	SPAWNINFO* pChar = (SPAWNINFO*)pLocalPlayer;
	int DoIndex = -1;

	for (int Index = 0; Index < 10; Index++)
	{
		if (EQADDR_DOABILITYLIST[Index] != -1)
		{
			if (!_strnicmp(szBuffer, szSkills[EQADDR_DOABILITYLIST[Index]], strlen(szSkills[EQADDR_DOABILITYLIST[Index]])))
			{
				if (Index < 4)
				{
					DoIndex = Index + 7; // 0-3 = Combat abilities (7-10)
				}
				else
				{
					DoIndex = Index - 3; // 4-9 = Abilities (1-6)
				}
			}
		}
	}

	if (DoIndex != -1)
	{
		_itoa_s(DoIndex, szBuffer, 10);
		cmdDoAbility(pChar, szBuffer);
	}
	else
	{
		int Index = 0;
		if (PcProfile* pProfile = GetPcProfile())
		{
			for (; Index < NUM_COMBAT_ABILITIES; Index++)
			{
				if (pCombatSkillsSelectWnd->ShouldDisplayThisSkill(Index))
				{
					if (SPELL* pCA = GetSpellByID(pProfile->CombatAbilities[Index]))
					{
						if (!_stricmp(pCA->Name, szBuffer))
						{
							// We got the cookie, let's try and do it
							pCharData->DoCombatAbility(pCA->ID);
							break;
						}
					}
				}
			}
		}

		if (Index >= NUM_COMBAT_ABILITIES)
			WriteChatColor("You do not seem to have that ability available", USERCOLOR_DEFAULT);
	}
}

// Function to check if the account has a given expansion enabled.
// Pass exansion macros from EQData.h to it -- e.g. HasExpansion(EXPANSION_RoF)
bool HasExpansion(int nExpansion)
{
	return (GetCharInfo()->ExpansionFlags & nExpansion) != 0;
}

// Just a Function that needs more work
// I use this to test merc aa struct -eqmule
void ListMercAltAbilities()
{
	if (pMercAltAbilities)
	{
		int mercaapoints = ((CHARINFO*)pCharData)->MercAAPoints;

		for (int i = 0; i < MERC_ALT_ABILITY_COUNT; i++)
		{
			PEQMERCALTABILITIES pinfo = (PEQMERCALTABILITIES)pMercAltAbilities;
			if (pinfo->MercAAInfo[i])
			{
				if (pinfo->MercAAInfo[i]->Ptr)
				{
					int nName = pinfo->MercAAInfo[i]->Ptr->nName;
					int maxpoints = pinfo->MercAAInfo[i]->Max;

					if (nName)
					{
						WriteChatf("You have %d mercaapoints to spend on %s (max is %d)",
							mercaapoints, pCDBStr->GetString(nName, eMercenaryAbilityName), maxpoints);
					}
				}
			}
		}
	}
}

CONTENTS* FindItemBySlot2(const ItemGlobalIndex& idx)
{
	return FindItemBySlot(idx.GetTopSlot(), idx.GetIndex().GetSlot(1), idx.GetLocation());
}

CONTENTS* FindItemBySlot(short InvSlot, short BagSlot, ItemContainerInstance location)
{
	CHARINFO* pChar = GetCharInfo();
	PcProfile* pProfile = GetPcProfile();

	if (!pChar || !pProfile)
		return nullptr;

	if (location == eItemContainerPossessions)
	{
		// check regular inventory
		if (pProfile->pInventoryArray && pProfile->pInventoryArray->InventoryArray)
		{
			for (int nSlot = 0; nSlot < NUM_INV_SLOTS; nSlot++)
			{
				if (CONTENTS* pItem = pProfile->pInventoryArray->InventoryArray[nSlot])
				{
					if (pItem->GetGlobalIndex().GetTopSlot() == InvSlot && pItem->GetGlobalIndex().GetIndex().GetSlot(1) == BagSlot)
					{
						return pItem;
					}
				}
			}
		}

		// check inside bags
		if (pProfile->pInventoryArray)
		{
			for (int nPack = 0; nPack < NUM_INV_BAG_SLOTS; nPack++)
			{
				if (CONTENTS* pPack = pProfile->pInventoryArray->Inventory.Pack[nPack])
				{
					if (GetItemFromContents(pPack)->Type == ITEMTYPE_PACK && pPack->Contents.ContainedItems.pItems)
					{
						for (int nItem = 0; nItem < GetItemFromContents(pPack)->Slots; nItem++)
						{
							if (CONTENTS* pItem = pPack->GetContent(nItem))
							{
								if (pItem->GetGlobalIndex().GetTopSlot() == InvSlot
									&& pItem->GetGlobalIndex().GetIndex().GetSlot(1) == BagSlot)
								{
									return pItem;
								}
							}
						}
					}
				}
			}
		}
	}
	else if (location == eItemContainerBank)
	{
		// check bank
		if (pChar->pBankArray && pChar->pBankArray->Bank)
		{
			for (int nSlot = 0; nSlot < NUM_BANK_SLOTS; nSlot++)
			{
				if (CONTENTS* pItem = pChar->pBankArray->Bank[nSlot])
				{
					if (pItem->GetGlobalIndex().GetTopSlot() == InvSlot
						&& pItem->GetGlobalIndex().GetIndex().GetSlot(1) == BagSlot)
					{
						return pItem;
					}
				}
			}
		}

		// check inside bank bags
		if (pChar->pBankArray && pChar->pBankArray->Bank)
		{
			for (int nPack = 0; nPack < NUM_BANK_SLOTS; nPack++)
			{
				if (CONTENTS* pPack = pChar->pBankArray->Bank[nPack])
				{
					if (GetItemFromContents(pPack)->Type == ITEMTYPE_PACK && pPack->Contents.ContainedItems.pItems)
					{
						for (int nItem = 0; nItem < GetItemFromContents(pPack)->Slots; nItem++)
						{
							if (CONTENTS* pItem = pPack->GetContent(nItem))
							{
								if (pItem->GetGlobalIndex().GetTopSlot() == InvSlot
									&& pItem->GetGlobalIndex().GetIndex().GetSlot(1) == BagSlot)
								{
									return pItem;
								}
							}
						}
					}
				}
			}
		}
	}
	else if (location == eItemContainerSharedBank)
	{
		// check shared bank
		if (pChar->pSharedBankArray && pChar->pSharedBankArray->SharedBank)
		{
			for (int nSlot = 0; nSlot < NUM_SHAREDBANK_SLOTS; nSlot++)
			{
				if (CONTENTS* pItem = pChar->pSharedBankArray->SharedBank[nSlot])
				{
					if (pItem->GetGlobalIndex().GetTopSlot() == InvSlot && pItem->GetGlobalIndex().GetIndex().GetSlot(1) == BagSlot) {
						return pItem;
					}
				}
			}
		}

		// check inside shared bank bags
		if (pChar && pChar->pSharedBankArray && pChar->pSharedBankArray->SharedBank)
		{
			for (int nPack = 0; nPack < NUM_SHAREDBANK_SLOTS; nPack++)
			{
				if (CONTENTS* pPack = pChar->pSharedBankArray->SharedBank[nPack])
				{
					if (GetItemFromContents(pPack)->Type == ITEMTYPE_PACK && pPack->Contents.ContainedItems.pItems)
					{
						for (int nItem = 0; nItem < GetItemFromContents(pPack)->Slots; nItem++)
						{
							if (CONTENTS* pItem = pPack->GetContent(nItem))
							{
								if (pItem->GetGlobalIndex().GetTopSlot() == InvSlot
									&& pItem->GetGlobalIndex().GetIndex().GetSlot(1) == BagSlot)
								{
									return pItem;
								}
							}
						}
					}
				}
			}
		}
	}

	return nullptr;
}

template <typename T>
CONTENTS* FindItem(T&& callback)
{
	PcProfile* pProfile = GetPcProfile();
	CHARINFO* pChar = GetCharInfo();

	if (!pProfile || !pChar)
		return nullptr;

	// check cursor
	if (pProfile->pInventoryArray && pProfile->pInventoryArray->Inventory.Cursor)
	{
		if (CONTENTS* pPack = pProfile->pInventoryArray->Inventory.Cursor)
		{
			ITEMINFO* pPackInfo = GetItemFromContents(pPack);

			if (callback(pPack, pPackInfo))
				return pPack;

			if (pPackInfo->Type != ITEMTYPE_PACK)
			{
				// it's not a pack we should check for augs
				if (pPack->Contents.ContainedItems.pItems && pPack->Contents.ContainedItems.Size)
				{
					for (size_t nAug = 0; nAug < pPack->Contents.ContainedItems.Size; nAug++)
					{
						if (CONTENTS* pAugItem = pPack->Contents.ContainedItems.pItems->Item[nAug])
						{
							ITEMINFO* pAugItemInfo = GetItemFromContents(pAugItem);

							if (pAugItemInfo->Type == ITEMTYPE_NORMAL && pAugItemInfo->AugType)
							{
								if (callback(pAugItem, pAugItemInfo))
									return pAugItem;
							}
						}
					}
				}
			}
			else if (pPack->Contents.ContainedItems.pItems)
			{
				// Found a pack, check the items inside.
				for (int nItem = 0; nItem < pPackInfo->Slots; nItem++)
				{
					if (CONTENTS* pItem = pPack->GetContent(nItem))
					{
						ITEMINFO* pItemInfo = GetItemFromContents(pItem);

						if (callback(pItem, pItemInfo))
							return pItem;

						// Check for augs next
						if (pItem->Contents.ContainedItems.pItems && pItem->Contents.ContainedItems.Size)
						{
							for (size_t nAug = 0; nAug < pItem->Contents.ContainedItems.Size; nAug++)
							{
								if (CONTENTS* pAugItem = pItem->Contents.ContainedItems.pItems->Item[nAug])
								{
									ITEMINFO* pAugItemInfo = GetItemFromContents(pAugItem);

									if (pAugItemInfo->Type == ITEMTYPE_NORMAL && pAugItemInfo->AugType)
									{
										if (callback(pAugItem, pAugItemInfo))
											return pAugItem;
									}
								}
							}
						}
					}
				}
			}
		}
	}

	// check toplevel slots
	if (pProfile->pInventoryArray && pProfile->pInventoryArray->InventoryArray)
	{
		for (int nSlot = 0; nSlot < NUM_INV_SLOTS; nSlot++)
		{
			if (CONTENTS* pItem = pProfile->pInventoryArray->InventoryArray[nSlot])
			{
				ITEMINFO* pItemInfo = GetItemFromContents(pItem);

				if (callback(pItem, pItemInfo))
					return pItem;

				// Check for augs next
				if (pItem->Contents.ContainedItems.pItems && pItem->Contents.ContainedItems.Size)
				{
					for (size_t nAug = 0; nAug < pItem->Contents.ContainedItems.Size; nAug++)
					{
						if (CONTENTS* pAugItem = pItem->Contents.ContainedItems.pItems->Item[nAug])
						{
							ITEMINFO* pAugItemInfo = GetItemFromContents(pAugItem);

							if (pAugItemInfo->Type == ITEMTYPE_NORMAL && pAugItemInfo->AugType)
							{
								if (callback(pAugItem, pAugItemInfo))
									return pAugItem;
							}
						}
					}
				}
			}
		}
	}

	// check the bags
	if (pProfile->pInventoryArray)
	{
		for (int nPack = 0; nPack < NUM_INV_BAG_SLOTS; nPack++)
		{
			if (CONTENTS* pPack = pProfile->pInventoryArray->Inventory.Pack[nPack])
			{
				ITEMINFO* pPackInfo = GetItemFromContents(pPack);

				if (pPackInfo->Type == ITEMTYPE_PACK && pPack->Contents.ContainedItems.pItems)
				{
					for (int nItem = 0; nItem < pPackInfo->Slots; nItem++)
					{
						if (CONTENTS* pItem = pPack->GetContent(nItem))
						{
							ITEMINFO* pItemInfo = GetItemFromContents(pItem);

							if (callback(pItem, pItemInfo))
								return pItem;

							// We should check for augs next
							if (pItem->Contents.ContainedItems.pItems && pItem->Contents.ContainedItems.Size)
							{
								for (size_t nAug = 0; nAug < pItem->Contents.ContainedItems.Size; nAug++)
								{
									if (CONTENTS* pAugItem = pItem->Contents.ContainedItems.pItems->Item[nAug])
									{
										ITEMINFO* pAugItemInfo = GetItemFromContents(pAugItem);

										if (pAugItemInfo->Type == ITEMTYPE_NORMAL && pAugItemInfo->AugType)
										{
											if (callback(pAugItem, pAugItemInfo))
												return pAugItem;
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}

	// check mount keyring
	if (pChar->pMountsArray && pChar->pMountsArray->Mounts)
	{
		for (int nSlot = 0; nSlot < MAX_KEYRINGITEMS; nSlot++)
		{
			if (CONTENTS* pItem = pChar->pMountsArray->Mounts[nSlot])
			{
				ITEMINFO* pItemInfo = GetItemFromContents(pItem);

				if (callback(pItem, pItemInfo))
					return pItem;
			}
		}
	}

	// check illusions keyring
	if (pChar->pIllusionsArray && pChar->pIllusionsArray->Illusions)
	{
		for (int nSlot = 0; nSlot < MAX_KEYRINGITEMS; nSlot++)
		{
			if (CONTENTS* pItem = pChar->pIllusionsArray->Illusions[nSlot])
			{
				ITEMINFO* pItemInfo = GetItemFromContents(pItem);

				if (callback(pItem, pItemInfo))
					return pItem;
			}
		}
	}

	// check familiars keyring
	if (pChar->pFamiliarArray && pChar->pFamiliarArray->Familiars)
	{
		for (int nSlot = 0; nSlot < MAX_KEYRINGITEMS; nSlot++)
		{
			if (CONTENTS* pItem = pChar->pFamiliarArray->Familiars[nSlot])
			{
				ITEMINFO* pItemInfo = GetItemFromContents(pItem);

				if (callback(pItem, pItemInfo))
					return pItem;
			}
		}
	}

	return nullptr;
}

CONTENTS* FindItemByName(const char* pName, bool bExact)
{
	return FindItem([pName, bExact](CONTENTS* pItem, ITEMINFO* pItemInfo)
		{ return ci_equals(pItemInfo->Name, pName, bExact); });
}

CONTENTS* FindItemByID(int ItemID)
{
	return FindItem([ItemID](CONTENTS* pItem, ITEMINFO* pItemInfo)
		{ return ItemID == pItemInfo->ItemNumber; });
}

int GetItemCount(CONTENTS* pItem)
{
	if (GetItemFromContents(pItem)->Type != ITEMTYPE_NORMAL || GetItemFromContents(pItem)->StackSize <= 1)
		return 1;

	return pItem->StackCount;
}

template <typename T>
int CountItems(T&& checkItem)
{
	PcProfile* pProfile = GetPcProfile();
	CHARINFO* pChar = GetCharInfo();

	if (!pProfile || !pChar)
		return 0;

	int Count = 0;

	// check cursor
	if (pProfile->pInventoryArray && pProfile->pInventoryArray->Inventory.Cursor)
	{
		if (CONTENTS* pPack = pProfile->pInventoryArray->Inventory.Cursor)
		{
			if (checkItem(pPack))
			{
				Count += GetItemCount(pPack);
			}

			if (GetItemFromContents(pPack)->Type != ITEMTYPE_PACK)
			{
				// should check for augs
				if (pPack->Contents.ContainedItems.pItems && pPack->Contents.ContainedItems.Size)
				{
					for (size_t nAug = 0; nAug < pPack->Contents.ContainedItems.Size; nAug++)
					{
						if (CONTENTS* pAugItem = pPack->Contents.ContainedItems.pItems->Item[nAug])
						{
							ITEMINFO* pAugItemInfo = GetItemFromContents(pAugItem);

							if (pAugItemInfo->Type == ITEMTYPE_NORMAL && pAugItemInfo->AugType)
							{
								if (checkItem(pAugItem))
								{
									Count += 1;
								}
							}
						}
					}
				}
			}
			else if (pPack->Contents.ContainedItems.pItems)
			{
				// it was a pack, if it has items in it lets check them
				for (int nItem = 0; nItem < GetItemFromContents(pPack)->Slots; nItem++)
				{
					if (CONTENTS* pItem = pPack->GetContent(nItem))
					{
						if (checkItem(pItem))
						{
							Count += GetItemCount(pItem);
						}

						// Check for augs next
						if (pItem->Contents.ContainedItems.pItems && pItem->Contents.ContainedItems.Size)
						{
							for (size_t nAug = 0; nAug < pItem->Contents.ContainedItems.Size; nAug++)
							{
								if (CONTENTS* pAugItem = pItem->Contents.ContainedItems.pItems->Item[nAug])
								{
									ITEMINFO* pAugItemInfo = GetItemFromContents(pAugItem);

									if (pAugItemInfo->Type == ITEMTYPE_NORMAL && pAugItemInfo->AugType)
									{
										if (checkItem(pAugItem))
										{
											Count += 1;
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}

	// check toplevel slots
	if (pProfile->pInventoryArray && pProfile->pInventoryArray->InventoryArray)
	{
		for (int nSlot = 0; nSlot < NUM_INV_SLOTS; nSlot++)
		{
			if (CONTENTS* pItem = pProfile->pInventoryArray->InventoryArray[nSlot])
			{
				if (checkItem(pItem))
				{
					Count += GetItemCount(pItem);
				}

				// Check for augs next
				if (pItem->Contents.ContainedItems.pItems && pItem->Contents.ContainedItems.Size)
				{
					for (size_t nAug = 0; nAug < pItem->Contents.ContainedItems.Size; nAug++)
					{
						if (CONTENTS* pAugItem = pItem->Contents.ContainedItems.pItems->Item[nAug])
						{
							ITEMINFO* pAugItemInfo = GetItemFromContents(pAugItem);

							if (pAugItemInfo->Type == ITEMTYPE_NORMAL && pAugItemInfo->AugType)
							{
								if (checkItem(pAugItem))
								{
									Count += 1;
								}
							}
						}
					}
				}
			}
		}
	}

	// check the bags
	if (pProfile->pInventoryArray)
	{
		for (int nPack = 0; nPack < NUM_INV_BAG_SLOTS; nPack++)
		{
			if (CONTENTS* pPack = pProfile->pInventoryArray->Inventory.Pack[nPack])
			{
				if (GetItemFromContents(pPack)->Type == ITEMTYPE_PACK && pPack->Contents.ContainedItems.pItems)
				{
					for (int nItem = 0; nItem < GetItemFromContents(pPack)->Slots; nItem++)
					{
						if (CONTENTS* pItem = pPack->GetContent(nItem))
						{
							if (checkItem(pItem))
							{
								Count += GetItemCount(pItem);
							}

							// We should check for augs next
							if (pItem->Contents.ContainedItems.pItems && pItem->Contents.ContainedItems.Size)
							{
								for (size_t nAug = 0; nAug < pItem->Contents.ContainedItems.Size; nAug++)
								{
									if (CONTENTS* pAugItem = pItem->Contents.ContainedItems.pItems->Item[nAug])
									{
										ITEMINFO* pAugItemInfo = GetItemFromContents(pAugItem);

										if (pAugItemInfo->Type == ITEMTYPE_NORMAL && pAugItemInfo->AugType)
										{
											if (checkItem(pAugItem))
											{
												Count += 1;
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}

	// check mount keyring
	if (pChar->pMountsArray && pChar->pMountsArray->Mounts)
	{
		for (int nSlot = 0; nSlot < MAX_KEYRINGITEMS; nSlot++)
		{
			if (CONTENTS* pItem = pChar->pMountsArray->Mounts[nSlot])
			{
				if (checkItem(pItem))
				{
					Count += (GetItemFromContents(pItem)->StackSize > 1 ? pItem->StackCount : 1);
				}
			}
		}
	}

	// check illusions keyring
	if (pChar && pChar->pIllusionsArray && pChar->pIllusionsArray->Illusions)
	{
		for (int nSlot = 0; nSlot < MAX_KEYRINGITEMS; nSlot++)
		{
			if (CONTENTS* pItem = pChar->pIllusionsArray->Illusions[nSlot])
			{
				if (checkItem(pItem))
				{
					Count += (GetItemFromContents(pItem)->StackSize > 1 ? pItem->StackCount : 1);
				}
			}
		}
	}

	// check familiars keyring
	if (pChar && pChar->pFamiliarArray && pChar->pFamiliarArray->Familiars)
	{
		for (int nSlot = 0; nSlot < MAX_KEYRINGITEMS; nSlot++)
		{
			if (CONTENTS* pItem = pChar->pFamiliarArray->Familiars[nSlot])
			{
				if (checkItem(pItem))
				{
					Count += (GetItemFromContents(pItem)->StackSize > 1 ? pItem->StackCount : 1);
				}
			}
		}
	}

	return Count;
}

int FindItemCountByName(const char* pName, bool bExact)
{
	return CountItems([pName, bExact](CONTENTS* pItem)
		{ return ci_equals(GetItemFromContents(pItem)->Name, pName, bExact); });
}

int FindItemCountByID(int ItemID)
{
	return CountItems([ItemID](CONTENTS* pItem)
		{ return GetItemFromContents(pItem)->ItemNumber == ItemID; });
}

template <typename T>
CONTENTS* FindBankItem(T&& checkItem)
{
	CHARINFO* pCharInfo = GetCharInfo();
	if (!pCharInfo)
		return nullptr;

	auto checkAugs = [&](CONTENTS* pContents) -> CONTENTS *
	{
		if (pContents->Contents.ContainedItems.pItems && pContents->Contents.ContainedItems.Size)
		{
			for (size_t nAug = 0; nAug < pContents->Contents.ContainedItems.Size; nAug++)
			{
				if (CONTENTS* pAugItem = pContents->Contents.ContainedItems.pItems->Item[nAug])
				{
					ITEMINFO* pItem = GetItemFromContents(pAugItem);
					if (pItem->Type == ITEMTYPE_NORMAL && pItem->AugType)
					{
						if (checkItem(pAugItem))
							return pAugItem;
					}
				}
			}
		}

		return nullptr;
	};

	auto checkContainer = [&](CONTENTS* pPack) -> CONTENTS *
	{
		// check this item
		if (checkItem(pPack))
			return pPack;

		if (GetItemFromContents(pPack)->Type != ITEMTYPE_PACK)
		{
			// Hey it's not a pack we should check for augs
			if (CONTENTS* pAugItem = checkAugs(pPack))
				return pAugItem;
		}
		else if (pPack->Contents.ContainedItems.pItems)
		{
			// Ok it was a pack, if it has items in it lets check them
			for (int nItem = 0; nItem < GetItemFromContents(pPack)->Slots; nItem++)
			{
				if (CONTENTS* pItem = pPack->GetContent(nItem))
				{
					// check this item
					if (checkItem(pItem))
						return pItem;

					// Check for augs next
					if (CONTENTS* pAugItem = checkAugs(pItem))
						return pAugItem;
				}
			}
		}

		return nullptr;
	};

	// Check bank slots
	if (pCharInfo->pBankArray && pCharInfo->pBankArray->Bank)
	{
		for (int nPack = 0; nPack < NUM_BANK_SLOTS; nPack++)
		{
			CONTENTS* pPack = pCharInfo->pBankArray->Bank[nPack];
			if (!pPack)
				continue;

			if (CONTENTS* pItem = checkContainer(pPack))
				return pItem;
		}
	}

	// Check shared bank slots
	if (pCharInfo->pSharedBankArray)
	{
		for (int nPack = 0; nPack < NUM_SHAREDBANK_SLOTS; nPack++)
		{
			CONTENTS* pPack = pCharInfo->pSharedBankArray->SharedBank[nPack];
			if (!pPack)
				continue;

			if (CONTENTS* pItem = checkContainer(pPack))
				return pItem;
		}
	}

	return nullptr;
}

CONTENTS* FindBankItemByName(const char* pName, bool bExact)
{
	return FindBankItem([pName, bExact](CONTENTS* pItem)
		{ return ci_equals(GetItemFromContents(pItem)->Name, pName, bExact); });
}

CONTENTS* FindBankItemByID(int ItemID)
{
	return FindBankItem([ItemID](CONTENTS* pItem)
		{ return GetItemFromContents(pItem)->ItemNumber == ItemID; });
}

template <typename T>
int CountBankItems(T&& checkItem)
{
	CHARINFO* pCharInfo = GetCharInfo();
	if (!pCharInfo)
		return 0;

	int Count = 0;

	// Check bank slots
	if (pCharInfo && pCharInfo->pBankArray && pCharInfo->pBankArray->Bank)
	{
		for (int nPack = 0; nPack < NUM_BANK_SLOTS; nPack++)
		{
			if (CONTENTS* pPack = pCharInfo->pBankArray->Bank[nPack])
			{
				if (checkItem(pPack))
				{
					Count += GetItemCount(pPack);
				}

				if (GetItemFromContents(pPack)->Type != ITEMTYPE_PACK)
				{
					// check for augs
					if (pPack->Contents.ContainedItems.pItems && pPack->Contents.ContainedItems.Size)
					{
						for (size_t nAug = 0; nAug < pPack->Contents.ContainedItems.Size; nAug++)
						{
							if (CONTENTS* pAugItem = pPack->Contents.ContainedItems.pItems->Item[nAug])
							{
								if (GetItemFromContents(pAugItem)->Type == ITEMTYPE_NORMAL && GetItemFromContents(pAugItem)->AugType)
								{
									if (checkItem(pAugItem))
									{
										Count++;
									}
								}
							}
						}
					}
				}
				else if (pPack->Contents.ContainedItems.pItems)
				{
					// it was a pack, if it has items in it lets check them
					for (int nItem = 0; nItem < GetItemFromContents(pPack)->Slots; nItem++)
					{
						if (CONTENTS* pItem = pPack->GetContent(nItem))
						{
							if (checkItem(pItem))
							{
								Count += GetItemCount(pItem);
							}

							// Check for augs next
							if (pItem->Contents.ContainedItems.pItems && pItem->Contents.ContainedItems.Size)
							{
								for (size_t nAug = 0; nAug < pItem->Contents.ContainedItems.Size; nAug++)
								{
									if (CONTENTS* pAugItem = pItem->Contents.ContainedItems.pItems->Item[nAug])
									{
										if (GetItemFromContents(pAugItem)->Type == ITEMTYPE_NORMAL && GetItemFromContents(pAugItem)->AugType)
										{
											if (checkItem(pAugItem))
											{
												Count++;
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}

	// Check shared bank slots
	if (pCharInfo->pSharedBankArray)
	{
		for (int nPack = 0; nPack < NUM_SHAREDBANK_SLOTS; nPack++)
		{
			if (CONTENTS* pPack = pCharInfo->pSharedBankArray->SharedBank[nPack])
			{
				if (checkItem(pPack))
				{
					Count += GetItemCount(pPack);
				}

				if (GetItemFromContents(pPack)->Type != ITEMTYPE_PACK)
				{
					// check for augs
					if (pPack->Contents.ContainedItems.pItems && pPack->Contents.ContainedItems.Size)
					{
						for (size_t nAug = 0; nAug < pPack->Contents.ContainedItems.Size; nAug++)
						{
							if (CONTENTS* pAugItem = pPack->Contents.ContainedItems.pItems->Item[nAug])
							{
								if (GetItemFromContents(pAugItem)->Type == ITEMTYPE_NORMAL && GetItemFromContents(pAugItem)->AugType)
								{
									if (checkItem(pAugItem))
									{
										Count++;
									}
								}
							}
						}
					}
				}
				else if (pPack->Contents.ContainedItems.pItems)
				{
					// Ok it was a pack, if it has items in it lets check them
					for (int nItem = 0; nItem < GetItemFromContents(pPack)->Slots; nItem++)
					{
						if (CONTENTS* pItem = pPack->GetContent(nItem))
						{
							if (checkItem(pItem))
							{
								Count += GetItemCount(pItem);
							}

							// Check for augs next
							if (pItem->Contents.ContainedItems.pItems && pItem->Contents.ContainedItems.Size)
							{
								for (size_t nAug = 0; nAug < pItem->Contents.ContainedItems.Size; nAug++)
								{
									if (CONTENTS* pAugItem = pItem->Contents.ContainedItems.pItems->Item[nAug])
									{
										if (GetItemFromContents(pAugItem)->Type == ITEMTYPE_NORMAL && GetItemFromContents(pAugItem)->AugType)
										{
											if (checkItem(pItem))
											{
												Count++;
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}

	return Count;
}

int FindBankItemCountByName(const char* pName, bool bExact)
{
	return CountBankItems([pName, bExact](CONTENTS* pItem)
		{ return ci_equals(GetItemFromContents(pItem)->Name, pName, bExact); });
}

int FindBankItemCountByID(int ItemID)
{
	return CountBankItems([ItemID](CONTENTS* pItem)
		{ return GetItemFromContents(pItem)->ItemNumber == ItemID; });
}

CInvSlot* GetInvSlot2(const ItemGlobalIndex & index)
{
	return GetInvSlot(index.Location, index.GetIndex().GetSlot(0), index.GetIndex().GetSlot(1));
}

CInvSlot* GetInvSlot(DWORD type, short invslot, short bagslot)
{
	if (pInvSlotMgr)
	{
		for (int i = 0; i < pInvSlotMgr->TotalSlots; i++)
		{
			CInvSlot* pSlot = pInvSlotMgr->SlotArray[i];

			if (pSlot && pSlot->bEnabled && pSlot->pInvSlotWnd
				&& pSlot->pInvSlotWnd->ItemLocation.GetLocation() == type
				&& (short)pSlot->pInvSlotWnd->ItemLocation.GetIndex().GetSlot(0) == invslot
				&& (short)pSlot->pInvSlotWnd->ItemLocation.GetIndex().GetSlot(1) == bagslot)
			{
				if (CXMLData* pXMLData = pSlot->pInvSlotWnd->GetXMLData())
				{
					if (ci_equals(pXMLData->ScreenID, "HB_InvSlot"))
					{
						// we dont want this, the user specified a container, not a hotbutton...
						continue;
					}
				}

				return pSlot;
			}
		}
	}

	return nullptr;
}

//work in progress -eqmule
bool IsItemInsideContainer(CONTENTS* pItem)
{
	if (!pItem)
		return false;
	PcProfile* pChar2 = GetPcProfile();
	if (!pChar2)
		return false;

	// TODO: Just check if !IsBaseIndex()
	int index = pItem->GetGlobalIndex().GetTopSlot();

	if (index >= 0 && index <= NUM_INV_SLOTS)
	{
		if (pChar2 && pChar2->pInventoryArray)
		{
			if (CONTENTS* pItemFound = pChar2->pInventoryArray->InventoryArray[index])
			{
				if (pItemFound != pItem)
				{
					return true;
				}
			}
		}
	}

	return false;
}

bool OpenContainer(CONTENTS* pItem, bool hidden, bool flag)
{
	if (!pItem)
		return false;

	if (CONTENTS* pcont = FindItemBySlot2(pItem->GetGlobalIndex()))
	{
		if (pcont->Open)
			return true;

		if (GetItemFromContents(pcont)->Type == ITEMTYPE_PACK)
		{
			if (CInvSlot* pSlot = GetInvSlot2(pcont->GetGlobalIndex()))
			{
				if (hidden)
				{
					// put code to hide bag here
					// until i can figure out how to call moveitemqty
				}

				ItemGlobalIndex To = pSlot->pInvSlotWnd->ItemLocation;
				To.Location = pcont->GetGlobalIndex().Location; // eItemContainerPossessions;

				pContainerMgr->OpenContainer(&pcont, To, flag);
				//pPCData->AlertInventoryChanged();
				return pcont->Open;
			}
		}
	}
	return false;
}

bool CloseContainer(CONTENTS* pItem)
{
	if (!pItem)
		return false;

	if (CONTENTS* pcont = FindItemBySlot2(pItem->GetGlobalIndex()))
	{
		if (!pcont->Open)
			return false;

		if (GetItemFromContents(pcont)->Type == ITEMTYPE_PACK)
		{
			pContainerMgr->CloseContainer(&pcont, true);
			return !pcont->Open;
		}
	}

	return false;
}

//WaitForBagToOpen code by eqmule 2014
DWORD __stdcall WaitForBagToOpen(void* pData)
{
	PLARGE_INTEGER i64tmp = (PLARGE_INTEGER)pData;
	ItemContainerInstance type = (ItemContainerInstance)i64tmp->LowPart;
	CONTENTS* pItem = (CONTENTS*)i64tmp->HighPart;
	int timeout = 0;

	if (CONTENTS* pcont = FindItemBySlot2(pItem->GetGlobalIndex()))
	{
		if (pInvSlotMgr)
		{
			if (CInvSlot* theslot = pInvSlotMgr->FindInvSlot(pItem->GetGlobalIndex()))
			{
				if (theslot->pInvSlotWnd)
				{
					while (!theslot->pInvSlotWnd->IsVisible())
					{
						if (GetGameState() != GAMESTATE_INGAME)
							break;

						Sleep(10);
						timeout += 100;
						if (timeout >= 1000)
						{
							break;
						}
					}
				}
			}
		}

		//this is most likely completely useless
		//since the bag will actually always be open at this point
		//how can i check if the item is in the slot?
		//need to look into this further
		//get the texture maybe? -eqmule
		/*while(!pcont->Open) {
		Sleep(10);
		timeout+=100;
		if(timeout>=1000) {
		break;
		}
		}*/
	}

	Sleep(100);

	if (pWndMgr)
	{
		bool Old = pWndMgr->KeyboardFlags[1];
		pWndMgr->KeyboardFlags[1] = 1;

		if (ItemOnCursor())
		{
			DropItem(type, pItem->GetGlobalIndex().GetTopSlot(), pItem->GetGlobalIndex().GetIndex().GetSlot(1));
		}
		else
		{
			PickupItem(type, pItem);
		}

		pWndMgr->KeyboardFlags[1] = Old;
		LocalFree(pData);
		//CloseContainer(pItem);
	}
	return 1;
}

bool ItemOnCursor()
{
	PcProfile* pChar2 = GetPcProfile();
	if (pChar2 && pChar2->pInventoryArray && pChar2->pInventoryArray->Inventory.Cursor)
	{
		return true;
	}
	return false;
}

bool PickupItem(ItemContainerInstance type, CONTENTS* pItem)
{
	if (!pItem || !pInvSlotMgr)
	{
		return false;
	}

	bool bSelectSlot = false;

	if (pMerchantWnd && pMerchantWnd->IsVisible()) {
		// if the merchant window is open, we dont actually drop anything we just select the slot
		bSelectSlot = true;
	}

	if (pItem->GetGlobalIndex().GetIndex().IsBase())
	{
		// ok so they want to pick it up from a toplevelslot
		CInvSlot* pSlot = GetInvSlot(type, pItem->GetGlobalIndex().GetTopSlot());
		if (!pSlot || !pSlot->pInvSlotWnd)
		{
			// if we got all the way here this really shouldnt happen... but why assume...
			WriteChatf("Could not find the %d itemslot", pItem->GetGlobalIndex().GetTopSlot());
			return false;
		}

		if (bSelectSlot)
		{
			if (CInvSlot* theslot = pInvSlotMgr->FindInvSlot(pItem->GetGlobalIndex().GetTopSlot()))
			{
				pInvSlotMgr->SelectSlot(theslot);

				ItemGlobalIndex To;
				To.Location = eItemContainerPossessions;
				To.Index.SetSlot(0, pItem->GetGlobalIndex().GetTopSlot());
				To.Index.SetSlot(1, pItem->GetGlobalIndex().GetIndex().GetSlot(1));

				pMerchantWnd->SelectBuySellSlot(To);
				return true;
			}
		}
		else
		{
			// just move it from the slot to the cursor
			ItemGlobalIndex From;
			From.Location = (ItemContainerInstance)pItem->GetGlobalIndex().GetLocation();
			From.Index.SetSlot(0, pItem->GetGlobalIndex().GetTopSlot());

			ItemGlobalIndex To;
			To.Location = eItemContainerPossessions;
			To.Index.SetSlot(0, eItemContainerCursor);

			pInvSlotMgr->MoveItem(From, To, true, true);
			return true;
		}
	}
	else
	{
		// BagSlot is NOT -1 so they want to pick it up from INSIDE a bag
		if (bSelectSlot)
		{
			if (CInvSlot* theslot = pInvSlotMgr->FindInvSlot(pItem->GetGlobalIndex().GetTopSlot(), pItem->GetGlobalIndex().GetIndex().GetSlot(1)))
			{
				pInvSlotMgr->SelectSlot(theslot);
				ItemGlobalIndex To;
				To.Location = eItemContainerPossessions;
				To.Index.SetSlot(0, pItem->GetGlobalIndex().GetIndex().GetSlot(0));
				To.Index.SetSlot(1, pItem->GetGlobalIndex().GetIndex().GetSlot(1));

				pMerchantWnd->SelectBuySellSlot(To);
				return true;
			}
			else
			{
				// well now is where it gets complicated then... or not...
				ItemGlobalIndex To;
				To.Location = eItemContainerPossessions;
				To.Index.SetSlot(0, pItem->GetGlobalIndex().GetIndex().GetSlot(0));
				To.Index.SetSlot(1, pItem->GetGlobalIndex().GetIndex().GetSlot(1));

				pMerchantWnd->SelectBuySellSlot(To);
				return true;
			}
		}
		else
		{
			// not a selected slot
			// ok so its a slot inside a bag
			// is ctrl pressed?
			// if it is we HAVE to open the bag, until I get a bypass worked out

			uint32_t keybflag = pWndMgr->GetKeyboardFlags();

			if (keybflag == 2 && pItem->StackCount > 1)
			{
				// ctrl was pressed and it is a stackable item
				// I need to open the bag and notify it cause moveitem only picks up full stacks
				CInvSlot* pSlot = GetInvSlot2(pItem->GetGlobalIndex());
				if (!pSlot)
				{
					// well lets try to open it then
					if (CONTENTS* pBag = FindItemBySlot2(pItem->GetGlobalIndex().GetParent()))
					{
						bool wechangedpackopenstatus = OpenContainer(pBag, true);
						if (wechangedpackopenstatus)
						{
							if (PLARGE_INTEGER i64tmp = (PLARGE_INTEGER)LocalAlloc(LPTR, sizeof(LARGE_INTEGER)))
							{
								i64tmp->LowPart = type;
								i64tmp->HighPart = (int)pItem;
								DWORD nThreadId = 0;
								CreateThread(nullptr, 0, WaitForBagToOpen, i64tmp, 0, &nThreadId);
								return false;
							}
						}
					}
					else
					{
						WriteChatf("[PickupItem] falied due to no bag found in slot %d", pItem->GetGlobalIndex().GetTopSlot());
						return false;
					}
				}
				else
				{
					// ok so the bag is open...
					// well we just select it then...
					if (!pSlot->pInvSlotWnd || !SendWndClick2(pSlot->pInvSlotWnd, "leftmouseup"))
					{
						WriteChatf("Could not pickup %s", GetItemFromContents(pItem)->Name);
					}
					return true;
				}

				// thread this? hmm if i close it before item ends up on cursor, it wont...
				// if(wechangedpackopenstatus)
				//     CloseContainer(pItem);
				return false;
			}
			else
			{
				// ctrl is NOT pressed
				// we can just move the whole stack
				ItemGlobalIndex From;
				From.Location = (ItemContainerInstance)pItem->GetGlobalIndex().Location;
				From.Index.SetSlot(0, pItem->GetGlobalIndex().GetTopSlot());
				From.Index.SetSlot(1, pItem->GetGlobalIndex().GetIndex().GetSlot(1));

				ItemGlobalIndex To;
				To.Location = eItemContainerPossessions;
				To.Index.SetSlot(0, eItemContainerCursor); // this is probably wrong

				PcProfile* pChar2 = GetPcProfile();
				CONTENTS* pContBefore = pChar2->pInventoryArray->Inventory.Cursor;
				pInvSlotMgr->MoveItem(From, To, true, true, false, true);

				if (pChar2 && pChar2->pInventoryArray && pChar2->pInventoryArray->Inventory.Cursor)
				{
					CONTENTS* pContAfter = pChar2->pInventoryArray->Inventory.Cursor;

					EqItemGuid g;
					strcpy_s(g.guid, 18, "0000000000000000");

					pCursorAttachment->AttachToCursor(nullptr, nullptr, eCursorAttachment_Item, -1, g, 0, nullptr, nullptr);
				}
				else
				{
					pCursorAttachment->Deactivate();
				}
			}
			return true;
		}
	}
	return false;
}

bool DropItem2(const ItemGlobalIndex & index)
{
	return DropItem(index.GetLocation(), index.GetTopSlot(), index.GetIndex().GetSlot(1));
}

bool DropItem(ItemContainerInstance type, short ToInvSlot, short ToBagSlot)
{
	if (!pInvSlotMgr)
		return false;

	bool bSelectSlot = false;
	if (pMerchantWnd && pMerchantWnd->IsVisible())
	{
		// if the merchant window is open, we dont actually drop anything we just select the slot
		bSelectSlot = true;
	}

	if (ToBagSlot == -1)
	{
		// they want to drop it to a toplevelslot
		CInvSlot* pSlot = GetInvSlot(type, ToInvSlot);
		if (!pSlot || !pSlot->pInvSlotWnd)
		{
			// if we got all the way here this really shouldnt happen... but why assume...
			WriteChatf("Could not find the %d itemslot", ToInvSlot);
			return false;
		}

		if (bSelectSlot)
		{
			if (CInvSlot* theSlot = pInvSlotMgr->FindInvSlot(ToInvSlot))
			{
				// we select the slot, and that will set pSelectedItem correctly
				// we do this cause later on we need that address for the .Selection member
				pInvSlotMgr->SelectSlot(theSlot);

				ItemGlobalIndex To = theSlot->pInvSlotWnd->ItemLocation;
				To.Location = eItemContainerPossessions;

				pMerchantWnd->SelectBuySellSlot(To);
				return true;
			}
		}
		else
		{
			// just move it from cursor to the slot
			ItemGlobalIndex From;
			From.Location = eItemContainerPossessions;
			From.Index.SetSlot(0, eItemContainerCursor);   // TODO: Check this, i'm pretty sure its wrong.

			ItemGlobalIndex To;
			To.Location = type;
			To.Index.SetSlot(0, ToInvSlot);
			To.Index.SetSlot(1, ToBagSlot);

			pInvSlotMgr->MoveItem(From, To, true, true);
			return true;
		}
	}
	else
	{
		// BagSlot is NOT -1 so they want to drop it INSIDE a bag
		if (bSelectSlot)
		{
			if (CInvSlot* theSlot = pInvSlotMgr->FindInvSlot(ToInvSlot, ToBagSlot))
			{
				pInvSlotMgr->SelectSlot(theSlot);

				ItemGlobalIndex To;
				To.Location = eItemContainerPossessions;
				To.Index.SetSlot(0, theSlot->pInvSlotWnd->ItemLocation.GetTopSlot());
				To.Index.SetSlot(1, theSlot->pInvSlotWnd->ItemLocation.GetIndex().GetSlot(1));

				pMerchantWnd->SelectBuySellSlot(To);
				return true;

			}
			else
			{
				// well now is where it gets complicated then...
				// so we need to open the bag...

				ItemGlobalIndex To;
				To.Location = eItemContainerPossessions;
				To.Index.SetSlot(0, ToInvSlot);
				To.Index.SetSlot(1, ToBagSlot);

				pMerchantWnd->SelectBuySellSlot(To);
				return true;
			}
		}
		else
		{
			// ok so its a slot inside a bag
			ItemGlobalIndex From;
			From.Location = eItemContainerPossessions;
			From.Index.SetSlot(0, eItemContainerCursor); // TODO: Check this, i'm pretty sure its wrong.

			ItemGlobalIndex To;
			To.Location = type;
			To.Index.SetSlot(0, ToInvSlot);
			To.Index.SetSlot(1, ToBagSlot);

			PcProfile* pChar2 = GetPcProfile();
			CONTENTS* pContBefore = pChar2->pInventoryArray->Inventory.Cursor;

			pInvSlotMgr->MoveItem(From, To, true, true, true, false);

			if (pChar2 && pChar2->pInventoryArray && pChar2->pInventoryArray->Inventory.Cursor)
			{
				CONTENTS* pContAfter = pChar2->pInventoryArray->Inventory.Cursor;

				EqItemGuid g;
				strcpy_s(g.guid, 18, "0000000000000000");
				CCursorAttachment* pCursAtch = pCursorAttachment;

				pCursAtch->AttachToCursor(nullptr, nullptr, eCursorAttachment_Item, -1, g, 0, nullptr, nullptr);
			}
			else
			{
				pCursorAttachment->Deactivate();
			}
		}
		return true;
	}
	return false;
}

int GetTargetBuffByCategory(int category, unsigned int classmask, int startslot)
{
	if (pTargetWnd->Type <= 0)
		return false;

	int buffID = 0;
	for (int i = startslot; i < NUM_BUFF_SLOTS; i++)
	{
		buffID = pTargetWnd->BuffSpellID[i];
		if (buffID > 0)
		{
			if (SPELL* pSpell = GetSpellByID(buffID))
			{
				if (GetSpellCategory(pSpell) == category && IsSpellUsableForClass(pSpell, classmask))
				{
					return i;
				}
			}
		}
	}
	return -1;
}

int GetTargetBuffBySubCat(const char* subcat, unsigned int classmask, int startslot)
{
	if (pTargetWnd->Type <= 0)
		return -1;

	for (int i = startslot; i < NUM_BUFF_SLOTS; i++)
	{
		int buffID = pTargetWnd->BuffSpellID[i];
		if (buffID > 0)
		{
			SPELL* pSpell = GetSpellByID(buffID);
			if (!pSpell) continue;

			int cat = GetSpellSubcategory(pSpell);
			if (!cat) continue;

			const char* ptr = pCDBStr->GetString(cat, eSpellCategory);
			if (!ptr) continue;

			if (!_stricmp(ptr, subcat))
			{
				if (classmask == Unknown)
				{
					return i;
				}

				for (int N = 0; N < TotalPlayerClasses; N++)
				{
					if (classmask & (1 << N))
					{
						return i;
					}
				}
			}
		}
	}

	return -1;
}

bool HasCachedTargetBuffSubCat(const char* subcat, SPAWNINFO* pSpawn, TargetBuff* pcTargetBuff, unsigned int classmask)
{
	if (CachedBuffsMap.empty())
		return false;

	auto i = CachedBuffsMap.find(pSpawn->SpawnID);
	if (i == CachedBuffsMap.end())
		return false;

	for (auto& iter : i->second)
	{
		int buffID = iter.first;
		if (SPELL* pSpell = GetSpellByID(buffID))
		{
			if (int cat = GetSpellSubcategory(pSpell))
			{
				if (const char* ptr = pCDBStr->GetString(cat, eSpellCategory))
				{
					if (!_stricmp(ptr, subcat))
					{
						if (classmask == Unknown)
							return true;

						for (int N = 0; N < 16; N++)
						{
							if (classmask & (1 << N))
							{
								return true;
							}
						}
					}
				}
			}
		}
	}

	return false;
}

bool HasCachedTargetBuffSPA(int spa, bool bIncrease, SPAWNINFO* pSpawn, TargetBuff* pcTargetBuff)
{
	auto i = CachedBuffsMap.find(pSpawn->SpawnID);
	if (i == CachedBuffsMap.end())
		return false;

	for (auto& iter : i->second)
	{
		int buffID = iter.first;

		if (SPELL* pSpell = GetSpellByID(buffID))
		{
			if (int base = ((EQ_Spell*)pSpell)->SpellAffectBase(spa))
			{
				strcpy_s(pcTargetBuff->casterName, iter.second.casterName);
				pcTargetBuff->count = iter.second.count;
				pcTargetBuff->duration = iter.second.duration;
				pcTargetBuff->slot = iter.second.slot;
				pcTargetBuff->spellId = iter.second.spellId;
				pcTargetBuff->timeStamp = iter.second.timeStamp;

				switch (spa)
				{
				case 3: // Movement Rate
					if (!bIncrease && base < 0)
					{
						// below 0 means its a snare above its runspeed increase...
						return true;
					}
					else if (bIncrease && base > 0)
					{
						return true;
					}
					return false;

				case 11: // Melee Speed
					if (!bIncrease && base < 100)
					{
						// below 100 means its a slow above its haste...
						return true;
					}
					else if (bIncrease && base > 100)
					{
						return true;
					}
					return false;

				case 59: // Damage Shield
					if (!bIncrease && base > 0)
					{
						// decreased DS
						return true;
					}
					else if (bIncrease && base < 0)
					{
						// increased DS
						return true;
					}
					return false;

				case 121: // Reverse Damage Shield
					if (!bIncrease && base > 0)
					{
						// decreased DS
						return true;
					}
					else if (bIncrease && base < 0)
					{
						// increased DS
						return true;
					}
					return false;

				default:
					return true;
				}
			}
		}
	}

	return false;
}

//Usage: The spa is the spellaffect id, for example 11 for Melee Speed
//       the bIncrease tells the function if we want spells that increase or decrease the SPA
int GetTargetBuffBySPA(int spa, bool bIncrease, int startslot)
{
	if (pTargetWnd->Type <= 0)
		return false;

	int buffID = 0;
	for (int i = startslot; i < NUM_BUFF_SLOTS; i++)
	{
		buffID = pTargetWnd->BuffSpellID[i];
		if (buffID > 0 && buffID != -1)
		{
			if (SPELL* pSpell = GetSpellByID(buffID))
			{
				if (int base = ((EQ_Spell*)pSpell)->SpellAffectBase(spa))
				{
					switch (spa)
					{
					case 3: // Movement Rate
						if (!bIncrease && base < 0)
						{
							// below 0 means its a snare above its runspeed increase...
							return i;
						}
						else if (bIncrease && base > 0)
						{
							return i;
						}
						return -1;

					case 11: // Melee Speed
						if (!bIncrease && base < 100)
						{
							// below 100 means its a slow above its haste...
							return i;
						}
						else if (bIncrease && base > 100)
						{
							return i;
						}
						return -1;

					case 59: // Damage Shield
						if (!bIncrease && base > 0)
						{
							// decreased DS
							return i;
						}
						else if (bIncrease && base < 0)
						{
							// increased DS
							return i;
						}
						return -1;

					case 121: // Reverse Damage Shield
						if (!bIncrease && base > 0)
						{
							// decreased DS
							return i;
						}
						else if (bIncrease && base < 0)
						{
							// increased DS
							return i;
						}
						return -1;

					default:
						return i;
					}
				}
			}
		}
	}
	return -1;
}

int GetSelfBuffByCategory(int category, unsigned int classmask, int startslot)
{
	PcProfile* pProfile = GetPcProfile();
	if (!pProfile)
		return -1;

	for (int i = startslot; i < NUM_BUFF_SLOTS; i++)
	{
		if (SPELL* pSpell = GetSpellByID(pProfile->Buff[i].SpellID))
		{
			if (GetSpellCategory(pSpell) == category && IsSpellUsableForClass(pSpell, classmask))
			{
				return i;
			}
		}
	}

	return -1;
}

int GetSelfBuffBySubCat(const char* subcat, unsigned int classmask, int startslot)
{
	PcProfile* pProfile = GetPcProfile();
	if (!pProfile)
		return -1;

	for (int i = startslot; i < NUM_LONG_BUFFS; i++)
	{
		if (SPELL* pSpell = GetSpellByID(pProfile->Buff[i].SpellID))
		{
			if (DWORD cat = GetSpellSubcategory(pSpell))
			{
				if (const char* ptr = pCDBStr->GetString(cat, eSpellCategory))
				{
					if (!_stricmp(ptr, subcat) && IsSpellUsableForClass(pSpell, classmask))
					{
						return i;
					}
				}
			}
		}
	}

	return -1;
}

int GetSelfBuffBySPA(int spa, bool bIncrease, int startslot)
{
	PcProfile* pProfile = GetPcProfile();
	if (!pProfile)
		return -1;

	for (int i = startslot; i < NUM_LONG_BUFFS; i++)
	{
		if (SPELL* pSpell = GetSpellByID(pProfile->Buff[i].SpellID))
		{
			if (int base = ((EQ_Spell*)pSpell)->SpellAffectBase(spa))
			{
				switch (spa)
				{
				case 3: // Movement Rate
					if (!bIncrease && base < 0)
					{
						// below 0 means its a snare above its runspeed increase...
						return i;
					}
					else if (bIncrease && base > 0)
					{
						return i;
					}
					return -1;

				case 11: // Melee Speed
					if (!bIncrease && base < 100)
					{
						// below 100 means its a slow above its haste...
						return i;
					}
					else if (bIncrease && base > 100)
					{
						return i;
					}
					return -1;

				case 59: // Damage Shield
					if (!bIncrease && base > 0)
					{
						// decreased DS
						return i;
					}
					else if (bIncrease && base < 0)
					{
						// increased DS
						return i;
					}
					return -1;

				case 121: // Reverse Damage Shield
					if (!bIncrease && base > 0)
					{
						// decreased DS
						return i;
					}
					else if (bIncrease && base < 0)
					{
						// increased DS
						return i;
					}
					return -1;

				default:
					return i;
				}
			}
		}
	}

	return -1;
}

int GetSelfShortBuffBySPA(int spa, bool bIncrease, int startslot)
{
	PcProfile* pProfile = GetPcProfile();
	if (!pProfile)
		return -1;

	for (int i = startslot; i < NUM_SHORT_BUFFS; i++)
	{
		if (SPELL* pSpell = GetSpellByID(pProfile->ShortBuff[i].SpellID))
		{
			if (int base = ((EQ_Spell*)pSpell)->SpellAffectBase(spa))
			{
				switch (spa)
				{
				case 3: // Movement Rate
					if (!bIncrease && base < 0)
					{
						// below 0 means its a snare above its runspeed increase...
						return i;
					}
					else if (bIncrease && base > 0)
					{
						return i;
					}
					return -1;

				case 11: // Melee Speed
					if (!bIncrease && base < 100)
					{
						// below 100 means its a slow above its haste...
						return i;
					}
					else if (bIncrease && base > 100)
					{
						return i;
					}
					return -1;

				case 59: // Damage Shield
					if (!bIncrease && base > 0)
					{
						// decreased DS
						return i;
					}
					else if (bIncrease && base < 0)
					{
						// increased DS
						return i;
					}
					return -1;

				case 121: // Reverse Damage Shield
					if (!bIncrease && base > 0)
					{
						// decreased DS
						return i;
					}
					else if (bIncrease && base < 0)
					{
						// increased DS
						return i;
					}
					return -1;

				default:
					return i;
				}
			}
		}
	}

	return -1;
}

int GetSpellCategory(SPELL* pSpell)
{
	if (pSpell)
	{
		if (pSpell->CannotBeScribed)
		{
			if (SPELL* pTrigger = GetSpellParent(pSpell->ID))
			{
				return pTrigger->Category;
			}
		}
		else
		{
			return pSpell->Category;
		}
	}

	return 0;
}

int GetSpellSubcategory(SPELL* pSpell)
{
	if (pSpell)
	{
		if (pSpell->CannotBeScribed)
		{
			if (SPELL* pTrigger = GetSpellParent(pSpell->ID))
			{
				return pTrigger->Subcategory;
			}
		}
		else
		{
			return pSpell->Subcategory;
		}
	}

	return 0;
}

bool IsAegoSpell(SPELL* pSpell)
{
	if (pSpell->CannotBeScribed)
	{
		if (SPELL* pTrigger = GetSpellParent(pSpell->ID))
		{
			if ((pTrigger->Subcategory == 1) || (pTrigger->Subcategory == 112))
			{
				int base = ((EQ_Spell*)pSpell)->SpellAffectBase(1); // check if it has ac?
				if (base)
				{
					return true;
				}
			}
		}
	}
	else
	{
		if ((pSpell->Subcategory == 1) || (pSpell->Subcategory == 112))
		{
			int base = ((EQ_Spell*)pSpell)->SpellAffectBase(1);
			if (base)
			{
				return true;
			}
		}
	}

	return false;
}

bool IsSpellUsableForClass(SPELL* pSpell, unsigned int classmask)
{
	if (classmask != Unknown)
	{
		for (int N = 0; N < 16; N++)
		{
			if (classmask & (1 << N))
			{
				if (pSpell->ClassLevel[N] != 255)
					return true;
			}
		}
		return false;
	}
	return true;
}

int GetSpellRankByName(const char* SpellName)
{
	// uppercase the string
	char szTemp[256];
	strcpy_s(szTemp, SpellName);
	_strupr_s(szTemp);

	if (endsWith(szTemp, " II"))
		return 2;
	if (endsWith(szTemp, " III"))
		return 3;
	if (endsWith(szTemp, " IV"))
		return 4;
	if (endsWith(szTemp, " V"))
		return 5;
	if (endsWith(szTemp, " VI"))
		return 6;
	if (endsWith(szTemp, " VII"))
		return 7;
	if (endsWith(szTemp, " VIII"))
		return 8;
	if (endsWith(szTemp, " IX"))
		return 9;
	if (endsWith(szTemp, " X"))
		return 10;
	if (endsWith(szTemp, " XI"))
		return 11;
	if (endsWith(szTemp, " XII"))
		return 12;
	if (endsWith(szTemp, " XIII"))
		return 13;
	if (endsWith(szTemp, " XIV"))
		return 14;
	if (endsWith(szTemp, " XV"))
		return 15;
	if (endsWith(szTemp, " XVI"))
		return 16;
	if (endsWith(szTemp, " XVII"))
		return 17;
	if (endsWith(szTemp, " XVIII"))
		return 18;
	if (endsWith(szTemp, " XIX"))
		return 19;
	if (endsWith(szTemp, " XX"))
		return 20;
	if (endsWith(szTemp, " XXI"))
		return 21;
	if (endsWith(szTemp, " XXII"))
		return 22;
	if (endsWith(szTemp, " XXIII"))
		return 23;
	if (endsWith(szTemp, " XXIV"))
		return 24;
	if (endsWith(szTemp, " XXV"))
		return 25;
	if (endsWith(szTemp, " XXVI"))
		return 26;
	if (endsWith(szTemp, " XXVII"))
		return 27;
	if (endsWith(szTemp, " XXVIII"))
		return 28;
	if (endsWith(szTemp, " XXIX"))
		return 29;
	if (endsWith(szTemp, " XXX"))
		return 30;

	if (endsWith(szTemp, ".II"))
		return 2;
	if (endsWith(szTemp, ".III"))
		return 3;

	return 0;
}

void TruncateSpellRankName(char* SpellName)
{
	if (char* pch = strrchr(SpellName, '.'))
	{
		pch -= 3;
		*pch = 0;
	}
}

void RemoveBuff(SPAWNINFO* pChar, char* szLine)
{
	bool bPet = false;
	bool bAll = false;
	char szCmd[MAX_STRING] = { 0 };
	GetArg(szCmd, szLine, 1);

	if (!_stricmp(szCmd, "-pet"))
	{
		bPet = true;
		GetArg(szCmd, szLine, 2);
	}
	else if (!_stricmp(szCmd, "-both"))
	{
		bAll = true;
		GetArg(szCmd, szLine, 2);
	}

	if (szCmd && szCmd[0] != '\0')
	{
		if (bPet || bAll)
		{
			if (pPetInfoWnd && szLine && szLine[0] != 0)
			{
				for (int nBuff = 0; nBuff < NUM_BUFF_SLOTS; nBuff++)
				{
					if (SPELL* pBuffSpell = GetSpellByID(pPetInfoWnd->Buff[nBuff]))
					{
						if (!_strnicmp(pBuffSpell->Name, szCmd, strlen(szCmd)))
						{
							((PcZoneClient*)pPCData)->RemovePetEffect(nBuff);
							break;
						}
					}
				}
			}
			if (bPet) return;
		}

		if (PcProfile* pProfile = GetPcProfile())
		{
			for (int nBuff = 0; nBuff < NUM_LONG_BUFFS; nBuff++)
			{
				if (pProfile->Buff[nBuff].SpellID == 0 || pProfile->Buff[nBuff].SpellID == -1)
					continue;

				if (SPELL* pBuffSpell = GetSpellByID(pProfile->Buff[nBuff].SpellID))
				{
					if (!_strnicmp(pBuffSpell->Name, szCmd, strlen(szCmd)))
					{
						((PcZoneClient*)pPCData)->RemoveBuffEffect(nBuff, ((SPAWNINFO*)pLocalPlayer)->SpawnID);
						return;
					}
				}
			}

			for (int nBuff = 0; nBuff < NUM_SHORT_BUFFS; nBuff++)
			{
				if (pProfile->ShortBuff[nBuff].SpellID == 0 || pProfile->ShortBuff[nBuff].SpellID == -1)
					continue;
				if (SPELL* pBuffSpell = GetSpellByID(pProfile->ShortBuff[nBuff].SpellID))
				{
					if (!_strnicmp(pBuffSpell->Name, szCmd, strlen(szCmd)))
					{
						((PcZoneClient*)pPCData)->RemoveBuffEffect(nBuff + NUM_LONG_BUFFS, ((SPAWNINFO*)pLocalPlayer)->SpawnID);
						//pPCData->RemoveMyAffect(nBuff + NUM_LONG_BUFFS);
						return;
					}
				}
			}
		}
	}
}

void RemovePetBuff(SPAWNINFO* pChar, char* szLine)
{
	if (pPetInfoWnd && szLine && szLine[0] != '\0')
	{
		for (int nBuff = 0; nBuff < NUM_BUFF_SLOTS; nBuff++)
		{
			if (SPELL* pBuffSpell = GetSpellByID(pPetInfoWnd->Buff[nBuff]))
			{
				if (!_strnicmp(pBuffSpell->Name, szLine, strlen(szLine)))
				{
					((PcZoneClient*)pPCData)->RemovePetEffect(nBuff);
					return;
				}
			}
		}
	}
}

bool StripQuotes(char* str)
{
	bool bRet = false;
	if (strchr(str, '"'))
		bRet = true;
	char* s,* d;
	for (s = d = str; *d = *s; d += (*s++ != '"'));
	return bRet;
}

DWORD __stdcall RefreshKeyRingThread(void* pData)
{
	RefreshKeyRingsThreadData* kr = (RefreshKeyRingsThreadData*)pData;
	if (!kr) return 0;

	CSidlScreenWnd* krwnd = kr->phWnd;
	bool bExact = kr->bExact;
	bool bUseCmd = kr->bUseCmd;
	bool bToggled = false;
	char szItemName[256] = { 0 };
	strcpy_s(szItemName, kr->ItemName);
	delete kr;

	if (!krwnd) return 0;

	CTabWnd* pTab = (CTabWnd*)krwnd->GetChildItem(KeyRingTab);
	if (!pTab) return 0;

	if (!krwnd->IsVisible())
	{
		bToggled = true;
		krwnd->Activate();
		krwnd->StoreIniVis();
	}

	bool bRefresh = false;
	CListWnd* clist = nullptr;

	if (GetMountCount() > 0)
	{
		// tab 0 is the mount key ring page...
		pTab->SetPage(0, true);

		if (clist = (CListWnd*)krwnd->GetChildItem(MountWindowList))
		{
			ULONGLONG now = MQGetTickCount64();
			while (!clist->ItemsArray.Count)
			{
				Sleep(10);
				if (now + 5000 < MQGetTickCount64())
				{
					WriteChatColor("Timed out waiting for mount keyring refresh", CONCOLOR_YELLOW);
					break;
				}
			}
		}
	}

	if (GetIllusionCount() > 0)
	{
		// tab 1 is the illusion key ring page...
		pTab->SetPage(1, true);

		if (clist = (CListWnd*)krwnd->GetChildItem(IllusionWindowList))
		{
			ULONGLONG now = MQGetTickCount64();
			while (!clist->ItemsArray.Count)
			{
				Sleep(10);
				if (now + 5000 < MQGetTickCount64())
				{
					WriteChatColor("Timed out waiting for illusion keyring refresh", CONCOLOR_YELLOW);
					break;
				}
			}
		}
	}

	// TODO: De-duplicate GetFamiliarCount/GetMountCount/etc
	if (GetFamiliarCount() > 0)
	{
		//tab 2 is the familiar key ring page...
		pTab->SetPage(2, true);

		if (clist = (CListWnd*)krwnd->GetChildItem(FamiliarWindowList))
		{
			ULONGLONG now = MQGetTickCount64();
			while (!clist->ItemsArray.Count)
			{
				Sleep(10);
				if (now + 5000 < MQGetTickCount64())
				{
					WriteChatColor("Timed out waiting for familiar keyring refresh", CONCOLOR_YELLOW);
					break;
				}
			}
		}
	}

	WeDidStuff();

	if (bToggled)
	{
		krwnd->Deactivate();
		krwnd->StoreIniVis();
	}

	if (bUseCmd && clist && clist->ItemsArray.Count)
	{
		UseItemCmd(GetCharInfo()->pSpawn, szItemName);
	}

	return 0;
}

void RefreshKeyRings(void* kr)
{
	CreateThread(nullptr, 0, RefreshKeyRingThread, kr, 0, nullptr);
}

int GetMountCount()
{
	int Count = 0;
	if (CHARINFO* pChar = GetCharInfo())
	{
#ifdef NEWCHARINFO
		return pChar->MountKeyRingItems.Items.Size;
#else
		if (pChar && pChar->pMountsArray && pChar->pMountsArray->Mounts)
		{
			for (int nSlot = 0; nSlot < MAX_KEYRINGITEMS; nSlot++)
			{
				if (CONTENTS* pItem = pChar->pMountsArray->Mounts[nSlot])
				{
					Count++;
				}
			}
		}
#endif
	}
	return Count;
}

int GetIllusionCount()
{
	int Count = 0;
	if (CHARINFO* pChar = GetCharInfo())
	{
#ifdef NEWCHARINFO
		return pChar->IllusionKeyRingItems.Items.Size;
#else
		if (pChar && pChar->pIllusionsArray && pChar->pIllusionsArray->Illusions)
		{
			for (int nSlot = 0; nSlot < MAX_KEYRINGITEMS; nSlot++)
			{
				if (CONTENTS* pItem = pChar->pIllusionsArray->Illusions[nSlot])
				{
					Count++;
				}
			}
		}
#endif
	}
	return Count;
}

int GetFamiliarCount()
{
	int Count = 0;
	if (CHARINFO* pChar = GetCharInfo())
	{
#ifdef NEWCHARINFO
		return pChar->FamiliarKeyRingItems.Items.Size;
#else
		if (pChar && pChar->pFamiliarArray && pChar->pFamiliarArray->Familiars)
		{
			for (int nSlot = 0; nSlot < MAX_KEYRINGITEMS; nSlot++)
			{
				if (CONTENTS* pItem = pChar->pFamiliarArray->Familiars[nSlot])

				{
					Count++;
				}
			}
		}
#endif
	}
	return Count;
}

int GetKeyRingIndex(KeyRingType KeyRing, const char* szItemName, bool bExact, bool usecmd)
{
	int index = 0;

	if (CSidlScreenWnd* krWnd = (CSidlScreenWnd*)FindMQ2Window(KeyRingWindowParent))
	{
		CListWnd* clist = nullptr;

		if (KeyRing == eFamiliar)
			clist = (CListWnd*)krWnd->GetChildItem(FamiliarWindowList);
		else if (KeyRing == eIllusion)
			clist = (CListWnd*)krWnd->GetChildItem(IllusionWindowList);
		else if (KeyRing == eMount)
			clist = (CListWnd*)krWnd->GetChildItem(MountWindowList);

		if (clist)
		{
			if (int numitems = clist->ItemsArray.GetCount())
			{
				for (int i = 0; i < numitems; i++)
				{
					CXStr Str = clist->GetItemText(i, 2);

					if (!Str.empty())
					{
						if (bExact)
						{
							if (!_stricmp(szItemName, Str.c_str()))
							{
								index = i + 1;
								break;
							}
						}
						else
						{
							if (ci_find_substr(Str, szItemName) != -1)
							{
								index = i + 1;
								break;
							}
						}
					}
				}
			}
			else
			{
				if (CONTENTS* pCont = FindItemByName(szItemName, bExact))
				{
					bool bKeyring = false;
					if (CHARINFO* pCharInfo = GetCharInfo())
					{
						bKeyring = pCont->GetGlobalIndex().IsKeyRingLocation();
					}

					if (bKeyring)
					{
						// if the keyring lists has 0 items in it, we arrive here...
						// its not filled in until you open the mount keyring tab in the inventory window...
						// since numitems was 0, we know the user hasnt opened up his inventory
						// and been on the mount key ring tab...so we start a thread and force that... -eqmule

						RefreshKeyRingsThreadData* kr = new RefreshKeyRingsThreadData;
						kr->bExact = bExact;
						kr->phWnd = krWnd;
						kr->bUseCmd = usecmd;
						strcpy_s(kr->ItemName, szItemName);
						RefreshKeyRings(kr);
					}
				}
			}
		}
	}

	return index;
}

void InitKeyRings()
{
	if (CSidlScreenWnd* krwnd = (CSidlScreenWnd*)FindMQ2Window(KeyRingWindowParent))
	{
		CListWnd* clist = nullptr;
		bool bRefresh = false;

		if (GetMountCount() > 0)
		{
			if (clist = (CListWnd*)krwnd->GetChildItem(MountWindowList))
			{
				if (!clist->ItemsArray.Count)
				{
					bRefresh = true;
				}
			}
		}

		if (GetIllusionCount() > 0)
		{
			if (clist = (CListWnd*)krwnd->GetChildItem(IllusionWindowList))
			{
				if (!clist->ItemsArray.Count)
				{
					bRefresh = true;
				}
			}
		}

		if (GetFamiliarCount() > 0)
		{
			if (clist = (CListWnd*)krwnd->GetChildItem(FamiliarWindowList))
			{
				if (!clist->ItemsArray.Count)
				{
					bRefresh = true;
				}
			}
		}

		// ok it seems like the player has mounts/illusions/familiars in his keyring
		// lets make sure we initialize it for the Mount/Illusion/Familiar TLO
		if (bRefresh)
		{
			//WriteChatColor("Mount/Illusion/Familiar key ring initialized", CONCOLOR_YELLOW);
			RefreshKeyRingsThreadData* kr = new RefreshKeyRingsThreadData;
			kr->phWnd = krwnd;
			kr->bExact = false;
			kr->bUseCmd = false;
			kr->ItemName[0] = 0;
			RefreshKeyRings(kr);
		}
	}
}

//.text:00638049                 mov     ecx, pinstPCData_x
//.text:0063804F                 push    0
//.text:00638051                 push    0
//.text:00638053                 add     ecx, 1FE0h
//.text:00638059                 call    ?MakeMeVisible@CharacterZoneClient@@QAEXH_N@Z ; CharacterZoneClient::MakeMeVisible(int,bool)
void MakeMeVisible(SPAWNINFO* pChar, char* szLine)
{
	if (pCharData)
	{
		pCharData->MakeMeVisible(0, false);
	}
}

// ***************************************************************************
// Function:    RemoveAura
// Description: Removes auras
// Usage:       /removeaura <name> or <partial name>
// Author:      EqMule
// ***************************************************************************
void RemoveAura(SPAWNINFO* pChar, char* szLine)
{
	if (!pAuraWnd)
		return;

	if (!szLine || (szLine[0] == 0))
	{
		WriteChatColor("Usage: /removeaura <auraname> or <aurapartialname>", CONCOLOR_LIGHTBLUE);
		return;
	}

	if (CListWnd* clist = (CListWnd*)pAuraWnd->GetChildItem("AuraList"))
	{
		for (int i = 0; i < clist->ItemsArray.Count; i++)
		{
			CXStr Str = clist->GetItemText(i, 1);

			if (ci_find_substr(Str, szLine) != -1)
			{
				clist->SetCurSel(i);
				pAuraWnd->WndNotification(clist, XWM_MENUSELECT, (void*)1);
			}
		}
	}
}

bool GetAllMercDesc(std::map<int, MercDesc> & minfo)
{
	if (!pMercInfo)
		return false;

	if (MERCSLIST* pmlist = pMercInfo->pMercsList)
	{
		for (int i = 0; i < pMercInfo->MercenaryCount; i++)
		{
			MercDesc& outDesc = minfo[i];

			int mdesc = pmlist->mercinfo[i].nMercDesc;
			std::string smdesc = pCDBStr->GetString(mdesc, eMercenarySubCategoryDescription);
			size_t pos = 0;

			if ((pos = smdesc.find("Race: ")) != std::string::npos)
			{
				outDesc.Race = smdesc.substr(pos + 6);
				if ((pos = outDesc.Race.find("<br>")) != std::string::npos)
				{
					outDesc.Race.erase(pos);
				}
			}

			if ((pos = smdesc.find("Type: ")) != std::string::npos)
			{
				outDesc.Type = smdesc.substr(pos + 6);
				if ((pos = outDesc.Type.find("<br>")) != std::string::npos)
				{
					outDesc.Type.erase(pos);
				}
			}

			if ((pos = smdesc.find("Confidence: ")) != std::string::npos)
			{
				outDesc.Confidence = smdesc.substr(pos + 12);
				if ((pos = outDesc.Confidence.find("<br>")) != std::string::npos)
				{
					outDesc.Confidence.erase(pos);
				}
			}

			if ((pos = smdesc.find("Proficiency: ")) != std::string::npos)
			{
				outDesc.Proficiency = smdesc.substr(pos + 13);
				if ((pos = outDesc.Proficiency.find("<br>")) != std::string::npos)
				{
					outDesc.Proficiency.erase(pos);
				}
			}
		}
	}

	return true;
}

bool IsActiveAA(const char* pSpellName)
{
	int level = -1;
	if (SPAWNINFO* pMe = (SPAWNINFO*)pLocalPlayer)
	{
		level = pMe->Level;
	}

	for (int nAbility = 0; nAbility < AA_CHAR_MAX_REAL; nAbility++)
	{
		if (ALTABILITY* pAbility = GetAAByIdWrapper(pPCData->GetAlternateAbilityId(nAbility), level))
		{
			if (!_stricmp(pSpellName, pCDBStr->GetString(pAbility->nName, eAltAbilityName)))
			{
				if (pAbility->SpellID <= 0)
				{
					return true;
				}
			}
		}
	}

	return false;
}

struct Personal_Loot
{
	CButtonWnd* NPC_Name = nullptr;
	CButtonWnd* Item = nullptr;
	CButtonWnd* Loot = nullptr;
	CButtonWnd* Leave = nullptr;
	CButtonWnd* AN = nullptr;
	CButtonWnd* AG = nullptr;
	CButtonWnd* Never = nullptr;
};

CXWnd* GetAdvLootPersonalListItem(DWORD ListIndex, DWORD type)
{
	if (CListWnd* clist = (CListWnd*)pAdvancedLootWnd->GetChildItem("ADLW_PLLList"))
	{
		Personal_Loot pPAdvLoot;
		bool bFound = false;
		int listindex = -1;

		CXWnd* pFirstWnd = clist->GetFirstChildWnd();
		CXWnd* pNextWnd = pFirstWnd;

		for (int i = 0; i < clist->ItemsArray.Count; i++)
		{
			if (pNextWnd)
			{
				pPAdvLoot.NPC_Name = (CButtonWnd*)pNextWnd->GetFirstChildWnd();
				pNextWnd = pNextWnd->GetNextSiblingWnd();
				pPAdvLoot.Item = (CButtonWnd*)pNextWnd->GetFirstChildWnd();
				pNextWnd = pNextWnd->GetNextSiblingWnd();
				pPAdvLoot.Loot = (CButtonWnd*)pNextWnd->GetFirstChildWnd();
				pNextWnd = pNextWnd->GetNextSiblingWnd();
				pPAdvLoot.Leave = (CButtonWnd*)pNextWnd->GetFirstChildWnd();
				pNextWnd = pNextWnd->GetNextSiblingWnd();
				pPAdvLoot.Never = (CButtonWnd*)pNextWnd->GetFirstChildWnd();
				pNextWnd = pNextWnd->GetNextSiblingWnd();
				pPAdvLoot.AN = (CButtonWnd*)pNextWnd->GetFirstChildWnd();
				pNextWnd = pNextWnd->GetNextSiblingWnd();
				pPAdvLoot.AG = (CButtonWnd*)pNextWnd->GetFirstChildWnd();

				if (pNextWnd && pNextWnd->GetNextSiblingWnd())
				{
					pNextWnd = pNextWnd->GetNextSiblingWnd();
				}
			}

			if (ListIndex == i)
			{
				bFound = true;
				break;
			}
		}

		if (bFound)
		{
			CXWnd* ptr = nullptr;

			switch (type)
			{
			case 0:
				ptr = (CXWnd*)pPAdvLoot.NPC_Name;
				break;
			case 1:
				ptr = (CXWnd*)pPAdvLoot.Item;
				break;
			case 2:
				ptr = (CXWnd*)pPAdvLoot.Loot;
				break;
			case 3:
				ptr = (CXWnd*)pPAdvLoot.Leave;
				break;
			case 4:
				ptr = (CXWnd*)pPAdvLoot.Never;
				break;
			case 5:
				ptr = (CXWnd*)pPAdvLoot.AN;
				break;
			case 6:
				ptr = (CXWnd*)pPAdvLoot.AG;
				break;
			}

			return ptr;
		}
	}

	return nullptr;
}

struct Shared_Loot
{
	CButtonWnd* NPC_Name = nullptr;
	CButtonWnd* Item = nullptr;
	CButtonWnd* Status = nullptr;
	CButtonWnd* Action = nullptr;
	CButtonWnd* Manage = nullptr;
	CButtonWnd* AutoRoll = nullptr;
	CButtonWnd* ND = nullptr;
	CButtonWnd* GD = nullptr;
	CButtonWnd* NO = nullptr;
	CButtonWnd* AN = nullptr;
	CButtonWnd* AG = nullptr;
	CButtonWnd* NV = nullptr;
};

CXWnd* GetAdvLootSharedListItem(DWORD ListIndex, DWORD type)
{
	if (CListWnd* clist = (CListWnd*)pAdvancedLootWnd->GetChildItem("ADLW_CLLList"))
	{
		Shared_Loot pSAdvLoot;
		bool bFound = false;

		CXWnd* pFirstWnd = clist->GetFirstChildWnd();
		CXWnd* pNextWnd = pFirstWnd;

		for (int i = 0; i < clist->ItemsArray.Count; i++)
		{
			if (pNextWnd)
			{
				pSAdvLoot.NPC_Name = (CButtonWnd*)pNextWnd->GetFirstChildWnd();
				pNextWnd = pNextWnd->GetNextSiblingWnd();
				pSAdvLoot.Item = (CButtonWnd*)pNextWnd->GetFirstChildWnd();
				pNextWnd = pNextWnd->GetNextSiblingWnd();
				pSAdvLoot.Status = (CButtonWnd*)pNextWnd->GetFirstChildWnd();
				pNextWnd = pNextWnd->GetNextSiblingWnd();
				pSAdvLoot.Action = (CButtonWnd*)pNextWnd->GetFirstChildWnd();
				pNextWnd = pNextWnd->GetNextSiblingWnd();
				pSAdvLoot.Manage = (CButtonWnd*)pNextWnd->GetFirstChildWnd();
				pNextWnd = pNextWnd->GetNextSiblingWnd();
				pSAdvLoot.AN = (CButtonWnd*)pNextWnd->GetFirstChildWnd();
				pNextWnd = pNextWnd->GetNextSiblingWnd();
				pSAdvLoot.AG = (CButtonWnd*)pNextWnd->GetFirstChildWnd();
				pNextWnd = pNextWnd->GetNextSiblingWnd();
				pSAdvLoot.AutoRoll = (CButtonWnd*)pNextWnd->GetFirstChildWnd();
				pNextWnd = pNextWnd->GetNextSiblingWnd();
				pSAdvLoot.NV = (CButtonWnd*)pNextWnd->GetFirstChildWnd();
				pNextWnd = pNextWnd->GetNextSiblingWnd();
				pSAdvLoot.ND = (CButtonWnd*)pNextWnd->GetFirstChildWnd();
				pNextWnd = pNextWnd->GetNextSiblingWnd();
				pSAdvLoot.GD = (CButtonWnd*)pNextWnd->GetFirstChildWnd();
				pNextWnd = pNextWnd->GetNextSiblingWnd();
				pSAdvLoot.NO = (CButtonWnd*)pNextWnd->GetFirstChildWnd();

				if (pNextWnd && pNextWnd->GetNextSiblingWnd())
				{
					pNextWnd = pNextWnd->GetNextSiblingWnd();
				}
			}
			if (ListIndex == i)
			{
				bFound = true;
				break;
			}
		}

		// NPC_Name,Item,Status,Action,Manage,AN,AG,AutoRoll,NV,ND,GD,NO
		if (bFound)
		{
			CXWnd* ptr = nullptr;
			switch (type)
			{
			case 0:
				ptr = (CXWnd*)pSAdvLoot.NPC_Name;
				break;
			case 1:
				ptr = (CXWnd*)pSAdvLoot.Item;
				break;
			case 2:
				ptr = (CXWnd*)pSAdvLoot.Status;
				break;
			case 3:
				ptr = (CXWnd*)pSAdvLoot.Action;
				break;
			case 4:
				ptr = (CXWnd*)pSAdvLoot.Manage;
				break;
			case 5:
				ptr = (CXWnd*)pSAdvLoot.AN;
				break;
			case 6:
				ptr = (CXWnd*)pSAdvLoot.AG;
				break;
			case 7:
				ptr = (CXWnd*)pSAdvLoot.AutoRoll;
				break;
			case 8:
				ptr = (CXWnd*)pSAdvLoot.NV;
				break;
			case 9:
				ptr = (CXWnd*)pSAdvLoot.ND;
				break;
			case 10:
				ptr = (CXWnd*)pSAdvLoot.GD;
				break;
			case 11:
				ptr = (CXWnd*)pSAdvLoot.NO;
				break;
			case 12://root
				ptr = (CXWnd*)pSAdvLoot.Item;
				break;
			}

			return ptr;
		}
	}

	return nullptr;
}

bool LootInProgress(CAdvancedLootWnd* pAdvLoot, CListWnd* pPersonalList, CListWnd* pSharedList)
{
	if (pPersonalList)
	{
		for (int i = 0; i < pPersonalList->ItemsArray.Count; i++)
		{
			int listindex = (int)pPersonalList->GetItemData(i);
			if (listindex != -1)
			{
				AdvancedLootItem& lootItem = pAdvLoot->pPLootList->Items[listindex];
				if (lootItem.PLootInProgress || lootItem.CLootInProgress)
				{
					return true;
				}
			}
		}
	}

	if (pSharedList)
	{
		for (int i = 0; i < pSharedList->ItemsArray.Count; i++)
		{
			int listindex = (int)pSharedList->GetItemData(i);
			if (listindex != -1)
			{
				AdvancedLootItem& lootItem = pAdvLoot->pCLootList->Items[listindex];
				if (lootItem.PLootInProgress || lootItem.CLootInProgress)
				{
					return true;
				}
			}
		}
	}

	return false;
}

void WeDidStuff()
{
	gbCommandEvent = 1;
	gMouseEventTime = GetFastTime();
}

int GetFreeInventory(int nSize)
{
	PcProfile* pProfile = GetPcProfile();
	if (!pProfile)
		return 0;

	int freeSlots = 0;

	if (nSize)
	{
		for (int slot = BAG_SLOT_START; slot < NUM_INV_SLOTS; slot++)
		{
			if (pProfile->pInventoryArray
				&& pProfile->pInventoryArray->InventoryArray
				&& pProfile->pInventoryArray->InventoryArray[slot])
			{
				CONTENTS* pItem = pProfile->pInventoryArray->InventoryArray[slot];

				if (GetItemFromContents(pItem)->Type == ITEMTYPE_PACK
					&& GetItemFromContents(pItem)->SizeCapacity >= nSize)
				{
					if (!pItem->Contents.ContainedItems.pItems)
					{
						freeSlots += GetItemFromContents(pItem)->Slots;
					}
					else
					{
						for (int pSlot = 0; pSlot < GetItemFromContents(pItem)->Slots; pSlot++)
						{
							if (!pItem->GetContent(pSlot))
							{
								freeSlots++;
							}
						}
					}
				}
			}
			else
			{
				freeSlots++;
			}
		}
	}
	else
	{
		for (int slot = BAG_SLOT_START; slot < NUM_INV_SLOTS; slot++)
		{
			if (!HasExpansion(EXPANSION_HoT) && slot > BAG_SLOT_START + 7)
			{
				break;
			}

			if (pProfile->pInventoryArray
				&& pProfile->pInventoryArray->InventoryArray
				&& pProfile->pInventoryArray->InventoryArray[slot])
			{
				CONTENTS* pItem = pProfile->pInventoryArray->InventoryArray[slot];

				if (GetItemFromContents(pItem)->Type == ITEMTYPE_PACK)
				{
					if (!pItem->Contents.ContainedItems.pItems)
					{
						freeSlots += GetItemFromContents(pItem)->Slots;
					}
					else
					{
						for (int pSlot = 0; pSlot < (GetItemFromContents(pItem)->Slots); pSlot++)
						{
							if (!pItem->GetContent(pSlot))
							{
								freeSlots++;
							}
						}
					}
				}
			}
			else
			{
				freeSlots++;
			}
		}
	}

	return freeSlots;
}

bool CanItemMergeInPack(CONTENTS* pPack, CONTENTS* pItem)
{
	for (size_t i = 0; i < pPack->Contents.ContainedItems.Size; i++)
	{
		if (CONTENTS* pSlot = pPack->Contents.ContainedItems.pItems->Item[i])
		{
			if (pSlot->ID == pItem->ID)
			{
				if (ITEMINFO* pItemInfo = GetItemFromContents(pSlot))
				{
					if (pSlot->StackCount + pItem->StackCount <= (int)pItemInfo->StackSize)
					{
						return true;
					}
				}
			}
		}
	}

	return false;
}

bool CanItemGoInPack(CONTENTS* pPack, CONTENTS* pItem)
{
	// so CanGoInBag doesnt actually check if there is any room, all it checks
	// is IF there where room, could the item go in it.
	if (!((EQ_Item*)pItem)->CanGoInBag(&pPack))
		return false;

	for (size_t i = 0; i < pPack->Contents.ContainedItems.Size; i++)
	{
		if (CONTENTS* pSlot = pPack->Contents.ContainedItems.pItems->Item[i])
		{

		}
		else
		{
			return true; // free slot...
		}
	}

	return false;
}

bool WillFitInBank(CONTENTS* pContent)
{
	ITEMINFO* pMyItem = GetItemFromContents(pContent);
	if (!pMyItem)
		return false;

	CHARINFONEW* pChar = (CHARINFONEW*)GetCharInfo();
	if (!pChar)
		return false;

	for (size_t slot = 0; slot < pChar->BankItems.Items.Size; slot++)
	{
		CONTENTS* pCont = pChar->BankItems.Items[slot].pObject;
		if (!pCont)
		{
			// if its empty it will fit.
			return true;
		}

		ITEMINFO* pItem = GetItemFromContents(pCont);
		if (!pItem) continue;

		if (pItem->Type == ITEMTYPE_PACK)
		{
			if (CanItemMergeInPack(pCont, pContent))
			{
				return true;
			}

			if (CanItemGoInPack(pCont, pContent))
			{
				return true;
			}
		}
		else
		{
			// its not a pack but its an item, do we match?
			if (pCont->ID == pContent->ID)
			{
				if (pCont->StackCount + pContent->StackCount <= pItem->StackSize)
				{
					return true;
				}
			}
		}
	}

	return false;
}

bool WillFitInInventory(CONTENTS* pContent)
{
	ITEMINFO* pMyItem = GetItemFromContents(pContent);
	if (!pMyItem)
		return false;

	PcProfile* pProfile = GetPcProfile();
	if (!pProfile)
		return false;

	if (pProfile->pInventoryArray && pProfile->pInventoryArray->InventoryArray)
	{
		for (int slot = BAG_SLOT_START; slot < NUM_INV_SLOTS; slot++)
		{
			CONTENTS* pCont = pProfile->pInventoryArray->InventoryArray[slot];
			if (!pCont)
			{
				// if its empty it will fit.
				return true;
			}

			ITEMINFO* pItem = GetItemFromContents(pCont);
			if (!pItem)
				continue;

			if (pItem->Type == ITEMTYPE_PACK)
			{
				if (CanItemMergeInPack(pCont, pContent))
				{
					return true;
				}
				else if (CanItemGoInPack(pCont, pContent))
				{
					return true;
				}
			}
			else
			{
				// its not a pack but its an item, do we match?
				if (pCont->ID == pContent->ID)
				{
					if (pCont->StackCount + pContent->StackCount <= pItem->StackSize)
					{
						return true;
					}
				}
			}
		}
	}

	return false;
}

int GetGroupMemberClassByIndex(int index)
{
	if (CHARINFO* pChar = GetCharInfo())
	{
		if (!pChar->pGroupInfo)
			return 0;

		if (pChar->pGroupInfo->pMember[index] && pChar->pGroupInfo->pMember[index]->pSpawn)
		{
			return pChar->pGroupInfo->pMember[index]->pSpawn->mActorClient.Class;
		}
	}

	return 0;
}

int GetRaidMemberClassByIndex(int index)
{
	if (pRaid && pRaid->Invited == RaidStateInRaid)
	{
		if (pRaid->RaidMemberUsed[index])
			return pRaid->RaidMember[index].nClass;
	}

	return 0;
}

bool Anonymize(char* name, int maxlen, int NameFlag)
{
	if (GetGameState() != GAMESTATE_INGAME || !pLocalPlayer)
		return false;

	int isRmember = -1;
	bool isGmember = false;
	bool bChange = false;

	SPAWNINFO* pMySpawn = (SPAWNINFO*)pLocalPlayer;
	SPAWNINFO* pMyTarget = (SPAWNINFO*)pTarget;

	// if it is me, then there is no point in checking if its a group member
	int ItsMe = _stricmp(pMySpawn->Name, name);
	if (ItsMe != 0)
		isGmember = IsGroupMember(name);

	// if it is me or a groupmember, then there is no point in checking if its a raid member
	if (!isGmember && ItsMe != 0)
		isRmember = IsRaidMember(name);

	bool bisTarget = false;
	if (ItsMe != 0 && !isGmember && isRmember)
	{
		// my target?
		if (pTarget && pMyTarget->Type != SPAWN_NPC)
		{
			if (!_strnicmp(pMyTarget->DisplayedName, name, strlen(pMyTarget->DisplayedName)))
			{
				bisTarget = true;
			}
		}
	}

	if (ItsMe == 0 || isGmember || isRmember != -1 || bisTarget)
	{
		if (NameFlag == 1)
		{
			char buffer[L_tmpnam] = { 0 };
			tmpnam_s(buffer);
			char* pDest = strrchr(buffer, '\\');

			int len = strlen(name);
			for (int i = 1; i < len - 1; i++)
			{
				name[i] = '*';
			}

			strcat_s(name, 32, &pDest[1]);
			return true;
		}

		if (gAnonymizeFlag == EAF_Class)
		{
			if (ItsMe == 0)
			{
				strncpy_s(name, 16, GetClassDesc(pMySpawn->mActorClient.Class), 15);
				if (NameFlag == 2)
				{
					strcat_s(name, 16, "_0");
				}
				bChange = true;
			}
			else if (bisTarget)
			{
				strncpy_s(name, 16, GetClassDesc(pMyTarget->mActorClient.Class), 15);
				bChange = true;
			}
			else if (isGmember)
			{
				int theclass = GetGroupMemberClassByIndex(isGmember);
				strncpy_s(name, 16, GetClassDesc(theclass), 15);
				if (NameFlag == 2)
				{
					char sztmp[16];
					sprintf_s(sztmp, "_%d", isGmember);
					strcat_s(name, 16, sztmp);
				}
				bChange = true;
			}
			else if (isRmember != -1)
			{
				int theclass = GetRaidMemberClassByIndex(isRmember);
				strncpy_s(name, 16, GetClassDesc(theclass), 15);
				if (NameFlag == 2)
				{
					char sztmp[16];
					sprintf_s(sztmp, "_%d", isRmember);
					strcat_s(name, 16, sztmp);
				}
				bChange = true;
			}
		}
		else
		{
			int len = strlen(name);
			bChange = true;
			for (int i = 1; i < len - 1; i++)
			{
				name[i] = '*';
			}
		}
	}

	return bChange;
}

// this is not ideal.
bool Anonymize2(CXStr & name, int LootFlag /*= 0*/)
{
	char szOut[MAX_STRING];
	strcpy_s(szOut, name.c_str());

	bool result = Anonymize(szOut, MAX_STRING, LootFlag);

	if (result)
	{
		name = szOut;
	}

	return result;
}

void UpdatedMasterLooterLabel()
{
	if (!pAdvancedLootWnd)
		return;

	CHARINFO* pChar = GetCharInfo();
	if (!pChar || !pChar->pGroupInfo)
		return;

	CLabelWnd* MasterLooterLabel = (CLabelWnd*)pAdvancedLootWnd->GetChildItem("ADLW_CalculatedMasterLooter");
	if (!MasterLooterLabel)
		return;

	CXStr text;
	bool bFound = false;

	for (auto member : pChar->pGroupInfo->pMember)
	{
		if (member && member->MasterLooter)
		{
			text = member->Name;
			if (gAnonymize)
			{
				Anonymize2(text);
			}

			bFound = true;
			break;
		}
	}

	if (bFound)
	{
		MasterLooterLabel->SetWindowText(text);
	}
}

// this function performs a better rand since it removes the random bias
// towards the low end if the range of rand() isn't divisible by max - min + 1
int RangeRandom(int min, int max)
{
	int n = max - min + 1;
	int remainder = RAND_MAX % n;
	int x;
	do {
		x = rand();
	} while (x >= RAND_MAX - remainder);
	return min + x % n;
}

//============================================================================

ITEMINFO* GetItemFromContents(CONTENTS* c)
{
	if (!c)
		return nullptr;

	return c->Item1 ? c->Item1 : c->Item2;
}

struct EnumWindowsData
{
	HWND outHWnd;
	DWORD processId;
};

static BOOL CALLBACK EnumWindowsProc(HWND hWnd, LPARAM lParam)
{
	EnumWindowsData* enumData = reinterpret_cast<EnumWindowsData*>(lParam);

	// Get the process id for the window.
	DWORD dwProcessId = 0;
	GetWindowThreadProcessId(hWnd, &dwProcessId);

	// Only check windows in the current process
	if (enumData->processId == dwProcessId)
	{
		char szClass[24] = { 0 };
		GetClassName(hWnd, szClass, 23);

		// If its the EverQuest window class, return it.
		if (strcmp(szClass, "_EverQuestwndclass") == 0)
		{
			enumData->outHWnd = hWnd;
			return false;
		}
	}

	return true;
}

EQLIB_API HWND GetEQWindowHandle()
{
	DWORD dwProcessId = GetCurrentProcessId();

	EnumWindowsData enumData;
	enumData.outHWnd = nullptr;
	enumData.processId = dwProcessId;

	EnumWindows(EnumWindowsProc, (LPARAM)&enumData);

	return enumData.outHWnd;
}

// ***************************************************************************
// Function:    GetCharMaxBuffSlots
// Description: Returns the max number of buff slots available for a character
// ***************************************************************************
int GetCharMaxBuffSlots()
{
	int NumBuffs = 15;

	if (CHARINFO* pChar = GetCharInfo())
	{
		NumBuffs += pCharData->TotalEffect(327, 1, 0, 1, 1);

		if (pChar->pSpawn && pChar->pSpawn->Level > 70)
			NumBuffs++;
		if (pChar->pSpawn && pChar->pSpawn->Level > 74)
			NumBuffs++;
	}

	return NumBuffs;
}

int GetBodyType(SPAWNINFO* pSpawn)
{
	for (int i = 0; i < 104; i++)
	{
		PlayerClient* pc = (PlayerClient*)pSpawn;

		if (pc->HasProperty(i, 0, 0))
		{
			if (i == 100)
			{
				if (pc->HasProperty(i, 101, 0))
					return 101;
				if (pc->HasProperty(i, 102, 0))
					return 102;
				if (pc->HasProperty(i, 103, 0))
					return 103;
			}
			return i;
		}
	}

	return 0;
}

eSpawnType GetSpawnType(SPAWNINFO* pSpawn)
{
	switch (pSpawn->Type)
	{
	case SPAWN_PLAYER:
		return PC;

	case SPAWN_NPC:
		if (pSpawn->Rider)
			return MOUNT;

		if (pSpawn->MasterID)
			return PET;
		if (pSpawn->Mercenary)
			return MERCENARY;

		// some type of controller spawn for flying mobs - locations, speed, heading, all NaN
		if (IsNaN(pSpawn->Y) && IsNaN(pSpawn->X) && IsNaN(pSpawn->Z))
			return FLYER;

		switch (GetBodyType(pSpawn))
		{
		case 0:
			if (pSpawn->mActorClient.Class == 62)
				return OBJECT;
			return NPC;

		case 1:
			if (pSpawn->mActorClient.Race == 567)
				return CAMPFIRE;
			if (pSpawn->mActorClient.Race == 500|| (pSpawn->mActorClient.Race >= 553 && pSpawn->mActorClient.Race <= 557) || pSpawn->mActorClient.Race == 586)
				return BANNER;
			return NPC;

			//case 3:
			//    return NPC;

		case 5:
			if (strstr(pSpawn->Name, "Idol") || strstr(pSpawn->Name, "Poison") || strstr(pSpawn->Name, "Rune"))
				return AURA;
			if (pSpawn->mActorClient.Class == 62)
				return OBJECT;
			return NPC;

		case 7:
			if (pSpawn->mActorClient.Class == 62)
				return OBJECT;
			return NPC;

		case 11:
			if (strstr(pSpawn->Name, "Aura") || strstr(pSpawn->Name, "Circle_of") || strstr(pSpawn->Name, "Guardian_Circle") || strstr(pSpawn->Name, "Earthen_Strength"))
				return AURA;
			return UNTARGETABLE;

			//case 21:
			//    return NPC;
			//case 23:
			//    return NPC;

		case 33:
			return CHEST;

			//case 34:
			//    return NPC;
			//case 65:
			//    return TRAP;
			//case 66:
			//    return TIMER;
			//case 67:
			//    return TRIGGER;

		case 100:
			return UNTARGETABLE
				;
		case 101:
			return TRAP;

		case 102:
			return TIMER;

		case 103:
			return TRIGGER;

		default: break;
		}
		return NPC;

	case SPAWN_CORPSE:
		return CORPSE;

	default: break;
	}

	return ITEM;
}

bool IsRaidMember(const char* SpawnName)
{
	if (pRaid && pRaid->Invited == RaidStateInRaid)
	{
		for (int index = 0; index < MAX_RAID_SIZE; index++)
		{
			if (pRaid->RaidMemberUsed[index] && !_stricmp(SpawnName, pRaid->RaidMember[index].Name))
				return true;
		}
	}

	return false;
}

int GetRaidMemberIndex(const char* SpawnName)
{
	if (pRaid && pRaid->Invited == RaidStateInRaid)
	{
		for (int index = 0; index < MAX_RAID_SIZE; index++)
		{
			if (pRaid->RaidMemberUsed[index] && !_stricmp(SpawnName, pRaid->RaidMember[index].Name))
				return index;
		}
	}

	return -1;
}

bool IsRaidMember(SPAWNINFO* pSpawn)
{
	for (int index = 0; index < MAX_RAID_SIZE; index++)
	{
		if (pRaid->RaidMemberUsed[index] && !_stricmp(pSpawn->Name, pRaid->RaidMember[index].Name))
			return true;
	}

	return false;
}

int GetRaidMemberIndex(SPAWNINFO* pSpawn)
{
	for (int index = 0; index < MAX_RAID_SIZE; index++)
	{
		if (pRaid->RaidMemberUsed[index] && !_stricmp(pSpawn->Name, pRaid->RaidMember[index].Name))
			return index;
	}

	return -1;
}

bool IsGroupMember(const char* SpawnName)
{
	if (CHARINFO* pChar = GetCharInfo())
	{
		if (!pChar->pGroupInfo)
			return nullptr;

		for (int index = 1; index < MAX_GROUP_SIZE; index++)
		{
			if (pChar->pGroupInfo->pMember[index])
			{
				char Name[MAX_STRING] = { 0 };
				strcpy_s(Name, pChar->pGroupInfo->pMember[index]->Name.c_str());

				CleanupName(Name, sizeof(Name), false, false);

				if (!_stricmp(SpawnName, Name))
					return true;
			}
		}
	}

	return false;
}

bool IsGroupMember(SPAWNINFO* pSpawn)
{
	if (CHARINFO* pChar = GetCharInfo())
	{
		if (!pChar->pGroupInfo)
			return false;

		for (int index = 1; index < MAX_GROUP_SIZE; index++)
		{
			if (pChar->pGroupInfo->pMember[index])
			{
				char Name[MAX_STRING] = { 0 };
				strcpy_s(Name, pChar->pGroupInfo->pMember[index]->Name.c_str());

				//CleanupName(Name, sizeof(Name), false, false);

				if (!_stricmp(pSpawn->Name, Name))
					return true;
			}
		}
	}

	return false;
}

bool IsFellowshipMember(const char* SpawnName)
{
	if (CHARINFO* pChar = GetCharInfo())
	{
		if (!pChar->pFellowship)
			return false;

		for (int index = 0; index < pChar->pFellowship->Members; index++)
		{
			if (!_stricmp(SpawnName, pChar->pFellowship->FellowshipMember[index].Name))
				return true;
		}
	}

	return false;
}

bool IsGuildMember(const char* SpawnName)
{
	if (CHARINFO* pChar = GetCharInfo())
	{
		if (pChar->GuildID == 0)
			return false;

		if (pGuild)
		{
			if (GuildMember* mem = pGuild->FindMemberByName(SpawnName))
			{
				if (!_stricmp(SpawnName, mem->Name))
					return true;
			}
		}
	}

	return false;
}

int GetGroupMercenaryCount(uint32_t ClassMASK)
{
	int retValue = 0;

	if (CHARINFO* pChar = GetCharInfo())
	{
		if (!pChar->pGroupInfo)
			return 0;

		for (int index = 1; index < MAX_GROUP_SIZE; index++)
		{
			if (pChar->pGroupInfo->pMember[index])
			{
				if (pChar->pGroupInfo->pMember[index]->Mercenary
					&& (ClassMASK & (1 << (pChar->pGroupInfo->pMember[index]->pSpawn->mActorClient.Class - 1))))
				{
					retValue++;
				}
			}
		}
	}

	return retValue;
}

SPAWNINFO* GetRaidMember(int index)
{
	if (index >= MAX_RAID_SIZE)
		return nullptr;

	EQRAIDMEMBER* pRaidMember = &pRaid->RaidMember[index];

	if (!pRaidMember)
		return nullptr;

	return (SPAWNINFO*)GetSpawnByName(pRaidMember->Name);
}

inline SPAWNINFO* GetGroupMember(int index)
{
	if (index >= MAX_GROUP_SIZE)
		return nullptr;

	CHARINFO* pChar = GetCharInfo();
	if (!pChar->pGroupInfo)
		return nullptr;

	for (int i = 1; i < MAX_GROUP_SIZE; i++)
	{
		if (pChar->pGroupInfo->pMember[i])
		{
			index--;

			if (index == 0)
			{
				// FIXME: Why a copy?
				char Name[MAX_STRING] = { 0 };
				strcpy_s(Name, pChar->pGroupInfo->pMember[i]->Name.c_str());

				return (SPAWNINFO*)GetSpawnByName(Name);
			}
		}
	}

	return nullptr;
}

uint32_t GetGroupMainAssistTargetID()
{
	if (CHARINFO* pChar = GetCharInfo())
	{
		bool bMainAssist = false;

		if (GROUPINFO* pGroup = pChar->pGroupInfo)
		{
			if (GROUPMEMBER* pMember = pGroup->pMember[0])
			{
				for (int i = 0; i < MAX_GROUP_SIZE; i++)
				{
					if (pGroup->pMember[i])
					{
						if (pGroup->pMember[i]->MainAssist)
						{
							bMainAssist = true;
							break;
						}
					}
				}
			}
		}

		if (bMainAssist && pChar->pSpawn)
		{
			return pChar->pSpawn->GroupAssistNPC[0];
		}
	}

	return 0;
}

uint32_t GetRaidMainAssistTargetID(int index)
{
	if (SPAWNINFO* pSpawn = (SPAWNINFO*)pLocalPlayer)
	{
		if (pRaid)
		{
			bool bMainAssist = false;

			for (int i = 0; i < MAX_RAID_SIZE; i++)
			{
				if (pRaid->RaidMemberUsed[i] && pRaid->RaidMember[i].RaidMainAssist)
				{
					bMainAssist = true;
					break;
				}
			}

			if (bMainAssist)
			{
				// FIXME: Constant for raid assists
				if (index < 0 || index > 3)
					index = 0;

				return pSpawn->RaidAssistNPC[index];
			}
		}
	}

	return 0;
}

bool IsAssistNPC(SPAWNINFO* pSpawn)
{
	if (pSpawn)
	{
		if (uint32_t AssistID = GetGroupMainAssistTargetID())
		{
			if (AssistID == pSpawn->SpawnID)
			{
				return true;
			}
		}

		for (int nAssist = 0; nAssist < 3; nAssist++)
		{
			if (uint32_t AssistID = GetRaidMainAssistTargetID(nAssist))
			{
				if (AssistID == pSpawn->SpawnID)
				{
					return true;
				}
			}
		}
	}

	return false;
}