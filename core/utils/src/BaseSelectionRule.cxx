// @(#)root/core/utils:$Id: BaseSelectionRule.cxx 41697 2011-11-01 21:03:41Z pcanal $
// Author: Velislava Spasova September 2010

/*************************************************************************
 * Copyright (C) 1995-2011, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// BaseSelectionRule                                                    //
//                                                                      //
// Base selection class from which all                                  //
// selection classes should be derived                                  //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

#include "BaseSelectionRule.h"

#include <iostream>
#include <string.h>

#include "clang/Basic/SourceLocation.h"
#include "clang/Basic/SourceManager.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/DeclTemplate.h"

#ifdef _WIN32
#include "process.h"
#endif
#include <sys/stat.h>


static const char *R__GetDeclSourceFileName(const clang::Decl* D)
{
   clang::SourceLocation SL = D->getLocation();
   clang::ASTContext& ctx = D->getASTContext();
   clang::SourceManager& SM = ctx.getSourceManager();

   if (SL.isValid() && SL.isFileID()) {
      clang::PresumedLoc PLoc = SM.getPresumedLoc(SL);
      return PLoc.getFilename();
   }
   else {
      return "invalid";
   }   
}

#if MATCH_ON_INSTANTIATION_LOCATION
static const char *R__GetDeclSourceFileName(const clang::ClassTemplateSpecializationDecl *tmpltDecl)
{
   clang::SourceLocation SL = tmpltDecl->getPointOfInstantiation();
   clang::ASTContext& ctx = tmpltDecl->getASTContext();
   clang::SourceManager& SM = ctx.getSourceManager();

   if (SL.isValid() && SL.isFileID()) {
      clang::PresumedLoc PLoc = SM.getPresumedLoc(SL);
      return PLoc.getFilename();
   }
   else {
      return "invalid";
   }   
}
#endif

/******************************************************************
 * R__matchfilename(srcfilename,filename)
 ******************************************************************/
static bool R__match_filename(const char *srcname,const char *filename)
{
   if (srcname==0) {
      return false;
   }
   if((strcmp(srcname,filename)==0)) {
      return true;
   }
   
#ifdef G__WIN32
   G__FastAllocString i1name(_MAX_PATH);
   G__FastAllocString fullfile(_MAX_PATH);
   _fullpath( i1name, srcname, _MAX_PATH );
   _fullpath( fullfile, filename, _MAX_PATH );
   if((stricmp(i1name, fullfile)==0)) return 1;
#else
   struct stat statBufItem;
   struct stat statBuf;
   if (   ( 0 == stat( filename, & statBufItem ) )
       && ( 0 == stat( srcname, & statBuf ) )
       && ( statBufItem.st_dev == statBuf.st_dev )     // Files on same device
       && ( statBufItem.st_ino == statBuf.st_ino )     // Files on same inode (but this is not unique on AFS so we need the next 2 test
       && ( statBufItem.st_size == statBuf.st_size )   // Files of same size
       && ( statBufItem.st_mtime == statBuf.st_mtime ) // Files modified at the same time
       ) {
      return true;
   }
#endif
   return false;
}

const clang::CXXRecordDecl *R__ScopeSearch(const char *name, const clang::Type** resultType = 0);

BaseSelectionRule::BaseSelectionRule(long index, BaseSelectionRule::ESelect sel, const std::string& attributeName, const std::string& attributeValue)
   : fIndex(index), fIsSelected(sel),fMatchFound(false),fCXXRecordDecl(0),fRequestedType(0)
{
   fAttributes.insert(AttributesMap_t::value_type(attributeName, attributeValue));
}

void BaseSelectionRule::SetSelected(BaseSelectionRule::ESelect sel)
{
   fIsSelected = sel;
}

BaseSelectionRule::ESelect BaseSelectionRule::GetSelected() const
{
   return fIsSelected;
}

bool BaseSelectionRule::HasAttributeWithName(const std::string& attributeName) const
{
   AttributesMap_t::const_iterator iter = fAttributes.find(attributeName);
   
   if(iter!=fAttributes.end()) return true;
   else return false;
}

bool BaseSelectionRule::GetAttributeValue(const std::string& attributeName, std::string& returnValue) const
{
   AttributesMap_t::const_iterator iter = fAttributes.find(attributeName);
   
   if(iter!=fAttributes.end()) {
      returnValue = iter->second;
      return true;
   }
   else {
      returnValue = "No such attribute";
      return false;
   }
}

void BaseSelectionRule::SetAttributeValue(const std::string& attributeName, const std::string& attributeValue)
{
   fAttributes.insert(AttributesMap_t::value_type(attributeName, attributeValue));
   
   int pos = attributeName.find("pattern");
   int pos_file = attributeName.find("file_pattern");
   
   if (pos > -1) {
      if (pos_file > -1) // if we have file_pattern 
         ProcessPattern(attributeValue, fFileSubPatterns);
      else ProcessPattern(attributeValue, fSubPatterns); // if we have pattern and proto_pattern
   }
}

const BaseSelectionRule::AttributesMap_t& BaseSelectionRule::GetAttributes() const
{
   return fAttributes;
}

void BaseSelectionRule::DebugPrint() const
{
   Print(std::cout);
}  

void BaseSelectionRule::PrintAttributes(std::ostream &out, int level) const
{ 
   std::string tabs;
   for (int i = 0; i < level; ++i) {
      tabs+='\t';
   }
   
   if (!fAttributes.empty()) {
      for (AttributesMap_t::const_iterator iter = fAttributes.begin(); iter!=fAttributes.end(); ++iter) {
         out<<tabs<<iter->first<<" = "<<iter->second<<std::endl;
      }
   }
   else {
      out<<tabs<<"No attributes"<<std::endl;
   }
}

void BaseSelectionRule::PrintAttributes(int level) const
{ 
   PrintAttributes(std::cout, level);
}

BaseSelectionRule::EMatchType BaseSelectionRule::Match(const clang::NamedDecl *decl, const std::string& name, 
                                                       const std::string& prototype, 
                                                       bool isLinkdef) const
{
   /* This method returns whether and how the declaration is matching the rule.
    * It returns one of:
    *   kNoMatch : the rule does match the declaration
    *   kName    : the rule match the declaration by name
    *   kPattern : the rule match the declaration via a pattern
    *   kFile    : the declaration's file name is match by the rule (either by name or pattern).
    * To check whether the rule is accepting or vetoing the declaration see the result of
    * GetSelected().
    (
    * We pass as arguments of the method:
    *   name - the name of the Decl
    *   prototype - the prototype of the Decl (if it is function or method, otherwise "")
    *   file_name - name of the source file
    *   isLinkdef - if the selection rules were generating from a linkdef.h file 
    */ 
   
   if (HasAttributeWithName("pattern") || HasAttributeWithName("proto_pattern")) {
      if (fSubPatterns.empty()) {
         std::cout<<"Error - skip?"<<std::endl;
         return kNoMatch;
      }
   }

   std::string name_value;
   bool has_name_attribute = GetAttributeValue("name", name_value);
   std::string pattern_value;
   bool has_pattern_attribute = GetAttributeValue("pattern", pattern_value);
   
   if (GetCXXRecordDecl() !=0 && GetCXXRecordDecl() != (void*)-1) {
      const clang::CXXRecordDecl *target = GetCXXRecordDecl();
      const clang::CXXRecordDecl *D = llvm::dyn_cast<clang::CXXRecordDecl>(decl);
      if ( target && D && target == llvm::dyn_cast<clang::CXXRecordDecl>( D ) ) {
         //               fprintf(stderr,"DECL MATCH: %s %s\n",name_value.c_str(),name.c_str());
         const_cast<BaseSelectionRule*>(this)->SetMatchFound(true);
         return kName;
      }
   } else if (has_name_attribute) {
      if (name_value == name) {
         const_cast<BaseSelectionRule*>(this)->SetMatchFound(true);
         return kName;
      } else if ( GetCXXRecordDecl() != (void*)-1 ) {
         // Try a real match!
         
         const clang::CXXRecordDecl *D = llvm::dyn_cast<clang::CXXRecordDecl>(decl);
         const clang::CXXRecordDecl *target = R__ScopeSearch(name_value.c_str());
         if ( target ) {
            const_cast<BaseSelectionRule*>(this)->fCXXRecordDecl = target;
         } else {
            // If the lookup failed, let's not try it again, so mark the value has invalid.
            const_cast<BaseSelectionRule*>(this)->fCXXRecordDecl = (clang::CXXRecordDecl*)-1;
         }
         if ( target && D && target == llvm::dyn_cast<clang::CXXRecordDecl>( D ) ) {
            const_cast<BaseSelectionRule*>(this)->SetMatchFound(true);
            return kName;
         }
      }
   }

   // do we have matching against the file_name (or file_pattern) attribute and if yes - select or veto
   std::string file_name_value;
   bool has_file_name_attribute = GetAttributeValue("file_name", file_name_value);
   std::string file_pattern_value;
   bool has_file_pattern_attribute = GetAttributeValue("file_pattern", file_pattern_value);
   
   if ((has_file_name_attribute||has_file_pattern_attribute)) {
      const char *file_name = R__GetDeclSourceFileName(decl);
      bool hasFileMatch = ((has_file_name_attribute && 
           //FIXME It would be much better to cache the rule stat result and compare to the clang::FileEntry
           (R__match_filename(file_name_value.c_str(),file_name))) 
          ||
          (has_file_pattern_attribute && 
           CheckPattern(file_name, file_pattern_value, fFileSubPatterns, isLinkdef)));

#if MATCH_ON_INSTANTIATION_LOCATION
      if (!hasFileMatch) {
         const clang::ClassTemplateSpecializationDecl *tmpltDecl =
            llvm::dyn_cast<clang::ClassTemplateSpecializationDecl>(decl);
         // Try the instantiation point.
         if (tmpltDecl) {
            file_name = R__GetDeclSourceFileName(tmpltDecl);
            hasFileMatch = ((has_file_name_attribute && 
                             //FIXME It would be much better to cache the rule stat result and compare to the clang::FileEntry
                             (R__match_filename(file_name_value.c_str(),file_name))) 
                            ||
                            (has_file_pattern_attribute && 
                             CheckPattern(file_name, file_pattern_value, fFileSubPatterns, isLinkdef)));
         }
      }
#endif
      if (hasFileMatch) {
         // Reject utility class defined in ClassImp
         // when using a file based rule
         if (!strncmp(name.c_str(), "R__Init", 7) ||
             strstr(name.c_str(), "::R__Init")) {
            return kNoMatch;
         }
         if (has_pattern_attribute) {
         if (CheckPattern(name, pattern_value, fSubPatterns, isLinkdef)) {
            const_cast<BaseSelectionRule*>(this)->SetMatchFound(true);
            return kFile;
         }
         } else {
            const_cast<BaseSelectionRule*>(this)->SetMatchFound(true);
            return kFile;
         }
      }
      
      // We have file_name or file_pattern attribute but the
      // passed file_name is different than that in the selection rule then return no match
      return kNoMatch;
   }


   if (has_pattern_attribute && CheckPattern(name, pattern_value, fSubPatterns, isLinkdef)) {
      const_cast<BaseSelectionRule*>(this)->SetMatchFound(true);
      return kPattern;
   }
   
#if NOT_WORKING_AND_CURRENTLY_NOT_NEEDED
   std::string proto_name_value;
   bool has_proto_name_attribute = GetAttributeValue("proto_name", proto_name_value);
   std::string proto_pattern_value;
   bool has_proto_pattern_attribute = GetAttributeValue("proto_pattern", proto_pattern_value);
   
   // do we have matching against the proto_name (or proto_pattern)  attribute and if yes - select or veto
   // The following selects functions on whether the requested prototype exactly matches the
   // prototype issued by SelectionRules::GetFunctionPrototype which relies on
   //    ParmVarDecl::getType()->getAsString()
   // to get the type names.  Currently, this does not print the prototype in the usual
   // human (written) forms.   For example:
   //   For Hash have prototype: '(const class TString &)'
   //   For Hash have prototype: '(const class TString*)'
   //   For Hash have prototype: '(const char*)'
   // In addition, the const can legally be in various place in the type name and thus
   // a string based match will be hard to work out (it would need to normalize both
   // the user input string and the clang provided string).
   // Using lookup form cling would be probably be a better choice.
   if (!prototype.empty()) {
      if (has_proto_name_attribute && 
          (proto_name_value==prototype)) {
         
         const_cast<BaseSelectionRule*>(this)->SetMatchFound(true);
         return kName;
      }
      if (has_proto_pattern_attribute && 
          CheckPattern(prototype, proto_pattern_value, fSubPatterns, isLinkdef))  {
         const_cast<BaseSelectionRule*>(this)->SetMatchFound(true);
         return kPattern; 
      }
   }
}
#endif
      
   return kNoMatch;
}


/*
 * This method processes the pattern - which means that it splits it in a list of fSubPatterns.
 * The idea is the following - if we have a pattern = "this*pat*rn", it will be split in the
 * following list of subpatterns: "this", "pat", "rn". If we have "this*pat\*rn", it will be 
 * split in "this", "pat*rn", i.e. the star could be escaped. 
 */

void BaseSelectionRule::ProcessPattern(const std::string& pattern, std::list<std::string>& out) 
{
   std::string temp = pattern;
   std::string split;
   int pos;
   bool escape = false;
   
   if (pattern == "*"){
      out.push_back("");
      return;
   }
   
   while (!temp.empty()){
      pos = temp.find("*");
      if (pos == -1) {
         if (!escape){ // if we don't find a '*', push_back temp (contains the last sub-pattern)
            out.push_back(temp);
            // std::cout<<"1. pushed = "<<temp<<std::endl;
         }
         else { // if we don't find a star - add temp to split (in split we keep the previous sub-pattern + the last escaped '*')
            split += temp;
            out.push_back(split);
            // std::cout<<"1. pushed = "<<split<<std::endl;
         }
         return;
      }
      else if (pos == 0) { // we have '*' at the beginning of the pattern; can't have '\' before the '*'
         temp = temp.substr(1); // remove the '*'
      }
      else if (pos == (int)(temp.length()-1)) { // we have '*' at the end of the pattern
         if (pos > 0 && temp.at(pos-1) == '\\') { // check if we have '\' before the '*'; if yes, we have to escape it
            split += temp.substr(0, temp.length()-2);  // add evrything from the beginning of temp till the '\' to split (where we keep the last sub-pattern)
            split += temp.at(pos); // add the '*'
            out.push_back(split);  // push_back() split
            // std::cout<<"3. pushed = "<<split<<std::endl;
            temp.clear(); // empty temp (the '*' was at the last position of temp, so we don't have anything else to process)
         }
         temp = temp.substr(0, (temp.length()-1)); 
      }
      else { // the '*' is at a random position in the pattern
         if (pos > 0 && temp.at(pos-1) == '\\') { // check if we have '\' before the '*'; if yes, we have to escape it
            split += temp.substr(0, pos-1); // remove the '\' and add the star to split
            split += temp.at(pos);
            escape = true;                  // escape = true which means that we will add the next sub-pattern to that one 
            
            // DEBUG std::cout<<"temp = "<<temp<<std::endl;
            temp = temp.substr(pos);
            // DEBUG std::cout<<"temp = "<<temp<<", split = "<<split<<std::endl;
         }
         else { // if we don't have '\' before the '*'
            if (escape) { 
               split += temp.substr(0, pos);
            }
            else {
               split = temp.substr(0, pos);
            }
            escape = false;
            temp = temp.substr(pos);
            out.push_back(split);
            // std::cout<<"2. pushed = "<<split<<std::endl;
            // DEBUG std::cout<<"temp = "<<temp<<std::endl;
            split = "";
         }
      }
      // DEBUG std::cout<<"temp = "<<temp<<std::endl;
   }
}

bool BaseSelectionRule::BeginsWithStar(const std::string& pattern) {
   if (pattern.at(0) == '*') {
      return true;
   }
   else {
      return false;
   }
}

bool BaseSelectionRule::EndsWithStar(const std::string& pattern) {
   if (pattern.at(pattern.length()-1) == '*') {
      return true;
   }
   else {
      return false;
   }
}

/*
 * This method checks if the given test string is matched against the pattern 
 */

bool BaseSelectionRule::CheckPattern(const std::string& test, const std::string& pattern, const std::list<std::string>& patterns_list, bool isLinkdef)
{
   if (pattern == "*" /* && patterns_list.back().size() == 0 */) {
      // We have the simple pattern '*', it matches everything by definition!
      return true;
   }

   std::list<std::string>::const_iterator it = patterns_list.begin();
   int pos1 = -1, pos2 = -1, pos_end = -1;
   bool begin = BeginsWithStar(pattern);
   bool end = EndsWithStar(pattern);
   
   // we first check if the last sub-pattern is contained in the test string 
   std::string last = patterns_list.back();
   pos_end = test.rfind(last);
   
   if (pos_end == -1) { // the last sub-pattern isn't conatained in the test string
      return false;
   }
   if (!end) {  // if the pattern doesn't end with '*', the match has to be complete 
      // i.e. if the last sub-pattern is "sub" the test string should end in "sub" ("1111sub" is OK, "1111sub1" is not OK)
      
      int len = last.length(); // length of last sub-pattern
      if ((pos_end+len) < (int)test.length()) {
         return false;
      }
   }
   
   // position of the first sub-pattern
   pos1 = test.find(*it);
   
   
   if (pos1 == -1 || (!begin && pos1 != 0)) { // if the first sub-pattern isn't found in test or if it is found but the
      // pattern doesn't start with '*' and the sub-pattern is not at the first position
      //std::cout<<"\tNo match!"<<std::endl;
      return false;
   }
   
   if (isLinkdef) { // A* selects all global classes, unions, structs but not the nested, i.e. not A::B
      // A::* selects the nested classes
      int len = (*it).length();
      int pos_colon = test.find("::", pos1+len);
      
      if (pos_colon > -1) {
         
#ifdef SELECTION_DEBUG
         std::cout<<"\tNested - don't generate dict (ret false to isSelected)"<<std::endl;
#endif
         
         return false;
      }
      
   }
   
   if (patterns_list.size() > 1) {
      if ((int)((*it).length())+pos1 > pos_end) {
         // std::cout<<"\tNo match";
         return false; // end is contained in begin -> test = "A::B" sub-patterns = "A::", "::" will return false
      } 
   }
   
   
   ++it;
   
   for (; it != patterns_list.end(); ++it) {
      // std::cout<<"sub-pattern = "<<*it<<std::endl; 
      pos2 = test.find(*it);
      if (pos2 <= pos1) {
         // std::cout<<"\tNo match!"<<std::endl;
         return false;
      }
      pos1 = pos2;
   }
   
   //std::cout<<"\tMatch complete!"<<std::endl;
   return true;
}


void BaseSelectionRule::SetMatchFound(bool match)
{
   fMatchFound = match;
}

bool BaseSelectionRule::GetMatchFound() const
{
   return fMatchFound;
}

const clang::Type *BaseSelectionRule::GetRequestedType() const
{
   return fRequestedType;
}

const clang::CXXRecordDecl *BaseSelectionRule::GetCXXRecordDecl() const
{
   return fCXXRecordDecl;
}

void BaseSelectionRule::SetCXXRecordDecl(const clang::CXXRecordDecl *decl, const clang::Type *typeptr)
{
   fCXXRecordDecl = decl;
   fRequestedType = typeptr;
}

