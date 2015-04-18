// C0FFEED main header
#ifndef LLVM_CLANG_C0FFEED_C0FFEED_H_
#define LLVM_CLANG_C0FFEED_C0FFEED_H_

#include "clang/Basic/IdentifierTable.h"
#include "clang/Basic/LLVM.h"
#include "clang/Parse/Parser.h"
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
    StringRef fileBuf;
    C0phase phase;

    StreamEvent(C0phase p) : phase(p) {
      linfo = { -1, -1 };
    };
  public:
    StreamEvent(StreamEvent&& other) {
      ss = std::move(other.ss);
      linfo = other.linfo;
      phase = other.phase;
      fileBuf = other.fileBuf;
    }

    void prettyPrint();
  };

  class StreamHelper;

  // Main module class
  class C0FFEED : public RefCountedBase<C0FFEED> {
    friend class StreamHelper;
    std::vector <StreamEvent> evts;
    void cannibalizeEvent(StreamEvent&& evt) {
      evts.emplace_back(std::move(evt));
      evts.back().prettyPrint();      
    }
    const SourceManager& sourceMgr;
  public:

    C0FFEED (const SourceManager& smgr) : sourceMgr(smgr) {}

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

    template<>
    StreamHelper& operator<< <Token&> (Token& tok) {
      SourceLocation sl = tok.getLocation();
      auto decLoc = C0F.sourceMgr.getDecomposedLoc(sl);
      fileBuf = C0F.sourceMgr.getBufferData (decLoc.first);

      // linfo was { decLoc.second (the loc), tok.getLength()} but on an annotation token you can't
      // do a getLength

      //Annotation tokens
      //  Although the above is enough to understand how Clang approaches the problem, I want to mention another trick it uses to make parsing more efficient in some cases.

      //  The Sema::getTypeName method mentioned earlier can be costly.It performs a lookup in a set of nested scopes, which may be expensive if the scopes are deeply nested and a name is not actually a type(which is probably most often the case).It's alright (and inevitable!) to do this lookup once, but Clang would like to avoid repeating it for the same token when it backtracks trying to parse a statement in a different way.

      //  A word on what "backtracks" means in this context.Recursive descent parsers are naturally(by their very structure) backtracking.That is, they may try a number of different ways to parse a single grammatical production(be that a statement, an expression, a declaration, or whatever), before finding an approach that succeeds.In this process, the same token may need to be queried more than once.

      //  To avoid this, Clang has special "annotation tokens" it inserts into the token stream.The mechanism is used for other things as well, but in our case we're interested in the tok::annot_typename token. What happens is that the first time the parser encounters a tok::identifier and figures out it's a type, this token gets replaced by tok::annot_typename.The next time the parser encounters this token, it won't have to lookup whether it's a type once again, because it's no longer a generic tok::identifier [3].

      // TODO: find a way to retrieve the original token

      LineInfo lInfo = { 
        decLoc.second        
      };
      int length = 0;
      if (tok.isAnnotation() == true) {
        auto endLoc = tok.getAnnotationEndLoc();
        auto annDecLoc = C0F.sourceMgr.getDecomposedLoc(endLoc);
        lInfo.pos = annDecLoc.second; // TODO - annotations problem
        lInfo.selcount = annDecLoc.second;
      }
      else
        length = tok.getLength();
      lInfo.selcount = length;
      linfo = lInfo;
      return *this;
    }

    ~StreamHelper() {
      (C0F.*callback)(std::move(*this));
    }
  };

} // end namespace clang

#endif