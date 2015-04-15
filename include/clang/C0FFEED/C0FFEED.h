// C0FFEED main header
#ifndef LLVM_CLANG_C0FFEED_C0FFEED_H_
#define LLVM_CLANG_C0FFEED_C0FFEED_H_

#include "clang/Basic/LLVM.h"
#include "llvm/ADT/IntrusiveRefCntPtr.h"
#include <utility>
#include <sstream>

namespace clang {

    class StreamHelper;

    class C0FFEED : public RefCountedBase<C0FFEED> {
        friend class StreamHelper;
        void parserEvent(std::stringstream&& ss) {
            printf("%s", ss.str()); // TODO: do something sane
        }
    public:

        ~C0FFEED() = default;

        StreamHelper operator()(void);
    };

    class StreamHelper {
        std::stringstream ss;
        C0FFEED &C0F;
        void (C0FFEED::*callback)(std::stringstream&&);
    public:

        explicit StreamHelper(C0FFEED& c0f, void (C0FFEED::*cb)(std::stringstream&&)) : C0F(c0f), callback(cb) {}

        //StreamHelper(const StreamHelper& other) : C0F(other.C0F) {
        //   this->ss = std::move(other.ss);
        //}

        template<class T>
        StreamHelper& operator<< (T&& obj) {
            ss << obj;
            return *this;
        }
        ~StreamHelper() {
            (C0F.*callback)(std::move(ss));
        }
    };

} // end namespace clang

#endif