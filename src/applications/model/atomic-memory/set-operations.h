/*
The MIT License (MIT)

Copyright (c) 2016 Dongjae (David) Kim

		Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
		to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
		copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

		The above copyright notice and this permission notice shall be included in all
		copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
		AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
 */

#ifndef SetOperation_hpp
#define SetOperation_hpp

#include <stdio.h>
#include <set>
#include <string>
#include <sstream>

using namespace std;

template < class TargetClass >
class SetOperation
{
public:
	SetOperation();

    // Goal: Return union between two std::sets
    // Output: Set union
    //
    static std::set<TargetClass> unite(const std::set<TargetClass> &set1,
                                       const std::set<TargetClass> &set2);
    
    // Goal: Return intersection between two std::sets
    // Output: Set intersection
    //
    static std::set<TargetClass> intersect(const std::set<TargetClass> &set1,
                                           const std::set<TargetClass> &set2);
    
    // Goal: Find the complement of a std::set
    // Input: The universe of elements and the
    // std::set of which we want to ge the complement
    // Ourput: the complement
    static std::set<TargetClass> setComplement(const std::set<TargetClass> &U,
                                               const std::set<TargetClass> &S);
    
    // Goal: Check if the std::set given as first parameter is
    // a substd::set of the second std::set.
    // Output: 1 if first std::set is a substd::set of a second, 0 otherwise
    //
    static int subset(const std::set<TargetClass> &set1,
                      const std::set<TargetClass> &set2);
    
    //Goal: print the elements in a std::set
    //Input: (a) The std::set we want to print
    //       (b) name of the std::set
    //       (c) prefix of each element of the std::set
    static std::string printSet(const std::set<TargetClass> &S,
                                 const std::string &name,
                                 const std::string &prefix);
    
    // Goal: Get all the subsets of length k of a given set
    // Input: (a) The std::set of which we need the subsets
    //        (b) The index k indicating the size of the subsets
    // Output: A vector with all the subsets of certain size
    //
    static std::set< std::set<TargetClass> > subsetsSizeK(const std::set<TargetClass> &set1,
                                                        int k);
    
    // Goal: Get the i^th subset of length k of a given set
    // Input: (a) The std::set of which we need the subsets
    //        (b) The index k indicating the size of the subsets
    //        (c) The index i of the combinatic (i_th combination with k elements)
    // Output: A vector with all the subsets of certain size
    //
    // Algorithm by James McCaffrey Combinatic
    // https://msdn.microsoft.com/en-us/library/aa289166.aspx
    //
    static std::set<TargetClass> IthSubsetSizeK(const std::set<TargetClass> &set1,
                                                          int k,
                                                          int index);

    // Goal: Compute the factorial of a given number
    static int Factorial(int n);

    // Goal: Compute the factorial starting from s to n, i.e. s*s+1*s+2*...*n
    static int RangeFactorial(int s, int n);

};

#include "set-operations.cc"

#endif /* SetOperation_hpp */
