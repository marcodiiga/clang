// C0FFEED main header
#ifndef LLVM_CLANG_C0FFEED_C0FFEED_H_
#define LLVM_CLANG_C0FFEED_C0FFEED_H_

#include "clang/AST/Expr.h"
#include "clang/Basic/IdentifierTable.h"
#include "clang/Basic/LLVM.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Parse/Parser.h"
#include "clang/Sema/Scope.h"
#include "llvm/ADT/IntrusiveRefCntPtr.h"
#include <utility>
#include <vector>
#include <sstream>

namespace clang {

  enum C0phase {GENERIC, PARSE, SEMA};  

  class PrintInfo { // Base class for data objects
  public:
    virtual void print() = 0;
  };

  struct LineInfo {
    unsigned int pos;
    unsigned int selcount;
    LineInfo() : pos(-1), selcount(-1) {}
    LineInfo(unsigned int p, unsigned int s) : pos(p), selcount(s) {}
  };

  class TokenData : public PrintInfo {
  private:
    LineInfo linfo;
    StringRef fileBuf;
  public:
    void print();
    TokenData(LineInfo li, StringRef fb) : linfo(li), fileBuf(fb) {}
  };

  class AnnotationData : public PrintInfo{
  private:
    std::string statement;
  public:
    void print();
    AnnotationData(std::string st) : statement(st) {}
  };

  class ExpressionData : public PrintInfo {
  private:
    std::string scope;
    std::string expression;
  public:
    void print();
    ExpressionData() = default;
    void setScope(std::string sc) { scope = sc; }
    void setExpression(std::string ex) { expression = ex; }
  };

  class C0FFEED;

  // A StreamEvent is an event happened at any phase. It might be
  // identified by a specific phase and additional infos
  class StreamEvent {
    friend class C0FFEED;
  protected:
    
    std::unique_ptr<PrintInfo> data;
    std::stringstream ss;
    C0phase phase;

    StreamEvent(C0phase p) : phase(p) {};
  public:
    StreamEvent(StreamEvent&& other) {
      ss = std::move(other.ss);
      phase = other.phase;
      data.reset(other.data.release());
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
      evts.back().prettyPrint(); // DEBUG: after storing the event this might be removed     
    }
    const CompilerInstance *CI;
    // Getters
    const SourceManager& getSourceManager() const {
      assert(CI != nullptr && "Missing CompilerInstance (ASTUnit support is missing)");
      return CI->getSourceManager();
    }
    const ASTContext& getASTContext() const {
      assert(CI != nullptr && "Missing CompilerInstance (ASTUnit support is missing)");
      return CI->getASTContext();
    }
  public:

    C0FFEED (const CompilerInstance *ci) : CI(ci) {}

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
      : C0F(other.C0F), callback(other.callback), StreamEvent(std::move(other)) {}

    template<class T>
    StreamHelper& operator<< (T&& obj) {
      ss << obj;
      return *this;
    }

    // Notice: due to §14.7.3/3 template member function specialization declarations 
    // can't go in here

    ~StreamHelper() {
      (C0F.*callback)(std::move(*this));
    }
  };

  template<>
  StreamHelper& StreamHelper::operator<< <C0phase> (C0phase&& p);

  template<>
  StreamHelper& StreamHelper::operator<< <Token&> (Token& tok);

  template<>
  StreamHelper& StreamHelper::operator<< <Scope&> (Scope& scope);

  template<>
  StreamHelper& StreamHelper::operator<< <Expr&> (Expr& expr);

} // end namespace clang

#endif