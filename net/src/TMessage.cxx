// @(#)root/net:$Name$:$Id$
// Author: Fons Rademakers   19/12/96

/*************************************************************************
 * Copyright (C) 1995-2000, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// TMessage                                                             //
//                                                                      //
// Message buffer class used for serializing objects and sending them   //
// over a network. This class inherits from TBuffer the basic I/O       //
// serializer.                                                          //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

#include "TMessage.h"
#include "Bytes.h"

ClassImp(TMessage)

//______________________________________________________________________________
TMessage::TMessage(UInt_t what) : TBuffer(kWrite)
{
   // Create a TMessage object for storing objects. The "what" integer
   // describes the type of message. Predifined ROOT system message types
   // can be found in MessageTypes.h. Make sure your own message types are
   // unique from the ROOT defined message types (i.e. 0 - 10000 are
   // reserved by ROOT). In case you OR "what" with kMESS_ACK, the message
   // will wait for an acknowledgement from the remote side. This makes
   // the sending process synchronous.

   // space at the beginning of the message to contain the message length
   UInt_t   reserved = 0;
   *this << reserved;

   fClass = 0;
   fWhat  = what;
   *this << what;
}

//______________________________________________________________________________
TMessage::TMessage(void *buf, Int_t bufsize) : TBuffer(kRead, bufsize, buf)
{
   // Create a TMessage object for reading objects. The objects will be
   // read from buf. Use the What() method to get the message type.

   // pretend there is space at the beginning for the message length
   // this word will never be accessed, only used for offset calculation
   fBuffer -= sizeof(UInt_t);

   *this >> fWhat;

   if (fWhat == kMESS_OBJECT) {
      InitMap();
      fClass = ReadClass();     // get first the class stored in message
      Reset();
   } else
      fClass = 0;
}

//______________________________________________________________________________
TMessage::~TMessage()
{
   // TMessage dtor. In case we were reading reset fBuffer.

   if (IsReading())
      fBuffer += sizeof(UInt_t);
}

//______________________________________________________________________________
void TMessage::Reset()
{
   // Reset the message buffer so we can use (i.e. fill) it again.

   SetBufferOffset(sizeof(fWhat) + sizeof(UInt_t));
   ResetMap();
}

//______________________________________________________________________________
void TMessage::SetLength() const
{
   // Set the message length at the beginning of the message buffer.
   // This method is only called by TSocket::Send().

   if (IsWriting()) {
      char *buf = Buffer();
      tobuf(buf, (UInt_t)(Length() - sizeof(UInt_t)));
   }
}

//______________________________________________________________________________
void TMessage::SetWhat(UInt_t what)
{
   // Using this method one can change the message type a posteriory.

   char *buf = Buffer();
   if (IsWriting())
      buf += sizeof(UInt_t);   // skip reserved space
   tobuf(buf, what);
   fWhat = what;
}
