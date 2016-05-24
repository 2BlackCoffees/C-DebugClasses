// //////////////////////////////////////////////////////////////////////
//
// This file is under the license http://www.apache.org/licenses/LICENSE-2.0
//
// //////////////////////////////////////////////////////////////////////

#ifdef ENABLE_TRACE_DEBUG
#include "TraceDebug.hpp"
int DebugTrace::debugPrintDeepness = 0;
bool DebugTrace::traceActive = true;
std::map<std::string, int> DebugTrace::mapFileNameToLine;
std::map<std::string,
             std::vector<std::pair<std::string,
                                   std::chrono::system_clock::time_point>>>
                          DebugTrace::mapFileNameFunctionNameToVectorTimingInfo;
std::string DebugTrace::GetUniqueKey(const std::string &string1,
                                     const std::string &string2,
                                     const std::string &string3) {
  return string1 + string2 + string3;
}

DebugTrace::DebugTrace(const std::string& functionName,
                       const std::string& fileName,
                       int lineNumber,
                       const std::string& uniqueKey)
{
  keyDebugPerformanceToErase = uniqueKey;
  debugPerformanceMustBeDisplayed = true;
  ++debugPrintDeepness;
  lineHeader = DebugTrace::getSpaces() + functionName + " (" + uniqueKey + ") " +
      fileName + ":" + std::to_string(lineNumber);
  AddTrace(std::chrono::steady_clock::now(), "Start measure");
}

DebugTrace::DebugTrace(const std::string &functionName,
                       const std::string &fileName,
                       int lineNumber): debugPrintMustBeDecremented(false)  {
  debugPerformanceMustBeDisplayed = false;
  std::string uniqueKey = GetUniqueKey(fileName, functionName);
  bool savedFileName = mapFileNameToLine.find(uniqueKey) != mapFileNameToLine.end();
  if(!savedFileName || mapFileNameToLine[uniqueKey] == lineNumber) {
    keyDebugPrintToErase = uniqueKey;
    mapFileNameToLine[uniqueKey] = lineNumber;
    ++debugPrintDeepness;
    debugPrintMustBeDecremented = true;
  }
}

DebugTrace::~DebugTrace() {
  if((debugPrintMustBeDecremented || debugPerformanceMustBeDisplayed) &&
     debugPrintDeepness > 0) {
    --debugPrintDeepness;
    if(debugPrintMustBeDecremented) {
      mapFileNameToLine.erase(keyDebugPrintToErase);
    }
  }
  if(debugPrintDeepness == 0) {
    mapFileNameToLine.clear();
  }
  if(debugPerformanceMustBeDisplayed) {
    AddTrace(std::chrono::steady_clock::now(), "End measure");
#ifdef USE_QT_DEBUG
      qDebug() << QString::fromUtf8(GetPerformanceResults().c_str());
#else
    std::cout << GetPerformanceResults() << std::endl;
#endif
    mapFileNameFunctionNameToVectorTimingInfo.erase(keyDebugPerformanceToErase);
  }
}

void DebugTrace::AddTrace(std::chrono::system_clock::time_point timePoint,
                          const std::string & variableName) {
  std::pair<std::string, std::chrono::system_clock::time_point> tmpPair =
      std::make_pair(variableName, timePoint);

  auto vectorTimingInfoIt =
      mapFileNameFunctionNameToVectorTimingInfo.find(keyDebugPerformanceToErase);
  if(vectorTimingInfoIt == mapFileNameFunctionNameToVectorTimingInfo.end()) {
    std::vector<std::pair<std::string,
                          std::chrono::system_clock::time_point>> tmpVector;
    tmpVector.push_back(tmpPair);
    mapFileNameFunctionNameToVectorTimingInfo[keyDebugPerformanceToErase] = tmpVector;
  } else {
    vectorTimingInfoIt->second.push_back(tmpPair);
  }

}

void DebugTrace::PrintPerformanceInfos()
{
  std::string output = "";

}

void DebugTrace::ActiveTrace(bool activate) {
  traceActive = activate;
}

bool DebugTrace::IsTraceActive() {
  return traceActive;
}

std::string DebugTrace::getSpaces() {
  if(debugPrintDeepness > 1) {
    return std::string(2 * (debugPrintDeepness - 1), ' ');
  }
  return "";
}

void DebugTrace::PrintString(const std::string& inStr, bool showHierarchy) {
  std::string str;
  if(showHierarchy) {
    str = DebugTrace::getSpaces() + inStr;
  } else {
    str = inStr;
  }
#ifdef USE_QT_DEBUG
  qDebug() << QString::fromUtf8(str.c_str());
#else
  std::cout << str << std::endl;
#endif
}

std::string DebugTrace::GetPerformanceResults() {
  static auto firstAccess = std::chrono::steady_clock::now();
  auto now = std::chrono::steady_clock::now();
  auto performanceInfos = mapFileNameFunctionNameToVectorTimingInfo[keyDebugPerformanceToErase];

  std::string tmp = lineHeader + " (Full elapsed: " +
      std::to_string(std::chrono::duration <double, std::milli> (now - firstAccess).count()) +
      std::string(UNIT_TRACE_DEBUG) + ")";
  auto size = performanceInfos.size() - 1;
  if(size > 0) {
    for(decltype(size) index = 0; index < size; ++index)
    {
      const auto& valueMin = performanceInfos[index];
      const auto& valueMax = performanceInfos[index + 1];
      if(!tmp.empty())
      {
        tmp += ", ";
      }
      tmp += valueMax.first + " - " + valueMin.first + " = ";
      tmp +=
          std::to_string(std::chrono::duration <double, std::milli> (
                           valueMax.second - valueMin.second).count()) + std::string(UNIT_TRACE_DEBUG);
    }
    if(size > 1)
    {
      const auto& valueMin = performanceInfos[0];
      const auto& valueMax = performanceInfos[size];
      tmp += ", Full time: " + std::to_string(std::chrono::duration <double, std::milli> (
                           valueMax.second - valueMin.second).count()) + std::string(UNIT_TRACE_DEBUG);
    }
  } else {
    tmp += "Not enough trace to display results.";
  }
  return tmp;
}

//cl /EHsc TraceDebug.cpp
//#define TRACE_DEBUG_HPP_DEBUG_LOCAL
#ifdef TRACE_DEBUG_HPP_DEBUG_LOCAL
void main()
{
  START_TRACE_PERFORMANCE(testJP);
  auto val1 = std::chrono::steady_clock::now();
  std::cout << "Hello, world!" << std::endl;
  ADD_TRACE_PERFORMANCE(testJP, testJP1Val1);
  std::cout << "Hello, world 2!" << std::endl;
  auto val2 = std::chrono::steady_clock::now();
  std::cout << "val 2 - val1 = " << std::chrono::duration <double, std::milli> (
                           val2 - val1).count() << std::endl;
}
#endif
#endif
