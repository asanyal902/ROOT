// @(#)root/base:$Name$:$Id$
// Author: Rene Brun   28/11/94

/*************************************************************************
 * Copyright (C) 1995-2000, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#ifndef ROOT_TFile
#define ROOT_TFile


//////////////////////////////////////////////////////////////////////////
//                                                                      //
// TFile                                                                //
//                                                                      //
// ROOT file.                                                           //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

#ifndef ROOT_TDirectory
#include "TDirectory.h"
#endif

class TFree;


class TFile : public TDirectory {

protected:
   Int_t       fD;                //File descriptor
   Seek_t      fBEGIN;            //First used byte in file
   Seek_t      fEND;              //Last used byte in file
   Int_t       fVersion;          //File format version
   Int_t       fCompress;         //(=1 file is compressed, 0 otherwise)
   TString     fOption;           //File options
   Char_t      fUnits;            //Number of bytes for file pointers
   Seek_t      fSeekFree;         //Location on disk of free segments structure
   Int_t       fNbytesFree;       //Number of bytes for free segments structure
   Int_t       fWritten;          //Number of objects written so far
   Double_t    fSumBuffer;        //Sum of buffer sizes of objects written so far
   Double_t    fSum2Buffer;       //Sum of squares of buffer sizes of objects written so far
   TList      *fFree;             //Free segments linked list table
   Double_t    fBytesWrite;       //Number of bytes written to this file
   Double_t    fBytesRead;        //Number of bytes read from this file

   static Double_t fgBytesWrite;    //Number of bytes written by all TFile objects
   static Double_t fgBytesRead;     //Number of bytes read by all TFile objects

   static const Int_t  kBegin;
   static const Char_t kUnits;

   void Init(Bool_t create);

   // Interface to basic system I/O routines
   virtual Int_t  SysOpen(const char *pathname, Int_t flags, UInt_t mode);
   virtual Int_t  SysClose(Int_t fd);
   virtual Int_t  SysRead(Int_t fd, void *buf, Int_t len);
   virtual Int_t  SysWrite(Int_t fd, const void *buf, Int_t len);
   virtual Seek_t SysSeek(Int_t fd, Seek_t offset, Int_t whence);
   virtual Int_t  SysSync(Int_t fd);

private:
   TFile(const TFile &);            //Files cannot be copied
   void operator=(const TFile &);

public:
   enum ERelativeTo { kBeg = 0, kCur = 1, kEnd = 2 };

   TFile();
   TFile(const char *fname, Option_t *option="", const char *ftitle="", Int_t compress=1);
   virtual ~TFile();
   virtual void      Close(Option_t *option=""); // *MENU*
   virtual void      Copy(TObject &) { MayNotUse("Copy(TObject &)"); }
   virtual void      Delete(const char *namecycle="");
   virtual void      Draw(Option_t *option="");
   virtual void      FillBuffer(char *&buffer);
   virtual void      Flush();
   Int_t             GetBestBuffer();
   Int_t             GetCompressionLevel() {return fCompress;}
   Float_t           GetCompressionFactor();
   virtual Seek_t    GetEND() {return fEND;}
   Int_t             GetFd() const { return fD; }
   TList            *GetListOfFree() {return fFree;}
   virtual Int_t     GetNfree() {return fFree->GetSize();}
   Option_t         *GetOption() const { return fOption.Data();}
   Double_t          GetBytesRead() const { return fBytesRead; }
   Double_t          GetBytesWritten() const { return fBytesWrite; }
   Int_t             GetVersion() const { return fVersion; }
   Int_t             GetRecordHeader(char *buf, Seek_t first, Int_t maxbytes, Int_t &nbytes, Int_t &objlen, Int_t &keylen);
   virtual Bool_t    IsOpen() const;
   virtual void      ls(Option_t *option="");
   virtual void      MakeFree(Seek_t first, Seek_t last);
   virtual void      Map(); // *MENU*
   virtual void      Paint(Option_t *option="");
   virtual void      Print(Option_t *option="");
   virtual Bool_t    ReadBuffer(char *buf, int len);
   virtual void      ReadFree();
   virtual void      Recover();
   virtual void      Seek(Seek_t offset, ERelativeTo pos = kBeg);
   virtual void      SetCompressionLevel(Int_t level=1);
   virtual void      SetEND(Seek_t last) {fEND = last;}
   virtual void      SetOption(Option_t *option=">") {fOption = option;}
   virtual Int_t     Sizeof() const;
   void              SumBuffer(Int_t bufsize);
   virtual Bool_t    WriteBuffer(const char *buf, int len);
   virtual void      Write(const char *name=0, Int_t opt=0, Int_t bufsiz=0);
   virtual void      WriteFree();
   virtual void      WriteHeader();

   static TFile     *Open(const char *name, Option_t *option="",
                          const char *ftitle="", Int_t compress=1);

   static Double_t   GetFileBytesRead();
   static Double_t   GetFileBytesWritten();

   static void       SetFileBytesRead(Double_t bytes=0);
   static void       SetFileBytesWritten(Double_t bytes=0);

   ClassDef(TFile,1)  //ROOT file
};

R__EXTERN TFile   *gFile;

#endif

