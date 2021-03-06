//===----------------------------------------------------------------------===//
//
//                           The MIT License (MIT)
//                    Copyright (c) 2017 Peter Goldsborough
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//===----------------------------------------------------------------------===//

#ifndef CLANG_EXPAND_COMMON_DEFINITION_REWRITER_HPP
#define CLANG_EXPAND_COMMON_DEFINITION_REWRITER_HPP

// Clang includes
#include <clang/AST/RecursiveASTVisitor.h>

// LLVM includes
#include <llvm/ADT/SmallPtrSet.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/StringMap.h>

// Standard includes
#include <iosfwd>

namespace clang {
class ASTContext;
class Rewriter;
class Stmt;
class SourceLocation;
class MemberExpr;
class ReturnStmt;
}

namespace ClangExpand {
struct CallData;

/// Class to rewrite a function body for expansion purposes.
///
/// This class performs the heavy lifting in terms of rewriting a function body.
/// It rewrites `return` statements to assignments (under certain
/// circumstances), prefixes member expressions with their base objects and most
/// importantly replaces parameter references with the passed argument
/// expressions.
///
/// This class only stores references to the objects it is constructed with. It
/// should therefore not be stored, but used just like a function call with all
/// arguments stored on the stack.
class DefinitionRewriter
    : public clang::RecursiveASTVisitor<DefinitionRewriter> {
 public:
  using super = clang::RecursiveASTVisitor<DefinitionRewriter>;
  using ParameterMap = llvm::StringMap<std::string>;

  /// Constructor.
  explicit DefinitionRewriter(clang::Rewriter& rewriter,
                              const ParameterMap& parameterMap,
                              const CallData& call,
                              clang::ASTContext& context);

  /// Traverses the body of a function to rewrite.
  bool VisitStmt(clang::Stmt* body);

  /// Traversed `clang::TypeLoc`, which we use to replace type template
  /// parameters.
  bool VisitTypeLoc(clang::TypeLoc typeLocation);

  /// Rewrites all `return` statements to assignments, according to the stored
  /// `CallData`. `return` statement locations are stored during the traversal
  /// in `VisitStmt`. After this is done, this method can be called to actually
  /// replace each `return <something>` statement to `<variable> = <something>`.
  ///
  /// There are two required preconditions to calling this method:
  ///
  /// 1. The `assignee` member of the `CallData` must not be nullopt`.
  /// 2. There must be at least one return statement in the body of
  /// the function. This invariant *should* follow from (1), since there
  /// *should* be no assignee if there is no return statement.
  ///
  /// \returns True if it is necessary to prepend a declaration of the assignee
  /// to the function body (depending on the number of return statements), else
  /// false.
  bool rewriteReturnsToAssignments(const clang::Stmt& body);

 private:
  /// Stores the location of a return statement for later use. Once all return
  /// locations have been collected like this, `rewriteReturnsToAssignments` can
  /// later be called to perform the actual replacements.
  void _recordReturn(const clang::ReturnStmt& returnStatement,
                     const CallData& call);

  /// Replaces a single return location with the given text. The location should
  /// probably come out of `_returnLocations`.
  void _rewriteReturn(const clang::SourceLocation& begin,
                      const std::string& replacement);

  /// Handles rewriting a member expression. This is needed when the function
  /// being rewritten is a method. In that case we need to prefix every
  /// reference to a field or method with the base of the function (e.g. the 'x'
  /// in `x.foo()`).
  void _rewriteMemberExpression(const clang::MemberExpr& member);

  void _rewriteNonTypeTemplateParameterExpression(
      const clang::SubstNonTypeTemplateParmExpr& nonType);

  /// A rewriter to mess with the source text.
  clang::Rewriter& _rewriter;

  /// A reference to a parameter map, for replacing parameter uses with argument
  /// expressions.
  const ParameterMap& _parameterMap;

  /// A reference to a `CallData` structure.
  const CallData& _call;

  /// The current `clang::ASTContext`.
  clang::ASTContext& _context;

  /// Stores members we have rewritten, because sometimes they are encountered
  /// twice inside `VisitStmt` (dunno why).
  llvm::SmallPtrSet<const clang::MemberExpr*, 16> _rewrittenMembers;

  /// Stores the locations of return statements (at the 'r') so we can later
  /// rewrite them.
  llvm::SmallVector<clang::SourceLocation, 8> _returnLocations;
};
}  // namespace ClangExpand

#endif  // CLANG_EXPAND_COMMON_DEFINITION_REWRITER_HPP
