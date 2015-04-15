#include "clang/C0FFEED/C0FFEED.h"

StreamHelper C0FFEED::operator()(void) { // TODO: specialize this, perhaps with operator<Parser>() and operator<Sema>()
  return StreamHelper(*this, &C0FFEED::parserEvent);  // or even better called C0FFEE(this) << ... where operator(T) (just trust RVO here)
}