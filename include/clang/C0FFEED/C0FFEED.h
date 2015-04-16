// C0FFEED main header
#ifndef LLVM_CLANG_C0FFEED_C0FFEED_H_
#define LLVM_CLANG_C0FFEED_C0FFEED_H_

#include "clang/Basic/LLVM.h"
#include "llvm/ADT/IntrusiveRefCntPtr.h"
#include <utility>
#include <vector>
#include <sstream>

namespace clang {

  enum C0phase {GENERIC, PARSE, SEMA};

  struct LineInfo {
    unsigned int pos;
    unsigned int selcount;
  };

  class C0FFEED;

  // A StreamEvent is an event happened at any phase. It might be
  // identified by a specific phase and additional infos
  class StreamEvent {
    friend class C0FFEED;
  protected:
    std::stringstream ss;
    LineInfo linfo;
    C0phase phase;

    StreamEvent(C0phase p) : phase(p) {};
  public:
    StreamEvent(StreamEvent&& other) {
      ss = std::move(other.ss);
      linfo = other.linfo;
      phase = other.phase;
    }
  };

  class StreamHelper;

  // Main module class
  class C0FFEED : public RefCountedBase<C0FFEED> {
    friend class StreamHelper;
    std::vector <StreamEvent> evts;
    void cannibalizeEvent(StreamEvent&& evt) {
      evts.emplace_back(std::move(evt));
      printf("Event %d: %s", evts.back().phase, evts.back().ss.str().c_str()); // TODO: do something useful
    }

  public:

    StreamHelper operator()(void);
  };

  // A StreamHelper is an enhanced version of a StreamEvent object which
  // supports stream insertions, appropriate overloadings and manages
  // temporary callbacks to cannibalizing/GC functions
  class StreamHelper : public StreamEvent {
    C0FFEED &C0F;
    void (C0FFEED::*callback)(StreamEvent&&);
  public:

    explicit StreamHelper(C0phase p, C0FFEED& c0f, void (C0FFEED::*cb)(StreamEvent&&))
      : StreamEvent(p), C0F(c0f), callback(cb) {}

    StreamHelper(StreamHelper&& other)
      : StreamEvent(other.phase), C0F(other.C0F), callback(other.callback) {
      this->ss = std::move(other.ss);
      this->linfo = other.linfo;
    }

    template<class T>
    StreamHelper& operator<< (T&& obj) {
      ss << obj;
      return *this;
    }

    template<>
    StreamHelper& operator<< <C0phase> (C0phase&& p) {
      this->phase = p;
      return *this;
    }

    ~StreamHelper() {
      (C0F.*callback)(std::move(*this));
    }
  };

} // end namespace clang

#endif