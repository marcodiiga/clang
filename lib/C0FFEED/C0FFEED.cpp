#include "clang/C0FFEED/C0FFEED.h"
using namespace clang;

StreamHelper C0FFEED::operator() (void) {
  return StreamHelper (GENERIC, *this, &C0FFEED::cannibalizeEvent);
}