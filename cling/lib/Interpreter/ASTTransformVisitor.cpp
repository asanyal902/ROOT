//------------------------------------------------------------------------------
// CLING - the C++ LLVM-based InterpreterG :)
// version: $Id: ASTTransformVisitor.cpp 36608 2010-11-11 18:21:02Z vvassilev $
// author:  Vassil Vassilev <vasil.georgiev.vasilev@cern.ch>
//------------------------------------------------------------------------------

#include "ASTTransformVisitor.h"
#include "cling/Interpreter/Interpreter.h"

#include "llvm/ADT/SmallVector.h"
#include "clang/AST/DeclarationName.h"
#include "clang/Sema/Scope.h"
#include "clang/Sema/Lookup.h"

using namespace clang;

namespace llvm {
   class raw_string_ostream;
}

namespace {
 
   class StmtPrinterHelper : public PrinterHelper  {
   private:
      PrintingPolicy m_Policy;
      llvm::SmallVector<DeclRefExpr*, 64> &m_Environment;
   public:
      
      StmtPrinterHelper(const PrintingPolicy &Policy, llvm::SmallVector<DeclRefExpr*, 64> &Environment) : 
         m_Policy(Policy), m_Environment(Environment) {}
      
      virtual ~StmtPrinterHelper() {}
      

      // Handle only DeclRefExprs since they are local and the call wrapper
      // won't "see" them. Consequently we don't need to handle:
      // * DependentScopeDeclRefExpr
      // * CallExpr
      // * MemberExpr
      // * CXXDependentScopeMemberExpr
      virtual bool handledStmt(Stmt* S, llvm::raw_ostream& OS) {
         if (DeclRefExpr *Node = dyn_cast<DeclRefExpr>(S))
            // Exclude the artificially dependent DeclRefExprs, created by the Lookup
            if (!Node->isTypeDependent()) {
               if (NestedNameSpecifier *Qualifier = Node->getQualifier())
                  Qualifier->print(OS, m_Policy);
               m_Environment.push_back(Node);
               OS << "*("; 
               // Copy-paste from the StmtPrinter
               QualType T = Node->getType();
               SplitQualType T_split = T.split();
               OS << QualType::getAsString(T_split);

               if (!T.isNull()) {
                  // If the type is sugared, also dump a (shallow) desugared type.
                  SplitQualType D_split = T.getSplitDesugaredType();
                  if (T_split != D_split)
                     OS << ":" << QualType::getAsString(D_split);
               }
               // end
               
               OS <<"*)@";
               
               if (Node->hasExplicitTemplateArgs())
                  OS << TemplateSpecializationType::PrintTemplateArgumentList(
                                                                              Node->getTemplateArgs(),
                                                                              Node->getNumTemplateArgs(),
                                                                              m_Policy);  
               if (Node->hasExplicitTemplateArgs())
               assert((Node->getTemplateArgs() || Node->getNumTemplateArgs()) && "There shouldn't be template paramlist");
               
               return true;            
            }
         
         return false;
      }
   };
} // end anonymous namespace


namespace cling {
   // Constructors
   ASTTransformVisitor::ASTTransformVisitor(Interpreter* Interp, Sema* SemaPtr)
      : m_EvalDecl(0), m_CurDeclContext(0), m_Interpreter(Interp), SemaPtr(SemaPtr) {
   }

   ASTTransformVisitor::ASTTransformVisitor(): m_EvalDecl(0), m_CurDeclContext(0), m_Interpreter(0), SemaPtr(0){
   }
   
   void ASTTransformVisitor::Initialize() {
      m_DeclContextType = m_Interpreter->getQualType("clang::DeclContext");      
   }
   
   // DynamicLookupSource
   bool ASTTransformVisitor::LookupUnqualified(LookupResult &R, Scope *S) {
      if (R.getLookupKind() != Sema::LookupOrdinaryName) return false;
      if (R.isForRedeclaration()) return false;
      DeclarationName Name = R.getLookupName();
      IdentifierInfo *II = Name.getAsIdentifierInfo();
      SourceLocation NameLoc = R.getNameLoc();
      FunctionDecl *D = dyn_cast<FunctionDecl>(R.getSema().ImplicitlyDefineFunction(NameLoc, *II, S));
      if (D) { 
         BuiltinType *Ty = new BuiltinType(BuiltinType::Dependent);
         QualType QTy(Ty, 0);            
         D->setType(QTy);
         R.addDecl(D);
         // Mark this declaration for removal
         m_FakeDecls.push_back(D);
         
         // Say that we can handle the situation. Clang should try to recover
         return true;
      }
      // We cannot handle the situation. Give up
      return false;              
   }

   // DeclVisitor
   
   void ASTTransformVisitor::Visit(Decl *D) {
      //Decl *PrevDecl = ASTTransformVisitor::CurrentDecl;
      //ASTTransformVisitor::CurrentDecl = D;
      BaseDeclVisitor::Visit(D);
      //ASTTransformVisitor::CurrentDecl = PrevDecl;     
   }
   
   void ASTTransformVisitor::VisitFunctionDecl(FunctionDecl *D) {
      if (!D->isDependentContext() && D->isThisDeclarationADefinition()) {
         Stmt *Old = D->getBody();
         Stmt *New = Visit(Old).getNewStmt();
         if (Old != New)
            D->setBody(New);
      }
   }
 
   void ASTTransformVisitor::VisitTemplateDecl(TemplateDecl *D) {     
      if (D->getNameAsString().compare("Eval") == 0) {
         CXXRecordDecl *CXX = dyn_cast<CXXRecordDecl>(D->getDeclContext());
         if (CXX && CXX->getNameAsString().compare("Interpreter") == 0) {  
            NamespaceDecl *ND = dyn_cast<NamespaceDecl>(CXX->getDeclContext());
            if (ND && ND->getNameAsString().compare("cling") == 0) {
               if (FunctionDecl *FDecl = dyn_cast<FunctionDecl>(D->getTemplatedDecl()))
                  setEvalDecl(FDecl);
            }
         }
      }
    }
  
   void ASTTransformVisitor::VisitDecl(Decl *D) {
      if (!ShouldVisit(D))
         return;
      
      if (DeclContext *DC = dyn_cast<DeclContext>(D))
         if (!(DC->isDependentContext()))
            static_cast<ASTTransformVisitor*>(this)->VisitDeclContext(DC);
   }
   
   void ASTTransformVisitor::VisitDeclContext(DeclContext *DC) {
      m_CurDeclContext = DC;
      for (DeclContext::decl_iterator
              I = DC->decls_begin(), E = DC->decls_end(); I != E; ++I)        
         if (ShouldVisit(*I))
            Visit(*I);
   }
   
   // end DeclVisitor
   
   // StmtVisitor   

   EvalInfo ASTTransformVisitor::VisitStmt(Stmt *Node) {
      for (Stmt::child_iterator
              I = Node->child_begin(), E = Node->child_end(); I != E; ++I) {
         if (*I) {
            EvalInfo EInfo = Visit(*I);
            if (EInfo.IsEvalNeeded) {
               if (Expr *E = dyn_cast<Expr>(EInfo.getNewStmt()))
                  // Assume void if still not escaped
                  *I = SubstituteUnknownSymbol(SemaPtr->getASTContext().VoidTy, E);
            } 
            else {
               *I = EInfo.getNewStmt();
            }
         }
      }
      
      return EvalInfo(Node, 0);
   }
   
   EvalInfo ASTTransformVisitor::VisitExpr(Expr *Node) {
      for (Stmt::child_iterator
              I = Node->child_begin(), E = Node->child_end(); I != E; ++I) {
         if (*I) {
            EvalInfo EInfo = Visit(*I);
            if (EInfo.IsEvalNeeded) {
               if (Expr *E = dyn_cast<Expr>(EInfo.getNewStmt()))
                  // Assume void if still not escaped
                  *I = SubstituteUnknownSymbol(SemaPtr->getASTContext().VoidTy, E);
            } 
            else {
               *I = EInfo.getNewStmt();
            }
         }
      }
      return EvalInfo(Node, 0);
   }

   EvalInfo ASTTransformVisitor::VisitCallExpr(CallExpr *E) {
      // FIXME: Maybe we need to handle the arguments
      // EvalInfo EInfo = Visit(E->getCallee());
      return EvalInfo (E, IsArtificiallyDependent(E));
   }
      
   EvalInfo ASTTransformVisitor::VisitDeclRefExpr(DeclRefExpr *DRE) {
      return EvalInfo(DRE, IsArtificiallyDependent(DRE));
   }
      
   EvalInfo ASTTransformVisitor::VisitDependentScopeDeclRefExpr(DependentScopeDeclRefExpr *Node) {
      return EvalInfo(Node, IsArtificiallyDependent(Node));
   }
   
   // end StmtVisitor

   // EvalBuilder

   Expr *ASTTransformVisitor::SubstituteUnknownSymbol(const QualType InstTy, Expr *SubTree) {
      // Get the addresses
      BuildEvalEnvironment(SubTree);

      //Build the arguments for the call
      ASTOwningVector<Expr*> CallArgs(*SemaPtr);
      BuildEvalArgs(CallArgs);

      // Build the call
      CallExpr* EvalCall = BuildEvalCallExpr(InstTy, SubTree, CallArgs);

      // Add substitution mapping
      getSubstSymbolMap()[EvalCall] = SubTree;      

      return EvalCall;
   }

   // Creates the string, which is going to be escaped.
   void ASTTransformVisitor::BuildEvalEnvironment(Expr *SubTree) {
      m_EvalExpressionBuf = "";
      llvm::raw_string_ostream OS(m_EvalExpressionBuf);
      const PrintingPolicy &Policy = SemaPtr->getASTContext().PrintingPolicy;
      
      StmtPrinterHelper *helper = new StmtPrinterHelper(Policy, m_Environment);      
      SubTree->printPretty(OS, helper, Policy);
      
      OS.flush();
   }
   
   // Prepare the actual arguments for the call
   // Arg list for static T Eval(size_t This, const char* expr, void* varaddr[] )
   void ASTTransformVisitor::BuildEvalArgs(ASTOwningVector<Expr*> &Result) {
      ASTContext &C = SemaPtr->getASTContext();

      // Arg 0:
      Expr *Arg0 = BuildEvalArg0(C);
      Result.push_back(Arg0);

      // Arg 1:
      Expr *Arg1 = BuildEvalArg1(C);    
      Result.push_back(Arg1);

      // Arg 2:
      Expr *Arg2 = BuildEvalArg2(C);          
      Result.push_back(Arg2);

      // Arg 3:
      Expr *Arg3 = BuildEvalArg3(C);          
      Result.push_back(Arg3);

   }
   
   // Eval Arg0: size_t This
   Expr *ASTTransformVisitor::BuildEvalArg0(ASTContext &C) {
      const llvm::APInt gClingAddr(8 * sizeof(void *), (uint64_t)m_Interpreter);
      IntegerLiteral *Arg0 = IntegerLiteral::Create(C, gClingAddr, C.UnsignedLongTy, SourceLocation());

      return Arg0;
   }

   // Eval Arg1: const char* expr
   Expr *ASTTransformVisitor::BuildEvalArg1(ASTContext &C) {
      const QualType ConstChar = C.getConstType(C.CharTy);
      const QualType ConstCharArray = C.getConstantArrayType(ConstChar, llvm::APInt(C.getTypeSize(ConstChar), m_EvalExpressionBuf.length() + 1), ArrayType::Normal, /*IndexTypeQuals=*/ 0);
      Expr *Arg1 = StringLiteral::Create(C, &*m_EvalExpressionBuf.c_str(), m_EvalExpressionBuf.length(), /*Wide=*/ false, ConstCharArray, SourceLocation());
      const QualType CastTo = C.getPointerType(C.getConstType(C.CharTy));
      SemaPtr->ImpCastExprToType(Arg1, CastTo, CK_ArrayToPointerDecay);

      return Arg1;
   }

   // Eval Arg2: void* varaddr[]
   Expr *ASTTransformVisitor::BuildEvalArg2(ASTContext &C) {
      QualType VarAddrTy = SemaPtr->BuildArrayType(C.VoidPtrTy, 
                                                   ArrayType::Normal,
                                                   /*ArraySize*/0
                                                   , Qualifiers()
                                                   , SourceRange()
                                                   , DeclarationName() );

      ASTOwningVector<Expr*> Inits(*SemaPtr);
      for (unsigned int i = 0; i < m_Environment.size(); ++i) {
         Expr *UnOp = SemaPtr->BuildUnaryOp(SemaPtr->getScopeForContext(SemaPtr->CurContext), 
                               SourceLocation(), 
                               UO_AddrOf,
                               m_Environment[i]).takeAs<UnaryOperator>();
         SemaPtr->ImpCastExprToType(UnOp, C.getPointerType(C.VoidPtrTy), CK_BitCast);
         Inits.push_back(UnOp);
      }

      // We need fake the SourceLocation just to avoid assert(InitList.isExplicit()....)
      SourceLocation SLoc = getEvalDecl()->getLocStart();
      SourceLocation ELoc = getEvalDecl()->getLocEnd();
      InitListExpr *ILE = SemaPtr->ActOnInitList(SLoc, move_arg(Inits), ELoc).takeAs<InitListExpr>();
      Expr *Arg2 = SemaPtr->BuildCompoundLiteralExpr(SourceLocation(), C.CreateTypeSourceInfo(VarAddrTy), SourceLocation(), ILE).takeAs<CompoundLiteralExpr>();
      SemaPtr->ImpCastExprToType(Arg2, C.getPointerType(C.VoidPtrTy), CK_ArrayToPointerDecay);

      return Arg2;
   }

   // Eval Arg3: DeclContext* DC
   Expr *ASTTransformVisitor::BuildEvalArg3(ASTContext &C) {

      //if (m_LexedDeclContextType == 0)
      
      const llvm::APInt DCAddr(8 * sizeof(void *), (uint64_t)m_CurDeclContext);
      
      Expr *Arg3 = IntegerLiteral::Create(C, DCAddr, C.UnsignedLongTy, SourceLocation());
      //TypeSourceInfo *TSI = C.CreateTypeSourceInfo(m_LexedDeclContextType);
      //Arg3 = SemaPtr->BuildCStyleCastExpr(SourceLocation(), TSI, SourceLocation(), Arg3).takeAs<Expr>();
      SemaPtr->ImpCastExprToType(Arg3, m_DeclContextType, CK_IntegralToPointer);

      return Arg3;
   }

   // Here is the test Eval function specialization. Here the CallExpr to the function
   // is created.
   CallExpr *ASTTransformVisitor::BuildEvalCallExpr(const QualType InstTy, Expr *SubTree, 
                                                    ASTOwningVector<Expr*> &CallArgs) {      
      // Set up new context for the new FunctionDecl
      DeclContext *PrevContext = SemaPtr->CurContext;
      FunctionDecl *FDecl = getEvalDecl();
      
      assert(FDecl && "The Eval function not found!");

      SemaPtr->CurContext = FDecl->getDeclContext();
      
      // Create template arguments
      Sema::InstantiatingTemplate Inst(*SemaPtr, SourceLocation(), FDecl);
      TemplateArgument Arg(InstTy);
      TemplateArgumentList TemplateArgs(TemplateArgumentList::OnStack, &Arg, 1U);
      
      // Substitute the declaration of the templated function, with the 
      // specified template argument
      Decl *D = SemaPtr->SubstDecl(FDecl, FDecl->getDeclContext(), MultiLevelTemplateArgumentList(TemplateArgs));
      
      FunctionDecl *Fn = dyn_cast<FunctionDecl>(D);
      // Creates new body of the substituted declaration
      SemaPtr->InstantiateFunctionDefinition(Fn->getLocation(), Fn, true, true);
      
      SemaPtr->CurContext = PrevContext;                            
      
      const FunctionProtoType *Proto = Fn->getType()->getAs<FunctionProtoType>();

      //Walk the params and prepare them for building a new function type
      llvm::SmallVectorImpl<QualType> ParamTypes(FDecl->getNumParams());
      for (FunctionDecl::param_iterator P = FDecl->param_begin(), PEnd = FDecl->param_end();
           P != PEnd;
           ++P) {
         ParamTypes.push_back((*P)->getType());
         
      }
      
      // Build function type, needed by BuildDeclRefExpr 
      QualType FuncT = SemaPtr->BuildFunctionType(Fn->getResultType()
                                                  , ParamTypes.data()
                                                  , ParamTypes.size()
                                                  , Proto->isVariadic()
                                                  , Proto->getTypeQuals()
                                                  , RQ_None
                                                  , Fn->getLocation()
                                                  , Fn->getDeclName()
                                                  , Proto->getExtInfo());                  
      
      DeclRefExpr *DRE = SemaPtr->BuildDeclRefExpr(Fn, FuncT, VK_RValue, SourceLocation()).takeAs<DeclRefExpr>();
      
      CallExpr *EvalCall = SemaPtr->ActOnCallExpr(SemaPtr->getScopeForContext(SemaPtr->CurContext)
                                                  , DRE
                                                  , SourceLocation()
                                                  , move_arg(CallArgs)
                                                  , SourceLocation()
                                                  ).takeAs<CallExpr>();
      assert (EvalCall && "Cannot create call to Eval");
      return EvalCall;                  
      
   } 

   // end EvalBuilder
   
   // Helpers

   // Removes the implicitly created functions, which help to emulate the dynamic scopes
   void ASTTransformVisitor::RemoveFakeDecls() {      
      Scope *S = SemaPtr->getScopeForContext(SemaPtr->getASTContext().getTranslationUnitDecl());
      for (unsigned int i = 0; i < m_FakeDecls.size(); ++i) {
         S->RemoveDecl(m_FakeDecls[i]);
      }
   }

   bool ASTTransformVisitor::ShouldVisit(Decl *D) {
      while (true) {
         if (isa<TemplateTemplateParmDecl>(D))
            return false;
         if (isa<ClassTemplateDecl>(D))
            return false;
         if (isa<FriendTemplateDecl>(D))
            return false;
         if (isa<ClassTemplatePartialSpecializationDecl>(D))
            return false;
         if (CXXRecordDecl *CXX = dyn_cast<CXXRecordDecl>(D)) {
            if (CXX->getDescribedClassTemplate())
               return false;
         }
         if (CXXMethodDecl *CXX = dyn_cast<CXXMethodDecl>(D)) {
            if (CXX->getDescribedFunctionTemplate())
               return false;
         }
         if (isa<TranslationUnitDecl>(D)) {
            break;
         }
         
         if (DeclContext* DC = D->getDeclContext())
            if (!(D = dyn_cast<Decl>(DC)))
                break;
      }
      
      return true;
   }

   bool ASTTransformVisitor::IsArtificiallyDependent(Expr *Node) {
      if (!Node->isValueDependent() || !Node->isTypeDependent())
          return false;     
      return true;
   }
   // end Helpers   

}//end cling
