// @(#)root/html:$Id$
// Author: Axel Naumann 2007-01-09

/*************************************************************************
 * Copyright (C) 1995-2007, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#include "TDocOutput.h"

#include "Riostream.h"
#include "TClassDocOutput.h"
#include "TDataMember.h"
#include "TDataType.h"
#include "TDocInfo.h"
#include "TDocParser.h"
#include "TEnv.h"
#include "TGlobal.h"
#include "THtml.h"
#include "TMethod.h"
#include "TROOT.h"
#include "TSystem.h"
#include "TVirtualMutex.h"
#include <vector>
#include <list>
#include <set>
#include <sstream>

namespace {

   typedef std::vector<std::string> Words_t;
   typedef Words_t::const_iterator SectionStart_t;

   class TSectionInfo {
   public:
      TSectionInfo(SectionStart_t start, size_t chars, size_t size):
         fStart(start), fChars(chars), fSize(size) {};

         SectionStart_t fStart;
         size_t fChars;
         size_t fSize;
   };
   typedef std::list<TSectionInfo> SectionStarts_t;

   static void Sections_BuildIndex(SectionStarts_t& sectionStarts,
      SectionStart_t begin, SectionStart_t end, 
      size_t maxPerSection) 
   {
      // for each assumed section border, check that previous entry's
      // char[selectionChar] differs, else move section start forward

      SectionStart_t cursor = begin;
      if (sectionStarts.empty() || sectionStarts.back().fStart != cursor)
         sectionStarts.push_back(TSectionInfo(cursor, 1, 0));

      SectionStarts_t::iterator prevSection = sectionStarts.end();
      --prevSection;

      while (cursor != end) {
         size_t numLeft = end - cursor;
         size_t assumedNumSections = (numLeft + maxPerSection - 1 ) / maxPerSection;
         size_t step = ((numLeft + assumedNumSections - 1) / assumedNumSections);
         if (!step || step >= numLeft) return;
         cursor += step;
         if (cursor == end) break;

         SectionStart_t addWhichOne = prevSection->fStart;

         size_t selectionChar=1;
         for (; selectionChar <= cursor->length() && addWhichOne == prevSection->fStart; 
            ++selectionChar) {
            SectionStart_t checkPrev = cursor;
            while (--checkPrev != prevSection->fStart 
               && !strncasecmp(checkPrev->c_str(), cursor->c_str(), selectionChar));

            SectionStart_t checkNext = cursor;
            while (++checkNext != end
               && !strncasecmp(checkNext->c_str(), cursor->c_str(), selectionChar));

            // if the previous matching one is closer but not previous section start, take it!
            if (checkPrev != prevSection->fStart)
               if ((cursor - checkPrev) <= (checkNext - cursor))
                  addWhichOne = ++checkPrev;
               else if (checkNext != end
                  && (size_t)(checkNext - cursor) < maxPerSection) {
                  addWhichOne = checkNext;
               }
         }
         if (addWhichOne == prevSection->fStart)
            addWhichOne = cursor;

         selectionChar = 1;
         while (selectionChar <= prevSection->fStart->length() 
            && selectionChar <= addWhichOne->length() 
            && !strncasecmp(prevSection->fStart->c_str(), addWhichOne->c_str(), selectionChar))
            ++selectionChar;

         sectionStarts.push_back(TSectionInfo(addWhichOne, selectionChar, 0));
         cursor = addWhichOne;
         ++prevSection;
      } // while cursor != end
   }

   static void Sections_SetSize(SectionStarts_t& sectionStarts, const Words_t &words)
   {
      // Update the length of the sections
      for (SectionStarts_t::iterator iSectionStart = sectionStarts.begin();
         iSectionStart != sectionStarts.end(); ++iSectionStart) {
         SectionStarts_t::iterator next = iSectionStart;
         ++next;
         if (next == sectionStarts.end()) {
            iSectionStart->fSize = (words.end() - iSectionStart->fStart);
            break;
         }
         iSectionStart->fSize = (next->fStart - iSectionStart->fStart);
      }
   }

   static void Sections_PostMerge(SectionStarts_t& sectionStarts, const size_t maxPerSection)
   {
      // Merge sections that ended up being too small, up to maxPerSection entries
      for (SectionStarts_t::iterator iSectionStart = sectionStarts.begin();
         iSectionStart != sectionStarts.end();) {
         SectionStarts_t::iterator iNextSectionStart = iSectionStart;
         ++iNextSectionStart;
         if (iNextSectionStart == sectionStarts.end()) break;
         if (iNextSectionStart->fSize + iSectionStart->fSize < maxPerSection) {
            iSectionStart->fSize += iNextSectionStart->fSize;
            sectionStarts.erase(iNextSectionStart);
         } else ++iSectionStart;
      }
   }

   static void GetIndexChars(const Words_t& words, UInt_t numSectionsIn, 
      std::vector<std::string> &sectionMarkersOut)
   {
      // Given a list of words (class names, in this case), this function builds an
      // optimal set of about numSectionIn sections (even if almost all words start 
      // with a "T"...), and returns the significant characters for each section start 
      // in sectionMarkersOut.

      const size_t maxPerSection = (words.size() + numSectionsIn - 1)/ numSectionsIn;
      SectionStarts_t sectionStarts;
      Sections_BuildIndex(sectionStarts, words.begin(), words.end(), maxPerSection);
      Sections_SetSize(sectionStarts, words);
      Sections_PostMerge(sectionStarts, maxPerSection);

      // convert to index markers
      sectionMarkersOut.clear();
      sectionMarkersOut.resize(sectionStarts.size());
      size_t idx = 0;
      for (SectionStarts_t::iterator iSectionStart = sectionStarts.begin();
         iSectionStart != sectionStarts.end(); ++iSectionStart)
         sectionMarkersOut[idx++] = 
            iSectionStart->fStart->substr(0, iSectionStart->fChars);
   }

   static void GetIndexChars(const std::list<std::string>& wordsIn, UInt_t numSectionsIn, 
      std::vector<std::string> &sectionMarkersOut)
   {
      // initialize word vector
      Words_t words(wordsIn.size());
      size_t idx = 0;
      for (std::list<std::string>::const_iterator iWord = wordsIn.begin(); iWord != wordsIn.end(); ++iWord)
         words[idx++] = *iWord;
      GetIndexChars(words, numSectionsIn, sectionMarkersOut);
   }

}


namespace {

   //______________________________________________________________________________
   static int CaseInsensitiveSort(const void *name1, const void *name2)
   {
   // Friend function for sorting strings, case insensitive
   //
   //
   // Input: name1 - pointer to the first string
   //        name2 - pointer to the second string
   //
   //  NOTE: This function compares its arguments and returns an integer less
   //        than, equal to, or greater than zero, depending on whether name1
   //        is lexicographically less than, equal to, or greater than name2,
   //        but characters are forced to lower-case prior to comparison.
   //
   //

      return (strcasecmp(*((char **) name1), *((char **) name2)));
   }

   // std::list::sort(with_stricmp_predicate) doesn't work with Solaris CC...
   static void sort_strlist_stricmp(std::list<std::string>& l)
   {
      // sort strings ignoring case - easier for humans
      struct posList {
         const char* str;
         std::list<std::string>::const_iterator pos;
      };
      posList* carr = new posList[l.size()];
      size_t idx = 0;
      for (std::list<std::string>::const_iterator iS = l.begin(); iS != l.end(); ++iS) {
         carr[idx].pos = iS;
         carr[idx++].str = iS->c_str();
      }
      qsort(&carr[0].str, idx, sizeof(posList), CaseInsensitiveSort);
      std::list<std::string> lsort;
      for (idx = 0; idx < l.size(); ++idx) {
         lsort.push_back(*carr[idx].pos);
      }
      delete [] carr;
      l.swap(lsort);
   }

}

//______________________________________________________________________________
//
// THtml generated documentation is written to file by TDocOutput. So far only
// output of HTML is implemented. Customization of the output should be done
// with THtml's interfaces - TDocOutput should not be used nor re-implemented
// directly.
//
// TDocOutput generates the index tables:
// * classes (THtml invokes TClassDocOutput for each),
// * inheritance hierarchy,
// * types and typedefs,
// * libraries,
// * the product index, and
// * the module index (including the links to per-module documentation).
// It invokes AT&T's GraphViz tool (dot) if available; charts benefit a lot
// from it.
//
// TDocOutput also writes all pages' header and footer, which can be customized 
// by calling THtml::SetHeader(), THtml::SetFooter().
//______________________________________________________________________________

ClassImp(TDocOutput);

//______________________________________________________________________________
TDocOutput::TDocOutput(THtml& html): fHtml(&html)
{}

//______________________________________________________________________________
TDocOutput::~TDocOutput()
{}

//______________________________________________________________________________
void TDocOutput::AddLink(TSubString& str, TString& link, const char* comment)
{
   // Add a link around str, with title comment.
   // Update str so it surrounds the link.

   // prepend "./" to allow callers to replace a different relative directory
   if (ReferenceIsRelative(link) && !link.BeginsWith("./"))
      link.Prepend("./");
   link.Prepend("<a href=\"");
   link += "\"";
   if (comment && strlen(comment)) {
      link += " title=\"";
      TString description(comment);
      ReplaceSpecialChars(description);
      description.ReplaceAll("\"", "&quot;");
      link += description;
      link += "\"";
   }
   link += ">";

   str.String().Insert(str.Start() + str.Length(), "</a>");
   str.String().Insert(str.Start(), link);

   TString &strString = str.String();
   TSubString update = strString(str.Start(), str.Length() + link.Length() + 4);
   str = update;
}

//______________________________________________________________________________
void TDocOutput::AdjustSourcePath(TString& line, const char* relpath /*= "../"*/)
{
   // adjust the path of links for source files, which are in src/, but need
   // to point to relpath (usually "../"). Simply replaces "=\"./" by "=\"../"

   TString replWithRelPath("=\"");
   replWithRelPath += relpath;
   line.ReplaceAll("=\"./", replWithRelPath);
}

//______________________________________________________________________________
void TDocOutput::Convert(std::istream& in, const char* outfilename, const char *title,
                         const char *relpath /*= "../"*/)
{
   // Convert a text file into a html file.
   // outfilename doesn't have an extension yet; up to us to decide.
   // We generate HTML, so our extension is ".html".
   // See THtml::Convert() for the otehr parameters.

   TString htmlFilename(outfilename);
   htmlFilename += ".html";

   std::ofstream out(htmlFilename);

   if (!out.good()) {
      Error("Convert", "Can't open file '%s' !", htmlFilename.Data());
      return;
   }

   Printf("Convert: %s", htmlFilename.Data());

   // write a HTML header
   WriteHtmlHeader(out, title, relpath);

   out << "<h1>" << title << "</h1>" << endl;
   out << "<pre>" << endl;

   TDocParser parser(*this);
   parser.Convert(out, in, relpath);

   out << "</pre>" << endl;

   // write a HTML footer
   WriteHtmlFooter(out, relpath);
}

//______________________________________________________________________________
Bool_t TDocOutput::CopyHtmlFile(const char *sourceName, const char *destName)
{
// Copy file to HTML directory
//
//
//  Input: sourceName - source file name
//         destName   - optional destination name, if not
//                      specified it would be the same
//                      as the source file name
//
// Output: TRUE if file is successfully copied, or
//         FALSE if it's not
//
//
//   NOTE: The destination directory is always fHtml->GetOutputDir()
//

   R__LOCKGUARD(GetHtml()->GetMakeClassMutex());

   // source file name
   char *tmp1 = gSystem->Which(fHtml->GetSourceDir(), sourceName, kReadPermission);
   if (!tmp1) {
      Error("Copy", "Can't copy file '%s' to '%s/%s' - can't find source file!", sourceName,
            fHtml->GetOutputDir().Data(), destName);
      return kFALSE;
   }

   TString sourceFile(tmp1);
   delete[]tmp1;

   if (!sourceFile.Length()) {
      Error("Copy", "Can't copy file '%s' to '%s' directory - source file name invalid!", sourceName,
            fHtml->GetOutputDir().Data());
      return kFALSE;
   }

   // destination file name
   TString destFile;
   if (!destName || !*destName)
      destFile = fHtml->GetFileName(sourceFile);
   else
      destFile = fHtml->GetFileName(destName);

   gSystem->PrependPathName(fHtml->GetOutputDir(), destFile);

   // Get info about a file
   Long64_t size;
   Long_t id, flags, sModtime, dModtime;
   sModtime = 0;
   dModtime = 0;
   if (gSystem->GetPathInfo(sourceFile, &id, &size, &flags, &sModtime)
      || gSystem->GetPathInfo(destFile, &id, &size, &flags, &dModtime)
      || sModtime > dModtime)
      gSystem->CopyFile(sourceFile, destFile, kTRUE);

   return kTRUE;
}



//______________________________________________________________________________
void TDocOutput::CreateHierarchy()
{
// Create a hierarchical class list
// The algorithm descends from the base classes and branches into
// all derived classes. Mixing classes are displayed several times.
//
//

   // if (CreateHierarchyDot()) return;

   TString filename("ClassHierarchy.html");
   gSystem->PrependPathName(fHtml->GetOutputDir(), filename);

   // open out file
   std::ofstream out(filename);

   if (!out.good()) {
      Error("CreateHierarchy", "Can't open file '%s' !", filename.Data());
      return;
   }

   Printf(fHtml->GetCounterFormat(), "", fHtml->GetCounter(), filename.Data());

   // write out header
   WriteHtmlHeader(out, "Class Hierarchy");
   out << "<h1>Class Hierarchy</h1>" << endl;

   WriteSearch(out);

   // loop on all classes
   TClassDocInfo* cdi = 0;
   TIter iClass(fHtml->GetListOfClasses());
   while ((cdi = (TClassDocInfo*)iClass())) {
      if (!cdi->HaveSource())
         continue;

      // get class
      TClass *basePtr = cdi->GetClass();
      if (basePtr == 0) {
         Warning("THtml::CreateHierarchy", "skipping class %s\n", cdi->GetName());
         continue;
      }

      TClassDocOutput cdo(*fHtml, basePtr);
      cdo.CreateClassHierarchy(out, cdi->GetHtmlFileName());
   }

   // write out footer
   WriteHtmlFooter(out);
}

//______________________________________________________________________________
void TDocOutput::CreateClassIndex()
{
// Create index of all classes
//

   // create CSS file, we need it
   fHtml->CreateAuxiliaryFiles();

   TString filename("ClassIndex.html");
   gSystem->PrependPathName(fHtml->GetOutputDir(), filename);

   // open indexFile file
   std::ofstream indexFile(filename.Data());

   if (!indexFile.good()) {
      Error("CreateClassIndex", "Can't open file '%s' !", filename.Data());
      return;
   }

   Printf(fHtml->GetCounterFormat(), "", fHtml->GetCounter(), filename.Data());

   // write indexFile header
   WriteHtmlHeader(indexFile, "Class Index");

   indexFile << "<h1>Index</h1>" << endl;

   WriteModuleLinks(indexFile);

   std::vector<std::string> indexChars;
   if (fHtml->GetListOfClasses()->GetSize() > 10) {
      std::vector<std::string> classNames;
      {
         TIter iClass(fHtml->GetListOfClasses());
         TClassDocInfo* cdi = 0;
         while ((cdi = (TClassDocInfo*)iClass()))
            if (cdi->IsSelected() && cdi->HaveSource())
               classNames.push_back(cdi->GetName());
      }

      if (classNames.size() > 10) {
         indexFile << "<div id=\"indxShortX\"><h4>Jump to</h4>" << endl;
         // find index chars
         GetIndexChars(classNames, 50 /*sections*/, indexChars);
         for (UInt_t iIdxEntry = 0; iIdxEntry < indexChars.size(); ++iIdxEntry) {
            indexFile << "<a href=\"#idx" << iIdxEntry << "\">";
            ReplaceSpecialChars(indexFile, indexChars[iIdxEntry].c_str());
            indexFile << "</a>" << endl;
         }
         indexFile << "</div><br />" << endl;
      }
   }

   WriteSearch(indexFile);

   indexFile << "<ul id=\"indx\">" << endl;

   // loop on all classes
   UInt_t currentIndexEntry = 0;
   TIter iClass(fHtml->GetListOfClasses());
   TClassDocInfo* cdi = 0;
   Int_t i = 0;
   while ((cdi = (TClassDocInfo*)iClass())) {
      if (!cdi->IsSelected() || !cdi->HaveSource())
         continue;

      // get class
      TClass* currentClass = cdi->GetClass();
      if (!currentClass) {
         Warning("THtml::CreateClassIndex", "skipping class %s\n", cdi->GetName());
         continue;
      }

      indexFile << "<li class=\"idxl" << (i++)%2 << "\">";
      if (currentIndexEntry < indexChars.size()
         && !strncmp(indexChars[currentIndexEntry].c_str(), cdi->GetName(), 
                     indexChars[currentIndexEntry].length()))
         indexFile << "<a name=\"idx" << currentIndexEntry++ << "\"></a>";

      TString htmlFile(cdi->GetHtmlFileName());
      if (htmlFile.Length()) {
         indexFile << "<a href=\"";
         indexFile << htmlFile;
         indexFile << "\"><span class=\"typename\">";
         ReplaceSpecialChars(indexFile, cdi->GetName());
         indexFile << "</span></a> ";
      } else {
         indexFile << "<span class=\"typename\">";
         ReplaceSpecialChars(indexFile, cdi->GetName());
         indexFile << "</span> ";
      }

      // write title == short doc
      ReplaceSpecialChars(indexFile, currentClass->GetTitle());
      indexFile << "</li>" << endl;
   }

   indexFile << "</ul>" << endl;

   // write indexFile footer
   WriteHtmlFooter(indexFile);
}


//______________________________________________________________________________
void TDocOutput::CreateModuleIndex()
{
   // Create the class index for each module, picking up documentation from the 
   // module's TModuleDocInfo::GetSourceDir() plus the (possibly relative) 
   // THtml::GetModuleDocPath(). Also creates the library dependency plot if dot
   // exists, see THtml::HaveDot().

   const char* title = "LibraryDependencies";
   TString filename(title);
   gSystem->PrependPathName(fHtml->GetOutputDir(), filename);

   std::ofstream libDepDotFile(filename + ".dot");
   libDepDotFile << "digraph G {" << endl
                 << "ratio=compress;" << endl
                 << "node [fontsize=22,labeldistance=0.1];" << endl
                 << "edge [len=0.01];" << endl
                 << "fontsize=22;" << endl
                 << "size=\"16,16\";" << endl
                 << "overlap=false;" << endl
                 << "splines=true;" << endl
                 << "K=0.1;" << endl;

   TModuleDocInfo* module = 0;
   TIter iModule(fHtml->GetListOfModules());

   std::stringstream sstrCluster;
   std::stringstream sstrDeps;
   while ((module = (TModuleDocInfo*)iModule())) {
      if (!module->IsSelected())
         continue;

      std::vector<std::string> indexChars;
      TString filename(module->GetName());
      filename += "_Index.html";
      gSystem->PrependPathName(fHtml->GetOutputDir(), filename);
      std::ofstream outputFile(filename.Data());
      if (!outputFile.good()) {
         Error("MakeIndex", "Can't open file '%s' !", filename.Data());
         continue;
      }
      Printf(fHtml->GetCounterFormat(), "", fHtml->GetCounter(), filename.Data());

      TString htmltitle("Index of ");
      htmltitle += module->GetName();
      htmltitle += " classes";
      WriteHtmlHeader(outputFile, htmltitle);
      outputFile << "<h2>" << htmltitle << "</h2>" << endl;

      // Module doc
      if (GetHtml()->GetModuleDocPath().Length()) {
         TString outdir(module->GetName());
         gSystem->PrependPathName(GetHtml()->GetOutputDir(), outdir);

         TString moduleDocDir(GetHtml()->GetModuleDocPath());
         if (!gSystem->IsAbsoluteFileName(moduleDocDir))
            gSystem->PrependPathName(module->GetSourceDir(), moduleDocDir);
         ProcessDocInDir(outputFile, moduleDocDir, outdir, module->GetName());
      }

      std::list<std::string> classNames;
      {
         TIter iClass(module->GetClasses());
         TClassDocInfo* cdi = 0;
         while ((cdi = (TClassDocInfo*) iClass())) {
            if (!cdi->IsSelected() || !cdi->HaveSource())
               continue;
            classNames.push_back(cdi->GetName());

            if (classNames.size() > 1) continue;

            TString libs(cdi->GetClass()->GetSharedLibs());
            Ssiz_t posDepLibs = libs.Index(' ');
            TString thisLib(libs);
            if (posDepLibs != kNPOS)
               thisLib.Remove(posDepLibs, thisLib.Length());
            Ssiz_t posExt = thisLib.First('.');
            if (posExt != kNPOS)
               thisLib.Remove(posExt, thisLib.Length());

            if (!thisLib.Length())
               continue;

            // allocate entry, even if no dependencies
            TLibraryDocInfo *libdeps = 
               (TLibraryDocInfo*)fHtml->GetLibraryDependencies()->FindObject(thisLib);
            if (!libdeps) {
               libdeps = new TLibraryDocInfo(thisLib);
               fHtml->GetLibraryDependencies()->Add(libdeps);
            }
            libdeps->AddModule(module->GetName());
            if (posDepLibs != kNPOS) {
               std::string lib;
               for(Ssiz_t pos = posDepLibs + 1; libs[pos]; ++pos) {
                  if (libs[pos] == ' ') {
                     if (thisLib.Length() && lib.length()) {
                        size_t posExt = lib.find('.');
                        if (posExt != std::string::npos)
                           lib.erase(posExt);
                        libdeps->AddDependency(lib);
                     }
                     lib.erase();
                  } else 
                     lib += libs[pos];
               }
               if (lib.length() && thisLib.Length()) {
                  size_t posExt = lib.find('.');
                  if (posExt != std::string::npos)
                     lib.erase(posExt);
                  libdeps->AddDependency(lib);
               }
            } // if dependencies
         } // while next class in module
      } // just a scope block

      if (classNames.size() > 10) {
         outputFile << "<div id=\"indxShortX\"><h4>Jump to</h4>" << endl;
         UInt_t numSections = classNames.size() / 10;
         if (numSections < 10) numSections = 10;
         if (numSections > 50) numSections = 50;
         // find index chars
         GetIndexChars(classNames, numSections, indexChars);
         for (UInt_t iIdxEntry = 0; iIdxEntry < indexChars.size(); ++iIdxEntry) {
            outputFile << "<a href=\"#idx" << iIdxEntry << "\">";
            ReplaceSpecialChars(outputFile, indexChars[iIdxEntry].c_str());
            outputFile << "</a>" << endl;
         }
         outputFile << "</div><br />" << endl;
      }
      outputFile << "<ul id=\"indx\">" << endl;

      TIter iClass(module->GetClasses());
      TClassDocInfo* cdi = 0;
      UInt_t count = 0;
      UInt_t currentIndexEntry = 0;
      while ((cdi = (TClassDocInfo*) iClass())) {
         if (!cdi->IsSelected() || !cdi->HaveSource())
            continue;

         TClass *classPtr = cdi->GetClass();
         if (!classPtr) {
            Error("MakeIndex", "Unknown class '%s' !", cdi->GetName());
            continue;
         }

         // write a classname to an index file
         outputFile << "<li class=\"idxl" << (count++)%2 << "\">";
         if (currentIndexEntry < indexChars.size()
            && !strncmp(indexChars[currentIndexEntry].c_str(), cdi->GetName(), 
                        indexChars[currentIndexEntry].length()))
            outputFile << "<a name=\"idx" << currentIndexEntry++ << "\"></a>";

         TString htmlFile(cdi->GetHtmlFileName());
         if (htmlFile.Length()) {
            outputFile << "<a href=\"";
            outputFile << htmlFile;
            outputFile << "\"><span class=\"typename\">";
            ReplaceSpecialChars(outputFile, classPtr->GetName());
            outputFile << "</span></a> ";
         } else {
            outputFile << "<span class=\"typename\">";
            ReplaceSpecialChars(outputFile, classPtr->GetName());
            outputFile << "</span> ";
         }

         // write title
         ReplaceSpecialChars(outputFile, classPtr->GetTitle());
         outputFile << "</li>" << endl;
      }


      outputFile << "</ul>" << endl;

      // write outputFile footer
      WriteHtmlFooter(outputFile);
   } // while next module

   // libCint is missing as we don't have class doc for it
   // We need it for dependencies nevertheless, so add it by hand.
   /*
   sstrCluster << "subgraph clusterlibCint {" << endl
      << "style=filled;" << endl
      << "color=lightgray;" << endl
      << "label=\"libCint\";" << endl
      << "\"CINT\" [style=filled,color=white,fontsize=10]" << endl
      << "}" << endl;
   */

   // simplify the library dependencies, by removing direct links
   // that are equivalent to indirect ones, e.g. instead of having both
   // A->C, A->B->C, keep only A->B->C.

   TIter iLib(fHtml->GetLibraryDependencies());
   TLibraryDocInfo* libinfo = 0;
   while ((libinfo = (TLibraryDocInfo*)iLib())) {
      if (!libinfo->GetName() || !libinfo->GetName()[0]) continue;

      std::set<std::string>& deps = libinfo->GetDependencies();
      for (std::set<std::string>::iterator iDep = deps.begin();
           iDep != deps.end(); ++iDep) {
         Bool_t already_indirect = kFALSE;
         for (std::set<std::string>::const_iterator iDep2 = deps.begin();
              !already_indirect && iDep2 != deps.end(); ++iDep2) {
            if (iDep == iDep2) continue;
            TLibraryDocInfo* libinfo2 = (TLibraryDocInfo*) 
               fHtml->GetLibraryDependencies()->FindObject(iDep2->c_str());
            if (!libinfo2) continue;
            const std::set<std::string>& deps2 = libinfo2->GetDependencies();
            already_indirect |= deps2.find(*iDep) != deps2.end();
         }
         if (already_indirect) {
            std::set<std::string>::iterator iRemove = iDep;
            --iDep; // otherwise we cannot do the for loop's ++iDep
            deps.erase(*iRemove);
         }
      } // for library dependencies of module in library
   } // for libaries

   iLib.Reset();
   while ((libinfo = (TLibraryDocInfo*)iLib())) {
      if (!libinfo->GetName() || !libinfo->GetName()[0]) continue;

      const std::set<std::string>& modules = libinfo->GetModules();
      if (modules.size() > 1) {
         sstrCluster << "subgraph cluster" << libinfo->GetName() << " {" << endl
                     << "style=filled;" << endl
                     << "color=lightgray;" << endl
                     << "label=\"";
         if (!strcmp(libinfo->GetName(), "libCore"))
            sstrCluster << "Everything depends on ";
         sstrCluster << libinfo->GetName() << "\";" << endl;

         const std::set<std::string>& modules = libinfo->GetModules();
         for (std::set<std::string>::const_iterator iModule = modules.begin();
              iModule != modules.end(); ++iModule) {
            sstrCluster << "\"" << *iModule << "\" [style=filled,color=white,URL=\"" 
                        << *iModule << "_Index.html\"];" << endl;
         }
         sstrCluster << endl 
                     << "}" << endl;
      } else {
         // only one module
         sstrCluster << "\"" << *modules.begin() 
                     << "\" [label=\"" << libinfo->GetName() 
                     << "\",style=filled,color=lightgray,shape=box,URL=\""
                     << *modules.begin() << "_Index.html\"];" << endl;
      }

      // GetSharedLib doesn't mention libCore or libCint; add them by hand
      /*
        if (iLibDep->first != "libCore")
        sstrDeps << "\"" << iModule->first << "\" -> \"BASE\" [lhead=clusterlibCore];" << endl;
        sstrDeps << "\"" << iModule->first << "\" -> \"CINT\" [lhead=clusterlibCint];" << endl;
      */

      const std::string& mod = *(modules.begin());
      const std::set<std::string>& deps = libinfo->GetDependencies();
      for (std::set<std::string>::const_iterator iDep = deps.begin();
            iDep != deps.end(); ++iDep) {
         // cannot create dependency on iDep directly, use its first module instead.
         TLibraryDocInfo* depLibInfo = (TLibraryDocInfo*)
            fHtml->GetLibraryDependencies()->FindObject(iDep->c_str());
         if (!depLibInfo || depLibInfo->GetModules().empty())
            continue; // ouch!

         const std::string& moddep = *(depLibInfo->GetModules().begin());
         sstrDeps << "\"" << mod << "\" -> \"" << moddep << "\";" << endl;
      }
      // make sure libCore ends up at the bottom
      sstrDeps << "\"" << mod <<  "\" -> \"CONT\" [style=invis];" << endl;
   } // for libs

   libDepDotFile << sstrCluster.str() << endl
      << sstrDeps.str();
   libDepDotFile << "}" << endl;
   libDepDotFile.close();

   std::ofstream out(filename + ".html");
   if (!out.good()) {
      Error("CreateModuleIndex", "Can't open file '%s.html' !",
            filename.Data());
      return;
   }

   Printf(fHtml->GetCounterFormat(), "", fHtml->GetCounter(), (filename + ".html").Data());
   // write out header
   WriteHtmlHeader(out, "Library Dependencies");
   out << "<h1>Library Dependencies</h1>" << endl;

   WriteSearch(out);

   RunDot(filename, &out, kFdp);

   out << "<img alt=\"Library Dependencies\" class=\"classcharts\" usemap=\"#Map" << title << "\" src=\"" << title << ".gif\"/>" << endl;

   // write out footer
   WriteHtmlFooter(out);
}

//______________________________________________________________________________
void TDocOutput::CreateProductIndex()
{
   // Fetch documentation from THtml::GetProductDocDir() and put it into the
   // product index page.

   //TString outFile(GetHtml()->GetProductName());
   //outFile += ".html";
   TString outFile("index.html");
   gSystem->PrependPathName(GetHtml()->GetOutputDir(), outFile);
   std::ofstream out(outFile);

   if (!out.good()) {
      Error("CreateProductIndex", "Can't open file '%s' !", outFile.Data());
      return;
   }

   Printf(fHtml->GetCounterFormat(), "", "", outFile.Data());

   WriteHtmlHeader(out, GetHtml()->GetProductName() + " Reference Guide");
   out << "<h1>" << GetHtml()->GetProductName() + " Reference Guide</h1>" << std::endl;

   if (GetHtml()->GetProductDocDir().Length())
      ProcessDocInDir(out, GetHtml()->GetProductDocDir(), GetHtml()->GetOutputDir(), "./");

   WriteModuleLinks(out);

   out << "<h2>Chapters</h2>" << std::endl
      << "<h3><a href=\"./ClassIndex.html\">Class Index</a></h3>" << std::endl
      << "<p>A complete list of all classes defined in " << GetHtml()->GetProductName() << "</p>" << std::endl
      << "<h3><a href=\"./ClassHierarchy.html\">Class Hierarchy</a></h3>" << std::endl
      << "<p>A hierarchy graph of all classes, showing each class's base and derived classes</p>" << std::endl
      << "<h3><a href=\"./ListOfTypes.html\">Type Index</a></h3>" << std::endl
      << "<p>A complete list of all types</p>" << std::endl
      << "<h3><a href=\"./LibraryDependencies.html\">Library Dependency</a></h3>" << std::endl
      << "<p>A diagram showing all of " << GetHtml()->GetProductName() << "'s libraries and their dependencies</p>" << std::endl;

   WriteHtmlFooter(out);
}

//______________________________________________________________________________
void TDocOutput::CreateTypeIndex()
{
// Create index of all data types

   // open file
   TString outFile("ListOfTypes.html");
   gSystem->PrependPathName(fHtml->GetOutputDir(), outFile);
   std::ofstream typesList(outFile);

   if (!typesList.good()) {
      Error("CreateTypeIndex", "Can't open file '%s' !", outFile.Data());
      return;
   }

   Printf(fHtml->GetCounterFormat(), "", "", outFile.Data());

   // write typesList header
   WriteHtmlHeader(typesList, "List of data types");
   typesList << "<h2> List of data types </h2>" << endl;

   typesList << "<dl><dd>" << endl;

   // make loop on data types
   TDataType *type;
   TIter nextType(gROOT->GetListOfTypes());

   std::list<std::string> typeNames;
   while ((type = (TDataType *) nextType()))
      // no templates ('<' and '>'), no idea why the '(' is in here...
      if (*type->GetTitle() && !strchr(type->GetName(), '(')
          && !( strchr(type->GetName(), '<') && strchr(type->GetName(),'>'))
          && type->GetName())
            typeNames.push_back(type->GetName());
   sort_strlist_stricmp(typeNames);

   std::vector<std::string> indexChars;
   if (typeNames.size() > 10) {
      typesList << "<div id=\"indxShortX\"><h4>Jump to</h4>" << endl;
      // find index chars
      GetIndexChars(typeNames, 10 /*sections*/, indexChars);
      for (UInt_t iIdxEntry = 0; iIdxEntry < indexChars.size(); ++iIdxEntry) {
         typesList << "<a href=\"#idx" << iIdxEntry << "\">";
         ReplaceSpecialChars(typesList, indexChars[iIdxEntry].c_str());
         typesList << "</a>" << endl;
      }
      typesList << "</div><br />" << endl;
   }

   typesList << "<ul id=\"indx\">" << endl;

   nextType.Reset();
   int idx = 0;
   UInt_t currentIndexEntry = 0;

   for (std::list<std::string>::iterator iTypeName = typeNames.begin(); 
      iTypeName != typeNames.end(); ++iTypeName) {
      TDataType* type = gROOT->GetType(iTypeName->c_str(), kFALSE);
      typesList << "<li class=\"idxl" << idx%2 << "\">";
      if (currentIndexEntry < indexChars.size()
         && !strncmp(indexChars[currentIndexEntry].c_str(), iTypeName->c_str(), 
                     indexChars[currentIndexEntry].length()))
         typesList << "<a name=\"idx" << currentIndexEntry++ << "\"></a>" << endl;
      typesList << "<a name=\"";
      ReplaceSpecialChars(typesList, iTypeName->c_str());
      typesList << "\"><span class=\"typename\">";
      ReplaceSpecialChars(typesList, iTypeName->c_str());
      typesList << "</span></a> ";
      ReplaceSpecialChars(typesList, type->GetTitle());
      typesList << "</li>" << endl;
      ++idx;
   }
   typesList << "</ul>" << endl;

   // write typesList footer
   WriteHtmlFooter(typesList);

   // close file
   typesList.close();

}


//______________________________________________________________________________
void TDocOutput::DecorateEntityBegin(TString& str, Ssiz_t& pos, TDocParser::EParseContext type)
{
   // Add some colors etc to a source entity, contained in str.
   // The type of what's contained in str is given by type.
   // It's called e.g. by TDocParser::BeautifyLine().
   // This function should assume that only str.Begin() is valid.
   // When inserting into str.String(), str.Begin() must be updated.

   Ssiz_t originalLen = str.Length();

   switch (type) {
      case TDocParser::kCode: break;
      case TDocParser::kComment:
         str.Insert(pos, "<span class=\"comment\">");
         break;
      case TDocParser::kDirective:
         break;
      case TDocParser::kString:
         str.Insert(pos, "<span class=\"string\">");
         break;
      case TDocParser::kKeyword:
         str.Insert(pos, "<span class=\"keyword\">");
         break;
      case TDocParser::kCPP:
         str.Insert(pos, "<span class=\"cpp\">");
         break;
      case TDocParser::kVerbatim:
         str.Insert(pos, "<pre>");
         break;
      default:
         Error("DecorateEntityBegin", "Unhandled / invalid entity type %d!", (Int_t)type);
         return;
   }

   Ssiz_t addedLen = str.Length() - originalLen;
   pos += addedLen;
}

//______________________________________________________________________________
void TDocOutput::DecorateEntityEnd(TString& str, Ssiz_t& pos, TDocParser::EParseContext type)
{
   // Add some colors etc to a source entity, contained in str.
   // The type of what's contained in str is given by type.
   // It's called e.g. by TDocParser::BeautifyLine().
   // This function should assume that only str."End()"
   // (i.e. str.Begin()+str.Length()) is valid.
   // When inserting into str.String(), str.Length() must be updated.

   Ssiz_t originalLen = str.Length();

   switch (type) {
      case TDocParser::kCode: break;
      case TDocParser::kComment:
         str.Insert(pos, "</span>");
         break;
      case TDocParser::kDirective:
         break;
      case TDocParser::kString:
         str.Insert(pos, "</span>");
         break;
      case TDocParser::kKeyword:
         str.Insert(pos, "</span>");
         break;
      case TDocParser::kCPP:
         str.Insert(pos, "</span>");
         break;
      case TDocParser::kVerbatim:
         str.Insert(pos, "</pre>");
         break;
      default:
         Error("DecorateEntityBegin", "Unhandled / invalid entity type %d!", (Int_t)type);
         return;
   }
   Ssiz_t addedLen = str.Length() - originalLen;
   pos += addedLen;
}

//______________________________________________________________________________
void TDocOutput::FixupAuthorSourceInfo(TString& authors)
{
// Special author treatment; called when TDocParser::fSourceInfo[kInfoAuthor] is set.
// Modifies the author(s) description, which is a comma separated list of tokens
// either in the format
// (i) "FirstName LastName " or
// (ii) "FirstName LastName <link> more stuff"
// The first one generates an XWho link (CERN compatible),
// the second a http link (WORLD compatible), <link> being e.g.
// <mailto:user@host.bla> or <http://www.host.bla/page>.

   TString original(authors);
   authors = "";

   TString author;
   Ssiz_t pos = 0;
   Bool_t firstAuthor = kTRUE;
   while (original.Tokenize(author, pos, ",")) {
      author.Strip(TString::kBoth);

      if (!firstAuthor)
         authors += ", ";
      firstAuthor = kFALSE;

      // do we have a link for the current name?
      Ssiz_t cLink = author.First('<'); // look for link start tag
      if (cLink != kNPOS) {
         // split NAME <LINK> POST
         // into  <a href="LINK">NAME</a> POST
         Ssiz_t endLink = author.Index(">", cLink + 1);
         if(endLink == kNPOS)
            endLink = author.Length();
         authors += "<a href=\"";
         authors += author(cLink + 1,  endLink - (cLink + 1));
         authors += "\">";
         authors += author(0, cLink);
         authors += "</a>";
         if (endLink != author.Length())
            authors += author(endLink + 1, author.Length());
      } else {
         authors += "<a href=\"";
         authors += fHtml->GetXwho();

         // separate Firstname Middlename Lastname by '+'
         TString namePart;
         Ssiz_t posNamePart = 0;
         Bool_t firstNamePart = kTRUE;
         while (author.Tokenize(namePart, posNamePart, " ")) {
            namePart.Strip(TString::kBoth);
            if (!namePart.Length())
               continue;
            if (!firstNamePart)
               authors += '+';
            firstNamePart = kFALSE;
            authors += namePart;
         }
         authors += "\">";
         authors += author;
         authors += "</a>";
      }
   } // while next author
}

//______________________________________________________________________________
Bool_t TDocOutput::IsModified(TClass * classPtr, EFileType type)
{
// Check if file is modified
//
//
//  Input: classPtr - pointer to the class
//         type     - file type to compare with
//                    values: kSource, kInclude, kTree
//
// Output: TRUE     - if file is modified since last time
//         FALSE    - if file is up to date
//

   TString sourceFile;
   TString classname(classPtr->GetName());
   TString filename;
   TString dir;

   switch (type) {
   case kSource:
      if (classPtr->GetImplFileLine()) {
         sourceFile = fHtml->GetImplFileName(classPtr);
         fHtml->GetSourceFileName(sourceFile);
      } else {
         sourceFile = fHtml->GetDeclFileName(classPtr);
         fHtml->GetSourceFileName(sourceFile);
      }
      dir = "src";
      gSystem->PrependPathName(fHtml->GetOutputDir(), dir);
      filename = classname;
      NameSpace2FileName(filename);
      gSystem->PrependPathName(dir, filename);
      if (classPtr->GetImplFileLine())
         filename += ".cxx.html";
      else
         filename += ".h.html";
      break;

   case kInclude:
      filename = fHtml->GetDeclFileName(classPtr);
      sourceFile = filename;
      fHtml->GetSourceFileName(sourceFile);
      filename = fHtml->GetFileName(filename);
      gSystem->PrependPathName(fHtml->GetOutputDir(), filename);
      break;

   case kTree:
      sourceFile = fHtml->GetDeclFileName(classPtr);
      fHtml->GetSourceFileName(sourceFile);
      NameSpace2FileName(classname);
      gSystem->PrependPathName(fHtml->GetOutputDir(), classname);
      filename = classname;
      filename += "_Tree.pdf";
      break;

   case kDoc:
      if (classPtr->GetImplFileLine()) {
         sourceFile = fHtml->GetImplFileName(classPtr);
         fHtml->GetSourceFileName(sourceFile);
      } else {
         sourceFile = fHtml->GetDeclFileName(classPtr);
         fHtml->GetSourceFileName(sourceFile);
      }
      filename = classname;
      NameSpace2FileName(filename);
      gSystem->PrependPathName(fHtml->GetOutputDir(), filename);
      filename += ".html";
      break;

   default:
      Error("IsModified", "Unknown file type !");
   }

   R__LOCKGUARD(GetHtml()->GetMakeClassMutex());

   // Get info about a file
   Long64_t size;
   Long_t id, flags, sModtime, dModtime;

   if (!(gSystem->GetPathInfo(sourceFile, &id, &size, &flags, &sModtime)))
      if (!(gSystem->GetPathInfo(filename, &id, &size, &flags, &dModtime)))
         return (sModtime > dModtime);

   return kTRUE;
}


//______________________________________________________________________________
void TDocOutput::NameSpace2FileName(TString& name)
{
   // Replace "::" in name by "__"
   // Replace "<", ">", " ", ",", "~", "=" in name by "_"
   const char* replaceWhat = ":<> ,~=";
   for (Ssiz_t i=0; i < name.Length(); ++i)
      if (strchr(replaceWhat, name[i])) 
         name[i] = '_';
}

//______________________________________________________________________________
void TDocOutput::ProcessDocInDir(std::ostream& out, const char* indir, 
                                 const char* outdir, const char* linkdir)
{
   // Write links to files indir/*.txt, indir/*.html (non-recursive) to out.
   // If one of the files is called "index.{html,txt}" it will be
   // included in out (instead of copying it to outdir and generating a link 
   // to linkdir). txt files are passed through Convert(). 
   // The files' links are sorted alphabetically.

   R__LOCKGUARD(GetHtml()->GetMakeClassMutex());

   void * dirHandle = gSystem->OpenDirectory(indir);
   if (!dirHandle) return;

   const char* entry = 0;
   std::list<std::string> files;
   while ((entry = gSystem->GetDirEntry(dirHandle))) {
      FileStat_t stat;
      TString filename(entry);
      gSystem->PrependPathName(indir, filename);
      if (gSystem->GetPathInfo(filename, stat)) // funny ret
         continue;
      if (!R_ISREG(stat.fMode)) continue;

      if (TString(entry).BeginsWith("index.", TString::kIgnoreCase)) {
         // This is the part we put directly (verbatim) into the module index.
         // If it ends on ".txt" we run Convert first.
         if (filename.EndsWith(".txt", TString::kIgnoreCase)) {
            std::ifstream in(filename);
            if (in) {
               out << "<pre>"; // this is what e.g. the html directive expects
               TDocParser parser(*this);
               parser.Convert(out, in, "../");
               out << "</pre>";
            }
         } else if (filename.EndsWith(".html", TString::kIgnoreCase)) {
            std::ifstream in(filename);
            TString line;
            while (in) {
               if (!line.ReadLine(in)) break;
               out << line << endl;
            }
         }
      } else
         files.push_back(filename.Data());
   }

   std::stringstream furtherReading;
   files.sort();
   for (std::list<std::string>::const_iterator iFile = files.begin();
      iFile != files.end(); ++iFile) {
      TString filename(iFile->c_str());
      if (!filename.EndsWith(".txt", TString::kIgnoreCase) 
         && !filename.EndsWith(".html", TString::kIgnoreCase))
         continue;

      // Just copy and link this page.
      if (gSystem->AccessPathName(outdir))
         if (gSystem->mkdir(outdir, kTRUE) == -1)
            // bad - but let's still try to create the output
            Error("CreateModuleIndex", "Cannot create output directory %s", outdir);

      TString outfile(gSystem->BaseName(filename));
      gSystem->PrependPathName(outdir, outfile);
      if (outfile.EndsWith(".txt", TString::kIgnoreCase)) {
         // convert first
         outfile.Remove(outfile.Length()-3, 3);
         outfile += "html";
         std::ifstream in(filename);
         std::ofstream out(outfile);
         if (in && out) {
            out << "<pre>"; // this is what e.g. the html directive expects
            TDocParser parser(*this);
            parser.Convert(out, in, "../");
            out << "</pre>";
         }
      } else {
         if (gSystem->CopyFile(filename, outfile, kTRUE) == -1)
            continue;
      }
      TString showname(gSystem->BaseName(outfile));
      furtherReading << "<a class=\"linkeddoc\" href=\"" << linkdir << "/" << showname << "\">";
      showname.Remove(showname.Length() - 5, 5); // .html
      showname.ReplaceAll("_", " ");
      ReplaceSpecialChars(furtherReading, showname);
      furtherReading << "</a> " << endl;
   }

   gSystem->FreeDirectory(dirHandle);
   if (furtherReading.str().length())
      out << "<h3>Further Reading</h3><div id=\"furtherreading\">" << endl
          << furtherReading.str() << "</div><h3>List of Classes</h3>" << endl;
}

//______________________________________________________________________________
void TDocOutput::ReferenceEntity(TSubString& str, TClass* entity, const char* comment /*= 0*/)
{
   // Create a reference to a class documentation page.
   // str encloses the text to create the reference for (e.g. name of instance).
   // comment will be added e.g. as tooltip text.
   // After the reference is put into str.String(), str will enclose the reference
   // and the original text. Example:
   // Input:
   //  str.String(): "a gHtml test"
   //  str.Begin():  2
   //  str.Length(): 5
   // Output:
   //  str.String(): "a <a href="THtml.html">gHtml</a> test"
   //  str.Begin():  2
   //  str.Length(): 30

   TString link;
   fHtml->GetHtmlFileName(entity, link);

   if (comment && !strcmp(comment, entity->GetName()))
      comment = "";

   AddLink(str, link, comment);
}

//______________________________________________________________________________
void TDocOutput::ReferenceEntity(TSubString& str, TDataMember* entity, const char* comment /*= 0*/)
{
   // Create a reference to a data member documentation page.
   // str encloses the text to create the reference for (e.g. name of instance).
   // comment will be added e.g. as tooltip text.
   // After the reference is put into str.String(), str will enclose the reference
   // and the original text. Example:
   // Input:
   //  str.String(): "a gHtml test"
   //  str.Begin():  2
   //  str.Length(): 5
   // Output:
   //  str.String(): "a <a href="THtml.html">gHtml</a> test"
   //  str.Begin():  2
   //  str.Length(): 30
   TString link;
   TClass* scope = entity->GetClass();
   fHtml->GetHtmlFileName(scope, link);
   link += "#";

   TString mangledName(scope->GetName());
   NameSpace2FileName(mangledName);
   link += mangledName;
   link += ":";

   mangledName = entity->GetName();
   NameSpace2FileName(mangledName);
   link += mangledName;

   TString description;
   if (!comment) {
      description = entity->GetFullTypeName();
      description += " ";
      if (scope) {
         description += scope->GetName();
         description += "::";
      }
      description += entity->GetName();
      comment = description.Data();
   }

   if (comment && !strcmp(comment, entity->GetName()))
      comment = "";

   AddLink(str, link, comment);
}

//______________________________________________________________________________
void TDocOutput::ReferenceEntity(TSubString& str, TDataType* entity, const char* comment /*= 0*/)
{
   // Create a reference to a type documentation page.
   // str encloses the text to create the reference for (e.g. name of instance).
   // comment will be added e.g. as tooltip text.
   // After the reference is put into str.String(), str will enclose the reference
   // and the original text. Example:
   // Input:
   //  str.String(): "a gHtml test"
   //  str.Begin():  2
   //  str.Length(): 5
   // Output:
   //  str.String(): "a <a href="THtml.html">gHtml</a> test"
   //  str.Begin():  2
   //  str.Length(): 30

   TString link("ListOfTypes.html#");
   TString mangledEntity(entity->GetName());
   NameSpace2FileName(mangledEntity);
   link += mangledEntity;

   if (comment && !strcmp(comment, entity->GetName()))
      comment = "";

   AddLink(str, link, comment);
}

//______________________________________________________________________________
void TDocOutput::ReferenceEntity(TSubString& str, TMethod* entity, const char* comment /*= 0*/)
{
   // Create a reference to a method documentation page.
   // str encloses the text to create the reference for (e.g. name of instance).
   // comment will be added e.g. as tooltip text.
   // After the reference is put into str.String(), str will enclose the reference
   // and the original text. Example:
   // Input:
   //  str.String(): "a gHtml test"
   //  str.Begin():  2
   //  str.Length(): 5
   // Output:
   //  str.String(): "a <a href="THtml.html">gHtml</a> test"
   //  str.Begin():  2
   //  str.Length(): 30

   TString link;
   TClass* scope = entity->GetClass();
   fHtml->GetHtmlFileName(scope, link);
   link += "#";

   TString mangledName(scope->GetName());
   NameSpace2FileName(mangledName);
   link += mangledName;
   link += ":";

   mangledName = entity->GetName();
   NameSpace2FileName(mangledName);
   link += mangledName;

   TString description;
   if (!comment && entity->GetClass()) {
      TIter iMeth(scope->GetListOfMethods());
      TMethod* mCand = 0;
      while ((mCand = (TMethod*)iMeth()))
         if (!strcmp(mCand->GetName(), entity->GetName())) {
            if (description.Length()) {
               description += " or overloads";
               break;
            }
            description = mCand->GetPrototype();
         }
      comment = description.Data();
   }

   if (comment && !strcmp(comment, entity->GetName()))
      comment = "";

   AddLink(str, link, comment);
}

//______________________________________________________________________________
Bool_t TDocOutput::ReferenceIsRelative(const char* reference) const
{
   // Check whether reference is a relative reference, and can (or should)
   // be prependen by relative paths. For HTML, check that it doesn't start
   // with "http://" or "https://"

   return !reference || 
      strncmp(reference, "http", 4) ||
      strncmp(reference + 4, "://", 3) && strncmp(reference + 4, "s://", 4);
}

//______________________________________________________________________________
const char* TDocOutput::ReplaceSpecialChars(const char c)
{
// Replace ampersand, less-than and greater-than character, writing to out.
// If 0 is returned, no replacement needs to be done.

   /*
   if (fEscFlag) {
      fEscFlag = kFALSE;
      return buf;
   } else if (c == fEsc) {
      // text.Remove(pos, 1); - NO! we want to keep it nevertheless!
      fEscFlag = kTRUE;
      return buf;
   }

   */
   switch (c) {
      case '<': return "&lt;";
      case '&': return "&amp;";
      case '>': return "&gt;";
   };
   return 0;
}

//______________________________________________________________________________
void TDocOutput::ReplaceSpecialChars(TString& text, Ssiz_t &pos)
{
// Replace ampersand, less-than and greater-than character
//
//
// Input: text - text where replacement will happen,
//        pos  - index of char to be replaced; will point to next char to be 
//               replaced when function returns
//

   const char c = text[pos];
   const char* replaced = ReplaceSpecialChars(c);
   if (replaced) {
         text.Replace(pos, 1, replaced);
         pos += strlen(replaced) - 1;
   }
   ++pos;
}

//______________________________________________________________________________
void TDocOutput::ReplaceSpecialChars(TString& text) {
// Replace ampersand, less-than and greater-than character
//
//
// Input: text - text where replacement will happen,
//
   Ssiz_t pos = 0;
   while (pos < text.Length())
         ReplaceSpecialChars(text, pos);
}

//______________________________________________________________________________
void TDocOutput::ReplaceSpecialChars(std::ostream& out, const char *string)
{
// Replace ampersand, less-than and greater-than characters, writing to out
//
//
// Input: out    - output file stream
//        string - pointer to an array of characters
//

   while (string && *string) {
      const char* replaced = ReplaceSpecialChars(*string);
      if (replaced)
         out << replaced;
      else
         out << *string;
      string++;
   }
}

//______________________________________________________________________________
Bool_t TDocOutput::RunDot(const char* filename, std::ostream* outMap /* =0 */, 
                          EGraphvizTool gvwhat /*= kDot*/) {
// Run filename".dot", creating filename".gif", and - if outMap is !=0,
// filename".map", which gets then included literally into outMap.

   if (!fHtml->HaveDot()) 
      return kFALSE;

   TString runDot;
   switch (gvwhat) {
   case kNeato: runDot = "neato"; break;
   case kFdp: runDot = "fdp"; break;
   case kCirco: runDot = "circo"; break;
   default: runDot = "dot";
   };
   if (fHtml->GetDotDir() && *fHtml->GetDotDir())
      gSystem->PrependPathName(fHtml->GetDotDir(), runDot);
   runDot += " -q1 -Tgif -o";
   runDot += filename;
   runDot += ".gif ";
   if (outMap) {
      runDot += "-Tcmap -o";
      runDot += filename;
      runDot += ".map ";
   }
   runDot += filename;
   runDot += ".dot";

   if (gDebug > 3)
      Info("RunDot", "Running: %s", runDot.Data());
   Int_t retDot = gSystem->Exec(runDot);
   if (gDebug < 4 && !retDot)
      gSystem->Unlink(Form("%s.dot", filename));

   if (!retDot && outMap) {
      ifstream inmap(Form("%s.map", filename));
      std::string line;
      std::getline(inmap, line);
      if (inmap && !inmap.eof()) {
         *outMap << "<map name=\"Map" << gSystem->BaseName(filename) 
            << "\" id=\"Map" << gSystem->BaseName(filename) << "\">" << endl;
         while (inmap && !inmap.eof()) {
            if (line.compare(0, 6, "<area ") == 0) {
               size_t posEndTag = line.find('>');
               if (posEndTag != std::string::npos)
                  line.replace(posEndTag, 1, "/>");
            }
            *outMap << line << endl;
            std::getline(inmap, line);
         }
         *outMap << "</map>" << endl;
      }
      inmap.close();
      if (gDebug < 7)
         gSystem->Unlink(Form("%s.map", filename));
   }

   if (retDot) {
      Error("RunDot", "Error running %s!", runDot.Data());
      fHtml->SetFoundDot(kFALSE);
      return kFALSE;
   }

   return kTRUE;
}


//______________________________________________________________________________
void TDocOutput::WriteHtmlHeader(std::ostream& out, const char *titleNoSpecial, 
                            const char* dir /*=""*/, TClass *cls /*=0*/,
                            const char* header)
{
// Write HTML header
//
// Internal method invoked by the overload

   ifstream addHeaderFile(header);

   if (!addHeaderFile.good()) {
      Warning("THtml::WriteHtmlHeader",
              "Can't open html header file %s\n", header);
      return;
   }

   const TString& charset = GetHtml()->GetCharset();
   TDatime date;
   TString strDate(date.AsString());
   TString line;

   while (!addHeaderFile.eof()) {

      line.ReadLine(addHeaderFile, kFALSE);
      if (addHeaderFile.eof())
         break;

      if (line) {

         if (!cls && (
            line.Index("%CLASS%") != kNPOS ||
            line.Index("%INCFILE%") != kNPOS ||
            line.Index("%SRCFILE%") != kNPOS))
            continue; // skip class line for non-class files

         TString txt(line);

         txt.ReplaceAll("%TITLE%", titleNoSpecial);
         txt.ReplaceAll("%DATE%", strDate);
         txt.ReplaceAll("%RELDIR%", dir);
         txt.ReplaceAll("%CHARSET%", charset);

         if (cls) {
            txt.ReplaceAll("%CLASS%", cls->GetName());
            txt.ReplaceAll("%INCFILE%", fHtml->GetDeclFileName(cls));
            txt.ReplaceAll("%SRCFILE%", fHtml->GetImplFileName(cls));
         }

         out << txt << endl;
      }
   }
}

//______________________________________________________________________________
void TDocOutput::WriteHtmlHeader(std::ostream& out, const char *title, 
                            const char* dir /*=""*/, TClass *cls/*=0*/)
{
// Write HTML header
//
//
// Input: out   - output file stream
//        title - title for the HTML page
//        cls   - current class
//        dir   - relative directory to reach the top 
//                ("" for html doc, "../" for src/*cxx.html etc)
//
// evaluates the Root.Html.Header setting:
// * if not set, the standard header is written. (ROOT)
// * if set, and ends with a "+", the standard header is written and this file 
//   included afterwards. (ROOT, USER)
// * if set but doesn't end on "+" the file specified will be written instead 
//   of the standard header (USER)
//
// Any occurrence of "%TITLE%" (without the quotation marks) in the user 
// provided header file will be replaced by the value of this method's
// parameter "title" before written to the output file. %CLASS% is replaced by
// the class name, %INCFILE% by the header file name as given by
// TClass::GetDeclFileName() and %SRCFILE% by the source file name as given by
// TClass::GetImplFileName(). If the header is written for a non-class page,
// i.e. cls==0, lines containing %CLASS%, %INCFILE%, or %SRCFILE% will be
// skipped.

   TString userHeader = GetHtml()->GetHeader();
   TString noSpecialCharTitle(title);
   ReplaceSpecialChars(noSpecialCharTitle);

   Ssiz_t lenUserHeader = userHeader.Length();
   // standard header output if Root.Html.Header is not set, or it's set and it ends with a "+".
   Bool_t bothHeaders = lenUserHeader > 0 && userHeader[lenUserHeader - 1] == '+';
   if (lenUserHeader == 0 || bothHeaders) {
      TString header("header.html");
      gSystem->PrependPathName(fHtml->GetEtcDir(), header);
      WriteHtmlHeader(out, noSpecialCharTitle, dir, cls, header);
   }

   if (lenUserHeader != 0) {
      if (bothHeaders)
         userHeader.Remove(lenUserHeader - 1);
      WriteHtmlHeader(out, noSpecialCharTitle, dir, cls, userHeader);
   };
}

//______________________________________________________________________________
void TDocOutput::WriteHtmlFooter(std::ostream& out, const char* /*dir*/,
                                 const char* lastUpdate, const char* author,
                                 const char* copyright, const char* footer)
{
// Write HTML footer
//
// Internal method invoked by the overload

   static const char* templateSITags[TDocParser::kNumSourceInfos] = { "%UPDATE%", "%AUTHOR%", "%COPYRIGHT%"};

   TString datimeString;
   if (!lastUpdate || !strlen(lastUpdate)) {
      TDatime date;
      datimeString = date.AsString();
      lastUpdate = datimeString.Data();
   }
   const char* siValues[TDocParser::kNumSourceInfos] = { lastUpdate, author, copyright };

   ifstream addFooterFile(footer);

   if (!addFooterFile.good()) {
      Warning("THtml::WriteHtmlFooter",
              "Can't open html footer file %s\n", footer);
      return;
   }

   TString line;
   while (!addFooterFile.eof()) {

      line.ReadLine(addFooterFile, kFALSE);
      if (addFooterFile.eof())
         break;

      if (!line)
         continue;

      for (Int_t siTag = 0; siTag < (Int_t) TDocParser::kNumSourceInfos; ++siTag) {
         Ssiz_t siPos = line.Index(templateSITags[siTag]);
         if (siPos != kNPOS)
            if (siValues[siTag] && siValues[siTag][0])
               line.Replace(siPos, strlen(templateSITags[siTag]), siValues[siTag]);
            else
               line = ""; // skip e.g. %AUTHOR% lines if no author is set
      }

      out << line << std::endl;
   }

}

//______________________________________________________________________________
void TDocOutput::WriteHtmlFooter(std::ostream& out, const char *dir,
                            const char *lastUpdate, const char *author,
                            const char *copyright)
{
// Write HTML footer
//
//
// Input: out        - output file stream
//        dir        - usually equal to "" or "../", depends of
//                     current file directory position, i.e. if
//                     file is in the fHtml->GetOutputDir(), then dir will be ""
//        lastUpdate - last update string
//        author     - author's name
//        copyright  - copyright note
//
// Allows optional user provided footer to be written. Root.Html.Footer holds 
// the file name for this footer. For details see THtml::WriteHtmlHeader (here,
// the "+" means the user's footer is written in front of Root's!) Occurences 
// of %AUTHOR%, %UPDATE%, and %COPYRIGHT% in the user's file are replaced by 
// their corresponding values (author, lastUpdate, and copyright) before 
// written to out.
// If no author is set (author == "", e.g. for ClassIndex.html") skip the whole
// line of the footer template containing %AUTHOR%. Accordingly for %COPYRIGHT%.

   out << endl;

   TString userFooter = GetHtml()->GetFooter();

   if (userFooter.Length() != 0) {
      TString footer(userFooter);
      if (footer.EndsWith("+"))
         footer.Remove(footer.Length() - 1);
      WriteHtmlFooter(out, dir, lastUpdate, author, copyright, footer);
   };

   if (userFooter.Length() == 0 || userFooter.EndsWith("+")) {
      TString footer("footer.html");
      gSystem->PrependPathName(fHtml->GetEtcDir(), footer);
      WriteHtmlFooter(out, dir, lastUpdate, author, copyright, footer);
   }
}

//______________________________________________________________________________
void TDocOutput::WriteModuleLinks(std::ostream& out)
{
   // Create a dov containing links to all modules

   if (fHtml->GetListOfModules()->GetSize()) {
      out << "<div id=\"indxModules\"><h4>Modules</h4>" << endl;
      // find index chars
      TIter iModule(fHtml->GetListOfModules());
      TModuleDocInfo* module = 0;
      while ((module = (TModuleDocInfo*) iModule()))
         if (module->IsSelected())
            out << "<a href=\"" << module->GetName() << "_Index.html\">" 
                      << module->GetName() << "</a>" << endl;
      out<< "</div><br />" << endl;
   }
}

//______________________________________________________________________________
void TDocOutput::WriteSearch(std::ostream& out)
{
   // Write a search link or a search box, based on THtml::GetSearchStemURL()
   // and THtml::GetSearchEngine(). The first one is preferred.

   // e.g. searchCmd = "http://www.google.com/search?q=%s+site%3A%u";
   const TString& searchCmd = GetHtml()->GetSearchStemURL();
   if (searchCmd.Length()) {
      // create search input
      out << "<script type=\"text/javascript\">" << endl
         << "function onSearch() {" << endl
         << "var s='" << searchCmd <<"';" << endl
         << "var ref=String(document.location.href).replace(/https?:\\/\\//,'').replace(/\\/[^\\/]*$/,'').replace(/\\//g,'%2F');" << endl
         << "window.location.href=s.replace(/%u/ig,ref).replace(/%s/ig,escape(document.searchform.t.value));" << endl
         << "return false;}" << endl
         << "</script><form action=\"javascript:onSearch();\" id=\"searchform\" name=\"searchform\" onsubmit=\"return onSearch()\">" << endl
         << "<input name=\"t\" value=\"Search documentation...\"  onfocus=\"if (document.searchform.t.value=='Search documentation...') document.searchform.t.value='';\"></input>" << endl
         << "<button type=\"submit\">Search</button></form>" << endl;
      return;
   }

   const TString& searchEngine = GetHtml()->GetSearchEngine();
   if (searchEngine.Length())
      // create link to search engine page
      out << "<h2><a href=\"" << searchEngine
          << "\">Search the Class Reference Guide</a></h2>" << endl;
}
