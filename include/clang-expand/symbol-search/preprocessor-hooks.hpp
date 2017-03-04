#ifndef CLANG_EXPAND_SYMBOL_SEARCH_PREPROCESSOR_HOOKS_HPP
#define CLANG_EXPAND_SYMBOL_SEARCH_PREPROCESSOR_HOOKS_HPP

// Project includes
#include "clang-expand/common/structures.hpp"

// Clang includes
#include "clang/Basic/LangOptions.h"
#include "clang/Basic/SourceLocation.h"
#include "clang/Lex/PPCallbacks.h"
#include "clang/Lex/Preprocessor.h"

// Standard includes
#include <functional>
#include <utility>

namespace clang {
class Token;
class MacroDefinition;
class SourceRange;
class MacroArgs;
class MacroInfo;
class SourceManager;
class FileEntry;
class CompilerInstance;
}

namespace ClangExpand {
struct DefinitionState;
}

namespace ClangExpand::SymbolSearch {


struct PreprocessorHooks : public clang::PPCallbacks {
 public:
  using MatchCallback = std::function<void(DefinitionState&&)>;

  PreprocessorHooks(clang::CompilerInstance& compiler,
                    const clang::SourceLocation& location,
                    const MatchCallback& callback);

  void MacroExpands(const clang::Token& macroNameToken,
                    const clang::MacroDefinition& macroDefinition,
                    clang::SourceRange range,
                    const clang::MacroArgs* macroArgs) override;

 private:
  const clang::SourceManager& _sourceManager;
  const clang::LangOptions& _languageOptions;
  clang::Preprocessor& _preprocessor;
  const Structures::CanonicalLocation _callLocation;
  const MatchCallback _callback;
};

}  // namespace ClangExpand::SymbolSearch

#endif  // CLANG_EXPAND_SYMBOL_SEARCH_PREPROCESSOR_HOOKS_HPP