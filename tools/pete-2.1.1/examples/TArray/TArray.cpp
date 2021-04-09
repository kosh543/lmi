// -*- C++ -*-
// ACL:license
// ----------------------------------------------------------------------
// This software and ancillary information (herein called "SOFTWARE")
// called PETE (Portable Expression Template Engine) is
// made available under the terms described here.  The SOFTWARE has been
// approved for release with associated LA-CC Number LA-CC-99-5.
// 
// Unless otherwise indicated, this SOFTWARE has been authored by an
// employee or employees of the University of California, operator of the
// Los Alamos National Laboratory under Contract No.  W-7405-ENG-36 with
// the U.S. Department of Energy.  The U.S. Government has rights to use,
// reproduce, and distribute this SOFTWARE. The public may copy, distribute,
// prepare derivative works and publicly display this SOFTWARE without 
// charge, provided that this Notice and any statement of authorship are 
// reproduced on all copies.  Neither the Government nor the University 
// makes any warranty, express or implied, or assumes any liability or 
// responsibility for the use of this SOFTWARE.
// 
// If SOFTWARE is modified to produce derivative works, such modified
// SOFTWARE should be clearly marked, so as not to confuse it with the
// version available from LANL.
// 
// For more information about PETE, send e-mail to pete@acl.lanl.gov,
// or visit the PETE web page at http://www.acl.lanl.gov/pete/.
// ----------------------------------------------------------------------
// ACL:license

#include "TArray.h"

int main()
{
  TArray a(3,1,2);

  TArray b;

  cout << a << endl;
  cout << b << endl;

  b[0] = 7; b[1] = 33; b[2] = -99;

  TArray c(a);

  cout << c << endl;

  a = 0;

  cout << a << endl;
  cout << b << endl;
  cout << c << endl;

  a = b + c;

  cout << a << endl;

  a.printAssign(b+c*(b + 3*c));
}

// ACL:rcsinfo
// ----------------------------------------------------------------------
// $RCSfile: TArray.cpp,v $   $Author: swhaney $
// $Revision: 1.3 $   $Date: 2000/01/04 01:29:43 $
// ----------------------------------------------------------------------
// ACL:rcsinfo
