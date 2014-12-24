// //////////////////////////////////////////////////////////////////////
//
// This file is under the license http://www.apache.org/licenses/LICENSE-2.0
//
// //////////////////////////////////////////////////////////////////////

#ifndef __TRACE_DEBUG_HPP
#define __TRACE_DEBUG_HPP

// See here an example of the output:
//Processing f1() From main.cpp:368 (main)
// Processing f2() From f1.cpp:83 (f1)
// Processing f3() From f2.cpp:651 (f2)
// ->f2.cpp:651 (f2) f3() = 1
// Processing value From f2.cpp:660 (f2)
// ->f2.cpp:660 (f2) value = 1
// ->f1.cpp:83 (f1) f2() = 1
//->main.cpp:368 (main) f1() = 1
// Source code:
// main.cpp @ line 368
// DISPLAY_DEBUG_VALUE(f1());
// f1.cpp @ line 83
// DISPLAY_DEBUG_VALUE(f2());
// f2.cpp @ line 651
// DISPLAY_DEBUG_VALUE(f3());
// f2.cpp @ line 660
// DISPLAY_DEBUG_VALUE(value);

#include <map>
#include <string>
#include <iostream>

#define TOKENPASTE(x, y) x ## y
#define TOKENPASTE_EXPAND(x, y) TOKENPASTE(x , y)
// Activate traces
#define DISPLAY_DEBUG_ACTIVE_TRACE DebugTrace::ActiveTrace(true)
// Deactivate traces: any call to any debug macro will not be displaying anything. However the hierarchy is kept up to date.
// When traces are activated agin the lastcomputed hierarchy will be used to insert the right number of blank spaces
#define DISPLAY_DEBUG_DEACTIVE_TRACE DebugTrace::ActiveTrace(false)
// Display a value specifying the expression, its value and many blank spaces defining the deepness of the hierarchy,
// the filename, line number and function name are all displayed.
#define DISPLAY_DEBUG_VALUE(value) \
  DebugTrace TOKENPASTE_EXPAND(__Unused, __LINE__)(__func__, __FILE__, __LINE__); \
  if(DebugTrace::IsTraceActive()) { \
    std::cout << DebugTrace::getSpaces() << "Processing " << #value << "  From " << __BASE_FILE__ << ":" << __LINE__ << " ("<< __func__ << ")" << std::endl; \
    std::cout << DebugTrace::getSpaces() << "->" << __BASE_FILE__ << ":" << __LINE__ << " ("<< __func__ << ")\t" << #value << " = " << (value) << std::endl; \
  }
// Display a message and many blank spaces defining the deepness of the hierarchy
#define DISPLAY_DEBUG_MESSAGE(message) \
    DebugTrace TOKENPASTE_EXPAND(__Unused, __LINE__)(__func__, __FILE__, __LINE__); \
    if(DebugTrace::IsTraceActive()) { std::cout << message << std::endl; }
// Display a value specifying the expression, its value and many blank spaces defining the deepness of the hierarchy
// deeper hierarchy will however not be displayed
#define DISPLAY_DEBUG_VALUE_NON_HER(value) \
    DebugTrace TOKENPASTE_EXPAND(__Unused, __LINE__)(__func__, __FILE__, __LINE__); \
    if(DebugTrace::IsTraceActive()) { \
      DISPLAY_DEBUG_DEACTIVE_TRACE; \
      std::cout << DebugTrace::getSpaces() << __BASE_FILE__ << ":" << __LINE__ << " ("<< __func__ << ")\t" << #value << " = " << (value) << std::endl; \
      DISPLAY_DEBUG_ACTIVE_TRACE; \
    }

class DebugTrace {
    static int deep;
    static bool traceActive;
    static std::map<std::string, int> mapFileNameToLine;
    bool mustBeDecremented;
    std::string keyToErase;
  public:
    DebugTrace(const std::string & functionName, const std::string & fileName = __FILE__, int lineNumber = __LINE__): mustBeDecremented(false)  {
      std::string uniqueKey = fileName + functionName;
      bool savedFileName = mapFileNameToLine.find(uniqueKey) != mapFileNameToLine.end();
      if(!savedFileName || mapFileNameToLine[uniqueKey] == lineNumber) {
        keyToErase = uniqueKey;
        mapFileNameToLine[uniqueKey] = lineNumber;
        ++deep;
        mustBeDecremented = true;
      }
    }
    ~DebugTrace() {
      if(mustBeDecremented && deep > 0) {
        --deep;
        mapFileNameToLine.erase(keyToErase);
      }
      if(deep == 0) {
        mapFileNameToLine.clear();
      }
    }
    static void ActiveTrace(bool activate) {
      traceActive = activate;
    }
    static bool IsTraceActive() {
      return traceActive;
    }
    static std::string getSpaces() {
      return std::string(2 * (deep - 1), ' ');
    }
};

#endif
