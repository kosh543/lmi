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

#include "Eval.h"

int main()
{
  int i;
  const int n = 10;
  vector<int> a,b,c,d;
  vector<double> e(n);

  for (i = 0; i < n; ++i)
  {
    a.push_back(i);
    b.push_back(2*i);
    c.push_back(3*i);
    d.push_back(i);
  }

  assign(b, 2);
  assign(d, a + b * c);
  a += where(d < 30, b, c);

  assign(e, c);
  e += e - 4 / (c + 1);

  for (i = 0; i < n; ++i)
    {
      cout << " a(" << i << ") = " << a[i]
	   << " b(" << i << ") = " << b[i]
	   << " c(" << i << ") = " << c[i]
	   << " d(" << i << ") = " << d[i]
	   << " e(" << i << ") = " << e[i]
	   << endl;
    }
}

// ACL:rcsinfo
// ----------------------------------------------------------------------
// $RCSfile: Vector.cpp,v $   $Author: swhaney $
// $Revision: 1.8 $   $Date: 2000/01/04 01:39:17 $
// ----------------------------------------------------------------------
// ACL:rcsinfo
