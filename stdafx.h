// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#include <stdio.h>
#include <tchar.h>

#include <string>


struct MemoryStruct {
	char *memory;
	size_t size;
};



int DownloadFile(std::string url, struct MemoryStruct& csv);
