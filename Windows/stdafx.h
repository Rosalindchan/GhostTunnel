// stdafx.h : 标准系统包含文件的包含文件，
// 或是经常使用但不常更改的
// 特定于项目的包含文件
//

#pragma once

#include "targetver.h"

#include <stdio.h>
#include <tchar.h>

#define __DEBUG__
#ifdef  __DEBUG__
#define HLOG(format,...) printf(format,##__VA_ARGS__)
#else
#define HLOG(format,...)
#endif

// TODO: 在此处引用程序需要的其他头文件
