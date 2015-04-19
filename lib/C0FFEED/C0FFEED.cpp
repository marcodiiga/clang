#include "clang/C0FFEED/C0FFEED.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/PrettyPrinter.h"
#include <iostream>
#include <utility>
using namespace clang;

void AnnotationData::print() {
  std::cout << statement << std::endl;
}

void TokenData::print() { // TODO: line count would be nice
  std::stringstream ss;
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

void ExpressionData::print() {
  std::stringstream ss;
  ss << scope << " " << expression;
  std::cout << ss.str() << std::endl;
}

void StreamEvent::prettyPrint() {

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
  ss << this->ss.str();
  std::cout << ss.str() << std::endl;

  data->print(); // Data-specific print
}

StreamHelper C0FFEED::operator() (void) {
  return StreamHelper (GENERIC, *this, &C0FFEED::cannibalizeEvent);
}

template<>
StreamHelper& StreamHelper::operator << <C0phase> (C0phase&& p) {
  this->phase = p;
  return *this;
}

namespace {
  std::string exprPrintPretty(const ASTContext& astCnt, const Expr& ex) {
    std::string statement;
    llvm::raw_string_ostream oss(statement);
    ex.printPretty(oss, nullptr, PrintingPolicy(astCnt.getLangOpts()));
    oss.flush();
    return statement;
  }
}

template<>
StreamHelper& StreamHelper::operator << <Token&> (Token& tok) {

  if (tok.isAnnotation()) {
    // Annotation: this token was already parsed (an annotation is an identified placeholder)
    switch (tok.getKind()) { // See Parser::ParseCastExpression for the full list
      case tok::annot_primary_expr: {
        auto exprRes = Parser::getExprAnnotation(tok);
        auto expr = exprRes.get();
        std::string statement = exprPrintPretty(C0F.getASTContext(), *expr);
        data = std::make_unique<AnnotationData>(statement);
      } break;
      default:
        break;
    };
  } else {
    // Normal token
    SourceLocation sl = tok.getLocation();
    auto decLoc = C0F.getSourceManager().getDecomposedLoc(sl);
    LineInfo li{ decLoc.second, tok.getLength() };
    llvm::StringRef fb = C0F.getSourceManager().getBufferData(decLoc.first);
    data = std::make_unique<TokenData>(li, fb);
  }

  return *this;
}

template<>
StreamHelper& StreamHelper::operator << <Scope&> (Scope& scope) {
  
  std::stringstream ssFlags;
  ssFlags << "Scope: [";
  unsigned int flags = scope.getFlags();

#define CHECKANDSTREAM(X,F) do { \
  if ((X & F) == F)              \
    ssFlags << " " << #F;        \
  } while(0)

  using FL = Scope::ScopeFlags;
  CHECKANDSTREAM (flags, FL::FnScope);
  CHECKANDSTREAM (flags, FL::BreakScope);
  CHECKANDSTREAM (flags, FL::ContinueScope);
  CHECKANDSTREAM (flags, FL::DeclScope);
  CHECKANDSTREAM (flags, FL::ControlScope);
  CHECKANDSTREAM (flags, FL::ClassScope);
  CHECKANDSTREAM (flags, FL::BlockScope);
  CHECKANDSTREAM (flags, FL::TemplateParamScope);
  CHECKANDSTREAM (flags, FL::FunctionPrototypeScope);
  CHECKANDSTREAM (flags, FL::FunctionDeclarationScope);
  CHECKANDSTREAM (flags, FL::SwitchScope);
  CHECKANDSTREAM (flags, FL::TryScope);
  CHECKANDSTREAM (flags, FL::FnTryCatchScope);
  CHECKANDSTREAM (flags, FL::EnumScope);
  CHECKANDSTREAM (flags, FL::SEHTryScope);
  CHECKANDSTREAM (flags, FL::SEHExceptScope);
  CHECKANDSTREAM (flags, FL::SEHFilterScope);  
  
  ssFlags << " ]";
  auto ptr = data.get();
  if (ptr == nullptr) {
    data = std::make_unique<ExpressionData>();
  }
  static_cast<ExpressionData*>(data.get())->setScope(ssFlags.str());
  return *this;
}

template<>
StreamHelper& StreamHelper::operator << <Expr&> (Expr& expr) {
  std::string expression = exprPrintPretty(C0F.getASTContext(), expr);
  auto ptr = data.get();
  if (ptr == nullptr) {
    data = std::make_unique<ExpressionData>();
  }
  std::stringstream ex;
  ex << "Expression: " << expression;
  static_cast<ExpressionData*>(data.get())->setExpression(ex.str());
  return *this;
}