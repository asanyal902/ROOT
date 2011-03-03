//------------------------------------------------------------------------------
// CLING - the C++ LLVM-based InterpreterG :)
// version: $Id: ASTTransformVisitor.h 36608 2010-11-11 18:21:02Z vvassilev $
// author:  Vassil Vassilev <vasil.georgiev.vasilev@cern.ch>
//------------------------------------------------------------------------------

#ifndef CLING_AST_TRANFORM_VISITOR_H
#define CLING_AST_TRANFORM_VISITOR_H

#include "llvm/Support/MemoryBuffer.h"
#include "clang/AST/DeclVisitor.h"
#include "clang/AST/StmtVisitor.h"
#include "clang/AST/DeclTemplate.h"
#include "clang/AST/Stmt.h"
#include "clang/Sema/Sema.h"
#include "clang/Sema/SemaInternal.h"
#include "clang/Sema/Template.h"
#include "clang/Sema/ExternalSemaSource.h"

#include "EvalInfo.h"

namespace clang {
   class Decl;
}

namespace cling {
   class Interpreter;
   typedef llvm::DenseMap<clang::Stmt*, clang::Stmt*> MapTy;

   // Ideally the visitor should traverse the dependent nodes, which actially are 
   // the language extensions. For example File::Open("MyFile"); h->Draw() is not valid C++ call
   // if h->Draw() is defined in MyFile. In that case we need to skip Sema diagnostics, so the 
   // h->Draw() is marked as dependent node. That requires the ASTTransformVisitor to find all
   // dependent nodes and escape them to the interpreter, using pre-defined Eval function.
   class ASTTransformVisitor : public clang::ExternalSemaSource,
                               public clang::DeclVisitor<ASTTransformVisitor>,
                               public clang::StmtVisitor<ASTTransformVisitor, EvalInfo> {
      
   private: // members
      clang::FunctionDecl* m_EvalDecl;
      MapTy m_SubstSymbolMap;
      /* 
         Specifies the unknown symbol surrounding
         Example: int a; ...; h->Draw(a); -> Eval(gCling, "*(int*)@", {&a});
         m_EvalExpressionBuf holds the types of the variables.
         m_Environment holds the refs from which runtime addresses are built.
      */
      std::string m_EvalExpressionBuf;
      llvm::SmallVector<clang::DeclRefExpr*, 64> m_Environment;
      llvm::SmallVector<clang::Decl*, 8> m_FakeDecls;
      clang::DeclContext* m_CurDeclContext; // We need it for Evaluate()
      clang::QualType m_DeclContextType; // Used for building Eval args
   public: // members
      clang::Sema* SemaPtr;
      
   public: // types
      
      typedef clang::DeclVisitor<ASTTransformVisitor> BaseDeclVisitor;
      typedef clang::StmtVisitor<ASTTransformVisitor, EvalInfo> BaseStmtVisitor;

      using BaseStmtVisitor::Visit;

   public:
      
      //Constructors
      ASTTransformVisitor();      
      ASTTransformVisitor(clang::Sema* SemaPtr);
      
      // Destructors
      ~ASTTransformVisitor() { }

      void Initialize();
      clang::FunctionDecl *getEvalDecl() { return m_EvalDecl; }
      void setEvalDecl(clang::FunctionDecl *FDecl) { if (!m_EvalDecl) m_EvalDecl = FDecl; }
      MapTy &getSubstSymbolMap() { return m_SubstSymbolMap; }
      void RemoveFakeDecls();
      
      // ExternalSemaSource
      virtual bool LookupUnqualified(clang::LookupResult &R, clang::Scope *S);
      
      // DeclVisitor      
      void Visit(clang::Decl *D);
      void VisitFunctionDecl(clang::FunctionDecl *D);
      void VisitTemplateDecl(clang::TemplateDecl *D); 
      void VisitDecl(clang::Decl *D);
      void VisitDeclContext(clang::DeclContext *DC);

      // StmtVisitor
      EvalInfo VisitStmt(clang::Stmt *Node);
      EvalInfo VisitExpr(clang::Expr *Node);
      EvalInfo VisitCallExpr(clang::CallExpr *E);
      EvalInfo VisitDeclRefExpr(clang::DeclRefExpr *DRE);
      EvalInfo VisitDependentScopeDeclRefExpr(clang::DependentScopeDeclRefExpr *Node);

      // EvalBuilder
      clang::Expr *SubstituteUnknownSymbol(const clang::QualType InstTy, clang::Expr *SubTree);
      clang::CallExpr *BuildEvalCallExpr(clang::QualType type, clang::Expr *SubTree, clang::ASTOwningVector<clang::Expr*> &CallArgs);
      void BuildEvalEnvironment(clang::Expr *SubTree);
      void BuildEvalArgs(clang::ASTOwningVector<clang::Expr*> &Result);
      clang::Expr *BuildEvalArg0(clang::ASTContext &C);
      clang::Expr *BuildEvalArg1(clang::ASTContext &C);
      clang::Expr *BuildEvalArg2(clang::ASTContext &C);

      // Helper
      bool IsArtificiallyDependent(clang::Expr *Node);
      bool ShouldVisit(clang::Decl *D);      
   };
   
} // namespace cling

#endif
