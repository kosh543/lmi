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

#ifndef PETE_EXAMPLES_VECTOR_EVAL_H
#define PETE_EXAMPLES_VECTOR_EVAL_H

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------

#include <iostream.h>
#include <vector.h>
#include "PETE/PETE.h"
#include "VectorOperators.h"

//-----------------------------------------------------------------------------
// This file contains several class definitions that are used to evaluate
// expressions containing STL vectors.  The main function defined at the end
// is evaluate(lhs,op,rhs), which allows the syntax:
// vector<int> a,b,c;
// evaluate(a,OpAssign(),b+c);
//
// evaluate() is called by all the global assignment operator functions
// defined in VectorOperators.h
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// We need to specialize CreateLeaf<T> for our class, so that operators
// know what to stick in the leaves of the expression tree.
//-----------------------------------------------------------------------------

template<class T, class Allocator>
struct CreateLeaf<vector<T, Allocator> >
{
  typedef Reference<vector<T> > Leaf_t;
  inline static
  Leaf_t make(const vector<T, Allocator> &a) { return Leaf_t(a); }
};

//-----------------------------------------------------------------------------
// We need to write a functor that is capable of comparing the size of
// the vector with a stored value. Then, we supply LeafFunctor specializations
// for Scalar<T> and STL vector leaves.
//-----------------------------------------------------------------------------

class SizeLeaf
{
public:

  SizeLeaf(int s) : size_m(s) { }
  SizeLeaf(const SizeLeaf &model) : size_m(model.size_m) { }
  bool operator()(int s) const { return size_m == s; }
  
private:
  
  int size_m;
  
};

template<class T>
struct LeafFunctor<Scalar<T>, SizeLeaf>
{
  typedef bool Type_t;
  inline static
  bool apply(const Scalar<T> &, const SizeLeaf &) 
  {
    // Scalars always conform.
    
    return true;
  }
};

template<class T, class Allocator>
struct LeafFunctor<vector<T, Allocator>, SizeLeaf>
{
  typedef bool Type_t;
  inline static
  bool apply(const vector<T, Allocator> &v, const SizeLeaf &s) 
  {
    return s(v.size());
  }
};

//-----------------------------------------------------------------------------
// EvalLeaf1 is used to evaluate expression with vectors.
// (It's already defined for Scalar values.)
//-----------------------------------------------------------------------------

template<class T, class Allocator>
struct LeafFunctor<vector<T, Allocator>,EvalLeaf1>
{
  typedef T Type_t;
  inline static
  Type_t apply(const vector<T, Allocator>& vec,const EvalLeaf1 &f)
  {
    return vec[f.val1()];
  }
};

//-----------------------------------------------------------------------------
// Loop over vector and evaluate the expression at each location.
//-----------------------------------------------------------------------------

template<class T, class Allocator, class Op, class RHS>
inline void evaluate(vector<T, Allocator> &lhs, const Op &op, 
  const Expression<RHS> &rhs)
{
  if (forEach(rhs, SizeLeaf(lhs.size()), AndCombine()))
    {
      // We get here if the vectors on the RHS are the same size as those on
      // the LHS.
      
      for (int i = 0; i < lhs.size(); ++i)
        {
          // The actual assignment operation is performed here.
          // PETE operator tags all define operator() to perform the operation.
          // (In this case op performs an assignment.) forEach is used 
          // to compute the rhs value.  EvalLeaf1 gets the
          // values at each node using random access, and the tag 
          // OpCombine tells forEach to use the operator tags in the expression 
          // to combine values together.

          op(lhs[i], forEach(rhs, EvalLeaf1(i), OpCombine()));
        }
    }
  else
    {
      cerr << "Error: LHS and RHS don't conform." << endl;
      exit(1);
    }
}

#endif // PETE_EXAMPLES_VECTOR_EVAL_H

// ACL:rcsinfo
// ----------------------------------------------------------------------
// $RCSfile: Eval.h,v $   $Author: swhaney $
// $Revision: 1.11 $   $Date: 2000/01/04 01:39:17 $
// ----------------------------------------------------------------------
// ACL:rcsinfo
