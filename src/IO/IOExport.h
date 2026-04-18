/**
 * @file IOExport.h
 * @brief IO 模块导出宏定义
 * @author hxxcxx
 * @date 2026-04-18
 */
#pragma once

#ifdef BUILDING_IO
  #define IO_API __declspec(dllexport)
#else
  #define IO_API __declspec(dllimport)
#endif
