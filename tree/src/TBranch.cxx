// @(#)root/tree:$Name:  $:$Id: TBranch.cxx,v 1.6 2000/09/06 07:17:49 brun Exp $
// Author: Rene Brun   12/01/96

/*************************************************************************
 * Copyright (C) 1995-2000, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#include <string.h>
#include <stdio.h>

#include "TROOT.h"
#include "TFile.h"
#include "TBranch.h"
#include "TTree.h"
#include "TBasket.h"
#include "TBrowser.h"
#include "TLeaf.h"
#include "TLeafObject.h"
#include "TLeafB.h"
#include "TLeafC.h"
#include "TLeafI.h"
#include "TLeafF.h"
#include "TLeafS.h"
#include "TLeafD.h"
#include "TMessage.h"
#include "TClonesArray.h"
#include "TVirtualPad.h"

TBranch *gBranch;

R__EXTERN TTree *gTree;

const Int_t kMaxRAM = 10;

ClassImp(TBranch)

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// A TTree is a list of TBranches                                       //                                                                      //
//                                                                      //
// A TBranch supports:                                                  //
//   - The list of TLeaf describing this branch.                        //
//   - The list of TBasket (branch buffers).                            //
//                                                                      //
//       See TBranch structure in TTree.                                //
//                                                                      //
// See also specialized branches:                                       //
//     TBranchObject in case the branch is one object                   //
//     TBranchClones in case the branch is an array of clone objects    //
//////////////////////////////////////////////////////////////////////////

//______________________________________________________________________________
TBranch::TBranch(): TNamed()
{
//*-*-*-*-*-*Default constructor for Branch*-*-*-*-*-*-*-*-*-*
//*-*        ===================================
   fCompress       = 0;
   fBasketSize     = 32000;
   fEntryOffsetLen = 1000;
   fMaxBaskets     = 1000;
   fReadBasket     = 0;
   fReadEntry      = -1;
   fWriteBasket    = 0;
   fEntries        = 0;
   fTotBytes       = 0;
   fZipBytes       = 0;
   fNBasketRAM     = kMaxRAM+1;
   fBasketRAM      = new Int_t[kMaxRAM]; for (Int_t i=0;i<kMaxRAM;i++) fBasketRAM[i] = -1;
   fBasketEntry    = 0;
   fBasketBytes    = 0;
   fBasketSeek     = 0;
   fEntryNumber    = 0;
   fEntryBuffer    = 0;
   fNleaves        = 0;
   fTree           = 0;
   fAddress        = 0;
   fOffset         = 0;
   fDirectory      = 0;
   fFileName       = "";
}

//______________________________________________________________________________
TBranch::TBranch(const char *name, void *address, const char *leaflist, Int_t basketsize, Int_t compress)
    :TNamed(name,leaflist)
{
//*-*-*-*-*-*-*-*-*-*-*-*-*Create a Branch*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
//*-*                =====================
//
//       * address is the address of the first item of a structure
//         or the address of a pointer to an object (see example).
//       * leaflist is the concatenation of all the variable names and types
//         separated by a colon character :
//         The variable name and the variable type are separated by a slash (/).
//         The variable type may be 0,1 or 2 characters. If no type is given,
//         the type of the variable is assumed to be the same as the previous
//         variable. If the first variable does not have a type, it is assumed
//         of type F by default. The list of currently supported types is given below:
//            - C : a character string terminated by the 0 character
//            - B : an 8 bit signed integer (Char_t)
//            - b : an 8 bit unsigned integer (UChar_t)
//            - S : a 16 bit signed integer (Short_t)
//            - s : a 16 bit unsigned integer (UShort_t)
//            - I : a 32 bit signed integer (Int_t)
//            - i : a 32 bit unsigned integer (UInt_t)
//            - F : a 32 bit floating point (Float_t)
//            - D : a 64 bit floating point (Double_t)
//
//         By default, a variable will be copied to the buffer with the number of
//         bytes specified in the type descriptor character. However, if the type
//         consists of 2 characters, the second character is an integer that
//         specifies the number of bytes to be used when copying the variable
//         to the output buffer. Example:
//             X         ; variable X, type Float_t
//             Y/I       : variable Y, type Int_t
//             Y/I2      ; variable Y, type Int_t converted to a 16 bits integer
//
//   See an example of a Branch definition in the TTree constructor.
//
//   Note that in case the data type is an object, this branch can contain
//   only this object.
//
//    Note that this function is invoked by TTree::Branch
//
//*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*

   Int_t i;
   fCompress = compress;
   if (compress == -1 && gTree->GetDirectory()) {
      TFile *bfile = gTree->GetDirectory()->GetFile();
      if (bfile) fCompress = bfile->GetCompressionLevel();
   }
   if (basketsize < 100) basketsize = 100;
   fBasketSize     = basketsize;
   fEntryOffsetLen = 0;
   fMaxBaskets     = 1000;
   fReadBasket     = 0;
   fReadEntry      = -1;
   fWriteBasket    = 0;
   fEntryNumber    = 0;
   fEntryBuffer    = 0;
   fEntries        = 0;
   fTotBytes       = 0;
   fZipBytes       = 0;
   fOffset         = 0;
   fNleaves        = 0;
   fAddress        = (char*)address;
   fNBasketRAM     = kMaxRAM+1;
   fBasketRAM      = new Int_t[kMaxRAM]; for (i=0;i<kMaxRAM;i++) fBasketRAM[i] = -1;
   fBasketEntry    = new Int_t[fMaxBaskets];
   fBasketBytes    = new Int_t[fMaxBaskets];
   fBasketSeek     = new Seek_t[fMaxBaskets];
   fBasketEntry[0] = fEntryNumber;
   for (i=0;i<fMaxBaskets;i++) {
      fBasketBytes[i] = 0;
      fBasketEntry[i] = 0;
      fBasketSeek[i]  = 0;
   }
//*-*- Decode the leaflist (search for : as separator)
   char * varcur  = (char*)leaflist;
   char * var     = varcur;
   Int_t lenvar   = 0;
   Int_t offset   = 0;
   char *leafname = new char[64];
   char *leaftype = new char[32];
   strcpy(leaftype,"F");
   while (1) {
      lenvar++;
      if (*var == ':' || *var == 0) {
         strncpy(leafname,varcur,lenvar-1);
         leafname[lenvar-1] = 0;
         char *ctype = strstr(leafname,"/");
         if (ctype)  { *ctype=0; strcpy(leaftype,ctype+1);}
         TLeaf *leaf = 0;
         if (*leaftype == 'C') {
            leaf = new TLeafC(leafname,leaftype);
         } else if (*leaftype == 'B') {
            leaf = new TLeafB(leafname,leaftype);
         } else if (*leaftype == 'b') {
            leaf = new TLeafB(leafname,leaftype);
            leaf->SetUnsigned();
         } else if (*leaftype == 'S') {
            leaf = new TLeafS(leafname,leaftype);
         } else if (*leaftype == 's') {
            leaf = new TLeafS(leafname,leaftype);
            leaf->SetUnsigned();
         } else if (*leaftype == 'I') {
            leaf = new TLeafI(leafname,leaftype);
         } else if (*leaftype == 'i') {
            leaf = new TLeafI(leafname,leaftype);
            leaf->SetUnsigned();
         } else if (*leaftype == 'F') {
            leaf = new TLeafF(leafname,leaftype);
         } else if (*leaftype == 'D') {
            leaf = new TLeafD(leafname,leaftype);
         }
         if (!leaf) {
           Error("TLeaf","Illegal data type");
           return;
         }
         leaf->SetBranch(this);
         leaf->SetAddress((char*)(fAddress+offset));
         leaf->SetOffset(offset);
         if (leaf->GetLeafCount()) fEntryOffsetLen = 1000;
         if (leaf->InheritsFrom("TLeafC")) fEntryOffsetLen = 1000;
         fNleaves++;
         fLeaves.Add(leaf);
         gTree->GetListOfLeaves()->Add(leaf);
         if (*var == 0) break;
         varcur  = var+1;
         offset += leaf->GetLenType()*leaf->GetLen();
         lenvar  = 0;
      }
      var++;
   }
   delete [] leafname;
   delete [] leaftype;

//*-*-  Create the first basket
   fTree       = gTree;
   fDirectory  = fTree->GetDirectory();
   fFileName   = "";

   TBasket *basket = new TBasket(name,fTree->GetName(),this);
   fBaskets.AddAt(basket,0);
}

//______________________________________________________________________________
TBranch::~TBranch()
{
//*-*-*-*-*-*Default destructor for a Branch*-*-*-*-*-*-*-*-*-*-*-*
//*-*        ===============================

   fLeaves.Delete();
   fBaskets.Delete();
   if (fBasketRAM)   delete [] fBasketRAM;
   if (fBasketEntry) delete [] fBasketEntry;
   if (fBasketBytes) delete [] fBasketBytes;
   if (fBasketSeek)  delete [] fBasketSeek;
   // Warning. Must use FindObject by name instead of fDirectory->GetFile()
   // because two branches<may point to the same file and the file
   // already deleted in the previous branch
   if (fDirectory && fDirectory != fTree->GetDirectory()) {
      TFile *file = (TFile*)gROOT->GetListOfFiles()->FindObject(GetFileName());
      if (file ) delete file;
   }
   fBasketEntry = 0;
   fBasketBytes = 0;
   fBasketSeek  = 0;
   fTree        = 0;
   fDirectory   = 0;
}


//______________________________________________________________________________
void TBranch::Browse(TBrowser *b)
{
   // Browser interface.

   if (fNleaves > 1) {
      fLeaves.Browse(b);
   } else {
      GetTree()->Draw(GetName());
      if (gPad) gPad->Update();
   }
}

//______________________________________________________________________________
void TBranch::DropBaskets()
{
//   Loop on all branch baskets.  Drop all except readbasket

   Int_t i,j;
   TBasket *basket;
   // fast algorithm in case of only a few baskets in memory
   if (fNBasketRAM < kMaxRAM) {
      for (i=0;i<kMaxRAM;i++) {
         j = fBasketRAM[i];
         if (j < 0) continue;
         if (j == fReadBasket || j == fWriteBasket) continue;
         basket = (TBasket*)fBaskets.UncheckedAt(j);
         if (!basket) continue;
         basket->DropBuffers();
         GetListOfBaskets()->RemoveAt(j);
         delete basket;
         fBasketRAM[i] = -1;
         fNBasketRAM--;
      }
      if (fNBasketRAM < 0) {
         printf("ERROR, fNBasketRAM =%d\n",fNBasketRAM);
         fNBasketRAM = 0;
      }
      i = 0;
      for (j=0;j<kMaxRAM;j++) {
         if (fBasketRAM[j] < 0) continue;
         fBasketRAM[i] = fBasketRAM[j];
         i++;
      }
      return;
   }

   //general algorithm looping on the full baskets table.
   Int_t nbaskets = GetListOfBaskets()->GetEntriesFast();
   fNBasketRAM = 0;
   for (j=0;j<nbaskets-1;j++)  {
      basket = (TBasket*)fBaskets.UncheckedAt(j);
      if (!basket) continue;
      if (fNBasketRAM < kMaxRAM) fBasketRAM[fNBasketRAM] = j;
      fNBasketRAM++;
      if (j == fReadBasket || j == fWriteBasket) continue;
      basket->DropBuffers();
      GetListOfBaskets()->RemoveAt(j);
      delete basket;
      fNBasketRAM--;
      fBasketRAM[fNBasketRAM] = -1;
      if (!fTree->MemoryFull(0)) break;
   }
}

//______________________________________________________________________________
Int_t TBranch::Fill()
{
//*-*-*-*-*-*-*-*Loop on all leaves of this branch to fill Basket buffer*-*-*
//*-*            =======================================================

   if (TestBit(kDoNotProcess)) return 0;

   TBasket *basket = GetBasket(fWriteBasket);
   if (!basket) return 0;
   TBuffer *buf    = basket->GetBufferRef();

//*-*- Fill basket buffer
   Int_t nsize  = 0;
   if (buf->IsReading()) {
      basket->SetWriteMode();
   }
   buf->ResetMap();
   Int_t lold = buf->Length();
   Int_t objectStart = 0;
   if ( fEntryBuffer!=0 ) {
     if ( fEntryBuffer->IsA() == TMessage::Class() ) {
       objectStart = 8;
     }
     // We are required to copy starting at the version number (so not
     // including the class name.
     // See if byte count is here, if not it class still be a newClass
     const UInt_t kNewClassTag       = 0xFFFFFFFF;
     const UInt_t kByteCountMask     = 0x40000000;  // OR the byte count with this
     UInt_t tag, startpos = fEntryBuffer->Length();
     fEntryBuffer->SetBufferOffset(objectStart);
     *fEntryBuffer >> tag;
     if ( tag & kByteCountMask ) {
       *fEntryBuffer >> tag;
     }
     if ( tag == kNewClassTag ) {
       char s[80];

       fEntryBuffer->ReadString(s, 80);
     } else {
       fEntryBuffer->SetBufferOffset(objectStart);
     }
     objectStart = fEntryBuffer->Length();
     fEntryBuffer->SetBufferOffset(startpos);

     basket->Update(lold, objectStart - fEntryBuffer->GetBufferDisplacement());
   } else  basket->Update(lold);
   fEntries++;
   fEntryNumber++;
   if ( fEntryBuffer!=0 ) {

     UInt_t len, startpos = fEntryBuffer->Length();
     if (startpos > UInt_t(objectStart)) {
       // We assume this buffer have just been directly filled
       // the current position in the buffer indicates the end of the object!
       len = fEntryBuffer->Length() - objectStart;
     } else {
       // The buffer have been acquired either via TSocket or via
       // TBuffer::SetBuffer(newloc,newsize)
       // Only the actual size of the memory buffer gives us an hint about where
       // the object ends.
       len = fEntryBuffer->BufferSize() - objectStart;
     }
     buf->WriteBuf( fEntryBuffer->Buffer() + objectStart , len );

   } else {
     for (Int_t i=0;i<fNleaves;i++) {
       TLeaf *leaf = (TLeaf*)fLeaves.UncheckedAt(i);
       leaf->FillBasket(*buf);
     }
   }
   Int_t lnew   = buf->Length();
   Int_t nbytes = lnew - lold;
   if (fEntryOffsetLen) {
      Int_t nevbuf = basket->GetNevBuf();
      nsize = nevbuf*sizeof(Int_t); //total size in bytes of EntryOffset table
   } else {
      if (!basket->GetNevBufSize()) basket->SetNevBufSize(nbytes);
   }

//*-*- Should we create a new basket?
   if ( (lnew +2*nsize +nbytes >= fBasketSize) ) { //NOTE: we should not need that|| (fEntryBuffer!=0) ) {
      Int_t nout  = basket->WriteBuffer();    //  Write buffer
      fBasketBytes[fWriteBasket]  = basket->GetNbytes();
      fBasketSeek[fWriteBasket]   = basket->GetSeekKey();
      Int_t addbytes = basket->GetObjlen() + basket->GetKeylen() ;
      if (fDirectory != gROOT && fDirectory->IsWritable()) {
         delete basket;
         fBaskets[fWriteBasket] = 0;
      }
      fZipBytes += nout;
      fTotBytes += addbytes;
      fTree->AddTotBytes(addbytes);
      fTree->AddZipBytes(nout);
      basket = new TBasket(GetName(),fTree->GetName(),this);   //  create a new basket
      fWriteBasket++;
      fBaskets.AddAtAndExpand(basket,fWriteBasket);
      if (fWriteBasket >= fMaxBaskets) {
           //Increase BasketEntry buffer of a minimum of 10 locations
           // and a maximum of 50 per cent of current size
         Int_t newsize = TMath::Max(10,Int_t(1.5*fMaxBaskets));
         Int_t *newbuf = (Int_t*)TStorage::ReAlloc(fBasketEntry,
                         newsize*sizeof(Int_t),fMaxBaskets*sizeof(Int_t));
         fBasketEntry  = newbuf;
                newbuf = (Int_t*)TStorage::ReAlloc(fBasketBytes,
                         newsize*sizeof(Int_t),fMaxBaskets*sizeof(Int_t));
         fBasketBytes  = newbuf;
         Seek_t *nseek = (Seek_t*)TStorage::ReAlloc(fBasketSeek,
                         newsize*sizeof(Seek_t),fMaxBaskets*sizeof(Seek_t));
         fBasketSeek   = nseek;
         fMaxBaskets   = newsize;
      }
      fBasketEntry[fWriteBasket] = fEntryNumber;
      fBasketBytes[fWriteBasket] = 0;
      fBasketSeek[fWriteBasket]  = 0;
   }
   return nbytes;
}

//______________________________________________________________________________
TBasket *TBranch::GetBasket(Int_t basketnumber)
{
//*-*-*-*-*Return pointer to basket basketnumber in this Branch*-*-*-*-*-*
//*-*      ====================================================

   static Int_t nerrors = 0;

      // reference to an existing basket in memory ?
   if (basketnumber <0 || basketnumber > fWriteBasket) return 0;
   TBasket *basket = (TBasket*)fBaskets.UncheckedAt(basketnumber);
   if (basket) return basket;

      // must create a new basket
   gBranch = this;

     // create/decode basket parameters from buffer
   TDirectory *cursav = gDirectory;
   TFile *file = GetFile(0);
   basket = new TBasket();
//printf("branch:%s, basketnumber=%d, seek=%d\n",GetName(),basketnumber,fBasketSeek[basketnumber]);
   if (fBasketBytes[basketnumber] == 0) {
      fBasketBytes[basketnumber] = basket->ReadBasketBytes(fBasketSeek[basketnumber],file);
   }
   basket->ReadBasketBuffers(fBasketSeek[basketnumber],fBasketBytes[basketnumber],file);
   if (basket->GetSeekKey() != fBasketSeek[basketnumber]) {
      cursav->cd();
      nerrors++;
      if (nerrors > 10) return 0;
      if (nerrors == 10) {
         printf(" file probably overwritten: stopping reporting error messages\n");
         if (fBasketSeek[basketnumber] > 2000000000) {
            printf("===>File is more than 2 Gigabytes\n");
            return 0;
         }
         if (fBasketSeek[basketnumber] > 1000000000) {
            printf("===>Your file is may be bigger than the maximum file size allowed on your system\n");
            printf("    Check your AFS maximum file size limit for example\n");
            return 0;
         }
      }
      Error("GetBasket","File problem at address:%d, basket seekkey=%d, branch:%s",fBasketSeek[basketnumber],basket->GetSeekKey(),GetName());
      return 0;
   }

   cursav->cd();
   fBaskets[basketnumber] = basket;
   if (fNBasketRAM < kMaxRAM) fBasketRAM[fNBasketRAM] = basketnumber;
   fNBasketRAM++;
   return basket;
}

//______________________________________________________________________________
Seek_t TBranch::GetBasketSeek(Int_t basketnumber)
{
//*-*-*-*-*Return address of basket in the file*-*-*-*-*-*
//*-*      ====================================

   if (basketnumber <0 || basketnumber > fWriteBasket) return 0;
   return fBasketSeek[basketnumber];
}


//______________________________________________________________________________
Int_t TBranch::GetEntry(Int_t entry, Int_t getall)
{
//*-*-*-*-*-*Read all leaves of entry and return total number of bytes*-*-*
//*-*        =========================================================

   if (TestBit(kDoNotProcess) && !getall) return 0;
   if (fReadEntry == entry) return 1;
   if (entry < 0 || entry >= fEntryNumber) return 0;
   Int_t nbytes;
   Int_t first  = fBasketEntry[fReadBasket];
   Int_t last;
   if (fReadBasket == fWriteBasket) last = fEntryNumber - 1;
   else                             last = fBasketEntry[fReadBasket+1] - 1;
//
//      Are we still in the same ReadBasket?
   if (entry < first || entry > last) {
      fReadBasket = TMath::BinarySearch(fWriteBasket+1, fBasketEntry, entry);
      first       = fBasketEntry[fReadBasket];
   }

//     We have found the basket containing this entry.
//     make sure basket buffers are in memory.
   TBasket *basket = (TBasket*)fBaskets.UncheckedAt(fReadBasket);
   if (!basket) {
      basket = GetBasket(fReadBasket);
      if (!basket) return 0;
   }
   TBuffer *buf    = basket->GetBufferRef();
//     Set entry offset in buffer and read data from all leaves
   buf->ResetMap();
   if (!buf->IsReading()) {
      basket->SetReadMode();
   }

   Int_t bufbegin;
   Int_t *entryOffset = basket->GetEntryOffset();
   if (entryOffset) bufbegin = entryOffset[entry-first];
   else             bufbegin = basket->GetKeylen() + (entry-first)*basket->GetNevBufSize();
   buf->SetBufferOffset(bufbegin);
   Int_t *displacement = basket->GetDisplacement();
   if (displacement) buf->SetBufferDisplacement(displacement[entry-first]);
   else buf->SetBufferDisplacement();

   for (Int_t i=0;i<fNleaves;i++) {
      TLeaf *leaf = (TLeaf*)fLeaves.UncheckedAt(i);
      leaf->ReadBasket(*buf);
   }
   nbytes = buf->Length() - bufbegin;
   fReadEntry = entry;
   return nbytes;
}


//______________________________________________________________________________
Int_t TBranch::GetEntryExport(Int_t entry, Int_t getall, TClonesArray *list, Int_t nentries)
{
//*-*-*-*-*-*Read all leaves of entry and return total number of bytes*-*-*
//*-* export buffers to real objects in the TClonesArray list.
//*-*

   if (TestBit(kDoNotProcess)) return 0;
   if (fReadEntry == entry) return 1;
   if (entry < 0 || entry >= fEntryNumber) return 0;
   Int_t nbytes;
   Int_t first  = fBasketEntry[fReadBasket];
   Int_t last;
   if (fReadBasket == fWriteBasket) last = fEntryNumber - 1;
   else                             last = fBasketEntry[fReadBasket+1] - 1;
//
//      Are we still in the same ReadBasket?
   if (entry < first || entry > last) {
      fReadBasket = TMath::BinarySearch(fWriteBasket+1, fBasketEntry, entry);
      first       = fBasketEntry[fReadBasket];
   }

//     We have found the basket containing this entry.
//     make sure basket buffers are in memory.
   TBasket *basket = GetBasket(fReadBasket);
   if (!basket) return 0;
   TBuffer *buf    = basket->GetBufferRef();
//     Set entry offset in buffer and read data from all leaves
   if (!buf->IsReading()) {
      basket->SetReadMode();
   }
//   Int_t bufbegin = basket->GetEntryPointer(entry-first);
   Int_t bufbegin;
   Int_t *entryOffset = basket->GetEntryOffset();
   if (entryOffset) bufbegin = entryOffset[entry-first];
   else             bufbegin = basket->GetKeylen() + (entry-first)*basket->GetNevBufSize();
   buf->SetBufferOffset(bufbegin);
   Int_t *displacement = basket->GetDisplacement();
   if (displacement) buf->SetBufferDisplacement(displacement[entry-first]);
   else buf->SetBufferDisplacement();

   TLeaf *leaf = (TLeaf*)fLeaves.UncheckedAt(0);
   leaf->ReadBasketExport(*buf,list,nentries);

   nbytes = buf->Length() - bufbegin;
   fReadEntry = entry;

   return nbytes;
}

//______________________________________________________________________________
TFile *TBranch::GetFile(Int_t mode)
{
//  Return pointer to the file where branch buffers reside

   if (fDirectory) return fDirectory->GetFile();
   TFile *file;

   // check if a file with this name is in the list of Root files
   file = (TFile*)gROOT->GetListOfFiles()->FindObject(fFileName.Data());
   if (file) {
      fDirectory = (TDirectory*)file;
      return file;
   }

   // Open file (new file if mode = 1)
   if (mode) file = TFile::Open(fFileName.Data(),"recreate");
   else      file = TFile::Open(fFileName.Data());
   if (file->IsZombie()) {delete file; return 0;}
   fDirectory = (TDirectory*)file;
   return file;
}

//______________________________________________________________________________
TLeaf *TBranch::GetLeaf(const char *name)
{
//*-*-*-*-*-*Return pointer to the 1st Leaf named name in thisBranch-*-*-*-*-*
//*-*        =======================================================

   Int_t i;
   for (i=0;i<fNleaves;i++) {
      TLeaf *leaf = (TLeaf*)fLeaves.UncheckedAt(i);
      if (!strcmp(leaf->GetName(),name)) return leaf;
   }
   return 0;
}


//______________________________________________________________________________
Int_t TBranch::GetRow(Int_t)
{
//*-*-*-*-*Return all elements of one row unpacked in internal array fValues*-*
//*-*      =================================================================

   return 1;
}


//______________________________________________________________________________
Bool_t TBranch::IsAutoDelete()
{
//*-*-*-*-*Return TRUE if an existing object in a TBranchObject must be deleted
//*-*      ==================================================

   return TestBit(kAutoDelete);
}


//______________________________________________________________________________
Bool_t TBranch::IsFolder() const
{
//*-*-*-*-*Return TRUE if more than one leaf, FALSE otherwise*-*
//*-*      ==================================================

   if (fNleaves > 1) return kTRUE;
   else              return kFALSE;
}

//______________________________________________________________________________
void TBranch::Print(Option_t *)
{
//*-*-*-*-*-*-*-*-*-*-*-*Print TBranch parameters*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
//*-*                    ========================

  const int kLINEND = 77;
  Float_t cx = 1;
  int aLength = strlen (GetTitle());
  aLength += (aLength / 54 + 1) * 80 + 100;
  if (aLength < 200) aLength = 200;
  char *bline = new char[aLength];
  if (fZipBytes) cx = fTotBytes/fZipBytes;
  sprintf(bline,"*Branch  :%-9s : %-54s *",GetName(),GetTitle());
  if (strlen(bline) > UInt_t(kLINEND)) {
     int len=strlen(GetTitle());
     char *tmp = new char[strlen(bline)+1];
     strcpy(tmp, GetTitle());
     sprintf(bline,"*Branch  :%-9s : ",GetName());
     int pos = strlen (bline);
     int npos = pos;
     int beg=0, end;
     while (beg < len) {
        for (end=beg+1; end < len-1; end ++)
          if (tmp[end] == ':')  break;
        if (npos + end-beg+1 >= 78) {
           while (npos < kLINEND) {
              bline[pos ++] = ' ';
              npos ++;
           }
           bline[pos ++] = '*';
           bline[pos ++] = '\n';
           bline[pos ++] = '*';
           npos = 1;
           for (; npos < 22; npos ++)
               bline[pos ++] = ' ';
           bline[pos-2] = '|';
        }
        for (int n = beg; n <= end; n ++)
           bline[pos+n-beg] = tmp[n];
        pos += end-beg+1;
        npos += end-beg+1;
        beg = end+1;
     }
     while (npos < kLINEND) {
        bline[pos ++] = ' ';
        npos ++;
     }
     bline[pos ++] = '*';
     bline[pos] = '\0';
     delete[] tmp;
  }
  Printf(bline);
  if (fTotBytes > 2e9) {
     Printf("*Entries :%9d : Total  Size=%11g bytes  File Size  = %10d *",Int_t(fEntries),fTotBytes,Int_t(fZipBytes));
  } else {
     Printf("*Entries :%9d : Total  Size=%11d bytes  File Size  = %10d *",Int_t(fEntries),Int_t(fTotBytes),Int_t(fZipBytes));
  }
  Printf("*Baskets :%9d : Basket Size=%11d bytes  Compression= %6.2f     *",fWriteBasket,fBasketSize,cx);
  Printf("*............................................................................*");
  delete [] bline;
}


//______________________________________________________________________________
void TBranch::ReadBasket(TBuffer &)
{
//*-*-*-*-*-*-*-*Loop on all leaves of this branch to read Basket buffer*-*-*
//*-*            =======================================================

//   fLeaves->ReadBasket(basket);
}

//______________________________________________________________________________
void TBranch::Reset(Option_t *)
{
//*-*-*-*-*-*-*-*Reset a Branch*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
//*-*            ====================
//
//    Existing buffers are deleted
//    Entries, max and min are reset
//

   fBaskets.Delete();
   if (fBasketEntry) delete [] fBasketEntry;
   if (fBasketBytes) delete [] fBasketBytes;
   if (fBasketSeek)  delete [] fBasketSeek;
   fBasketEntry    = 0;
   fReadBasket     = 0;
   fReadEntry      = -1;
   fWriteBasket    = 0;
   fEntries        = 0;
   fTotBytes       = 0;
   fZipBytes       = 0;
   fEntryNumber    = 0;
   fBasketEntry    = new Int_t[fMaxBaskets];
   fBasketBytes    = new Int_t[fMaxBaskets];
   fBasketSeek     = new Seek_t[fMaxBaskets];
   fBasketEntry[0] = fEntryNumber;
   for (Int_t i=0;i<fMaxBaskets;i++) {
      fBasketBytes[i] = 0;
      fBasketEntry[i] = 0;
      fBasketSeek[i]  = 0;
   }
   TBasket *basket = new TBasket(GetName(),fTree->GetName(),this);
   fBaskets.AddAt(basket,0);
}

//______________________________________________________________________________
void TBranch::SetAddress(void *add)
{
//*-*-*-*-*-*-*-*Set address of this branch*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
//*-*            ====================
//

   fReadEntry = -1;
   fAddress = (char*)add;
   Int_t i,offset;
   for (i=0;i<fNleaves;i++) {
      TLeaf *leaf = (TLeaf*)fLeaves.UncheckedAt(i);
      offset = leaf->GetOffset();
      if (TestBit(kIsClone)) offset = 0;
      leaf->SetAddress(fAddress+offset);
   }
}

//______________________________________________________________________________
void TBranch::SetAutoDelete(Bool_t autodel)
{
//*-*-*-*-*-*-*-*Set the AutoDelete bit
//*-*            ====================
// this bit is used by TBranchObject::ReadBasket to decide if an object
// referenced by a TBranchObject must be deleted or not before reading
// a new entry
// if autodel is kTRUE, this existing object will be deleted, a new object
//    created by the default constructor, then object->Streamer
// if autodel is kFALSE, the existing object is not deleted. Root assumes
//    that the user is taking care of deleting any internal object or array
//    This can be done in Streamer itself.

   if (autodel) SetBit(kAutoDelete,1);
   else         SetBit(kAutoDelete,0);
}

//______________________________________________________________________________
void TBranch::SetBufferAddress(TBuffer *buf)
{
   // Set address of this branch directly from a TBuffer to avoid streaming.

   // Check this is possible
   if ( (fNleaves != 1)
       || (strcmp("TLeafObject",fLeaves.UncheckedAt(0)->ClassName())!=0) ) {
      Error("TBranch::SetAddress","Filling from a TBuffer can only be done with a not split object branch.  Request ignored.");
   } else {
      fReadEntry = -1;
      fEntryBuffer = buf;
   }
}

//______________________________________________________________________________
void TBranch::SetCompressionLevel(Int_t level)
{
//*-*-*-*-*-*-*-*Set the branch/subbranches compression level
//*-*            ============================================

   fCompress = level;
   Int_t nb = fBranches.GetEntriesFast();

   for (Int_t i=0;i<nb;i++) {
      TBranch *branch = (TBranch*)fBranches.UncheckedAt(i);
      branch->SetCompressionLevel(level);
   }
}

//______________________________________________________________________________
void TBranch::SetFile(TFile *file)
{
//  Set file where this branch writes/reads its buffers
//
   if (file == 0) file = fTree->GetCurrentFile();
   fDirectory = (TDirectory*)file;
   if (file == fTree->GetCurrentFile()) fFileName = "";
   else                                 fFileName = file->GetName();
}

//______________________________________________________________________________
void TBranch::SetFile(const char *fname)
{
//  Set file where this branch writes/reads its buffers
//  By default the branch buffers reside in the file where the Tree was created.
//  Branches of the same Tree may be on different files.
//
//  The Root file will be connected only when necessary.
//  If called by TBranch::Fill (via TBasket::WriteFile), the file
//  will be created with the option "recreate".
//  If called by TBranch::GetEntry (via TBranch::GetBasket), the file
//  will be open in read mode.
//
//  To open a file in "update" mode, use TBranch::SetFile(TFile *file).

   fFileName  = fname;
   fDirectory = 0;
}

//_______________________________________________________________________
void TBranch::Streamer(TBuffer &b)
{
//*-*-*-*-*-*-*-*-*Stream a class object*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
//*-*              =========================================
   UInt_t R__s, R__c;
   if (b.IsReading()) {
      gBranch = this;
      fTree = gTree;
      gROOT->SetReadingObject(kTRUE);
      Version_t v = b.ReadVersion(&R__s, &R__c);
      TNamed::Streamer(b);
      b >> fCompress;
      b >> fBasketSize;
      b >> fEntryOffsetLen;
      b >> fMaxBaskets;
      b >> fWriteBasket;
      b >> fEntryNumber;
      b >> fEntries;
      b >> fTotBytes;
      b >> fZipBytes;
      b >> fOffset;
      fAddress = 0;
      fBranches.Streamer(b);
      gBranch = this;  // must be set again, was changed in previous statement
      fLeaves.Streamer(b);
      fBaskets.Streamer(b);
      fNleaves = fLeaves.GetEntriesFast();
      fBasketEntry = new Int_t[fMaxBaskets];
      Int_t n  = b.ReadArray(fBasketEntry);
      fBasketBytes = new Int_t[fMaxBaskets];
      if (v > 4) {
         n  = b.ReadArray(fBasketBytes);
      } else {
         for (n=0;n<fMaxBaskets;n++) fBasketBytes[n] = 0;
      }
      if (v < 2) {
         fBasketSeek = new Seek_t[fMaxBaskets];
         for (n=0;n<fWriteBasket;n++) {
            fBasketSeek[n] = GetBasket(n)->GetSeekKey();
         }
      } else {
         fBasketSeek = new Seek_t[fMaxBaskets];
         n  = b.ReadArray(fBasketSeek);
      }
      fDirectory = gDirectory;
      if (v > 2) {
         fFileName.Streamer(b);
         if (fFileName.Length() != 0) fDirectory = 0;
      }
      if (v < 4) SetAutoDelete(kTRUE);
      gROOT->SetReadingObject(kFALSE);
      b.CheckByteCount(R__s, R__c, TBranch::IsA());
   } else {
      R__c = b.WriteVersion(TBranch::IsA(), kTRUE);
      TNamed::Streamer(b);
      b << fCompress;
      b << fBasketSize;
      b << fEntryOffsetLen;
      b << fMaxBaskets;
      b << fWriteBasket;
      b << fEntryNumber;
      b << fEntries;
      b << fTotBytes;
      b << fZipBytes;
      b << fOffset;
      fBranches.Streamer(b);
      fLeaves.Streamer(b);
      fBaskets.Streamer(b);
      b.WriteArray(fBasketEntry,fMaxBaskets);
      b.WriteArray(fBasketBytes,fMaxBaskets);
      b.WriteArray(fBasketSeek, fMaxBaskets);
      fFileName.Streamer(b);

         // if branch is in a separate file save this branch
         // as an independent key
      if (fDirectory && fDirectory != fTree->GetDirectory()) {
         TDirectory *cursav = gDirectory;
         fDirectory->cd();
         fDirectory = 0;  // to avoid recusive calls
         Write();
         fDirectory = gDirectory;
         cursav->cd();
      }
      b.SetByteCount(R__c, kTRUE);
   }
}
