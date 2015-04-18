#include "clang/C0FFEED/C0FFEED.h"
#include <iostream>
#include <utility>
using namespace clang;

void StreamEvent::prettyPrint() {
  //printf("Event %d: %s\n", evts.back().phase, evts.back().ss.str().c_str()); // TODO: do something useful
  /*std::stringstream ss;
  LineInfo linfo;
  StringRef fileBuf;
  C0phase phase;*/

  std::stringstream ss;
  ss << '\n';
  switch (this->phase) {
    case PARSE: {
      ss << "[PARSING] ";
    }break;
    case SEMA: {
      ss << "[SEMANTIC] ";
    }break;
    case GENERIC: {
      ss << "[GENERIC] ";
    }break;
  }
  ss << this->ss.str() << '\n';
  if (fileBuf.data() != nullptr && linfo.pos != -1 && linfo.selcount != -1) {
    auto getLineFromPos = [](StringRef buf, int pos) {
      int start = pos, end = pos;
      while (start >= 0 && buf.data()[start] != '\n') {
        --start;
      }
      while (end < buf.size() && buf.data()[end] != '\n') {
        ++end;
      }
      return std::make_pair<int, int>(start + 1, end - 1);
    };
    auto startEnd = getLineFromPos(fileBuf, linfo.pos);
    StringRef line = fileBuf.substr(startEnd.first, startEnd.second - startEnd.first);
    ss << line.str() << '\n';
    for (int i = 0; i < (int)linfo.pos - startEnd.first; ++i)
      ss << ' ';
    for (int i = 0; i < (int)linfo.selcount; ++i)
      ss << '^';
    for (int i = linfo.pos + linfo.selcount; i <= startEnd.second; ++i)
      ss << ' ';
  }
  ss << '\n';
  std::cout << ss.str() << std::endl;
}

StreamHelper C0FFEED::operator() (void) {
  return StreamHelper (GENERIC, *this, &C0FFEED::cannibalizeEvent);
}