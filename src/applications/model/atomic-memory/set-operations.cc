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
//#include "set-operations.h"
#ifndef SETOPERATION_CPP
#define SETOPERATION_CPP

template < class TargetClass >
std::set<TargetClass> SetOperation<TargetClass>::unite(const std::set<TargetClass> &set1,
                                          const std::set<TargetClass> &set2){
	std::set<TargetClass> outSet;
	typename std::set<TargetClass>::iterator it;

	for(it=set1.begin(); it!=set1.end(); it++){
		//SMNodeAgent::DEBUGING(1, "Adding element %d in union\n", *it);
		outSet.insert(*it);
	}

	for(it=set2.begin(); it!=set2.end(); it++){
		//SMNodeAgent::DEBUGING(1, "Adding element %d in union\n", *it);
		outSet.insert(*it);
	}

	return outSet;
}

// Goal: Return intersection between two std::sets
// Output: Set intersection
//
template < class TargetClass >
std::set<TargetClass> SetOperation<TargetClass>::intersect(const std::set<TargetClass> &set1,
                                              const std::set<TargetClass> &set2){
	std::set<TargetClass> outSet;
	typename std::set<TargetClass>::iterator it;

	for(it=set1.begin(); it!=set1.end(); it++){
		//DEBUGING(1, "Checking element: %d\n", *it);

		if(set2.find(*it) != set2.end()){
			//DEBUGING(1, "Element %d exists in std::set2!\n", *it);

			outSet.insert(*it);
		}
	}

	return outSet;
}

// Goal: Find the complement of a std::set
// Input: The universe of elements and the
// std::set of which we want to ge the complement
// Output: the complement
template < class TargetClass >
std::set<TargetClass> SetOperation<TargetClass>::setComplement(const std::set<TargetClass> &U,
                                                  const std::set<TargetClass> &S){
	typename std::set<TargetClass>::iterator it;
	std::set<TargetClass> tmp=U;

	//remove all the elements of S from U
	for (it=S.begin(); it!=S.end(); it++) {
		//SMNodeAgent::DEBUGING(2, "Removing Elm=%d\n", *it);

		tmp.erase(*it);
	}

	return tmp;
}

// Goal: Check if the std::set given as first parameter is
// a substd::set of the second std::set.
// Output: 1 if first std::set is a substd::set of a second, 0 otherwise
//
template < class TargetClass >
int SetOperation<TargetClass>::subset(const std::set<TargetClass> &set1,
                         const std::set<TargetClass> &set2){
	typename std::set<TargetClass>::iterator it;

	// check if all the elements of std::set1 belong in std::set2
	for(it=set1.begin(); it!=set1.end(); it++){
		//DEBUGING(1, "Checking element: %d\n", *it);

		if(set2.find(*it) == set2.end()){
			//DEBUGING(1, "Element %d not found!\n", *it);
			return 0;
		}
	}

	return 1;
}

//Goal: print the elements in a std::set
//Input: (a) The std::set we want to print
//       (b) name of the std::set
//       (c) prefix of each element of the std::set
template < class TargetClass >
std::string SetOperation<TargetClass>::printSet(const std::set<TargetClass> &S,
                                    const std::string &name,
                                    const std::string &prefix){

	typename std::set<TargetClass>::iterator it;
	std::stringstream ss;
	std::string outstr;

	outstr= name+"={";
	for(it=S.begin(); it!=S.end(); it++){
		ss << prefix << *it << " ";
	}
	outstr=outstr+ss.str()+"}\n";

	return outstr;

}

// Goal: Get all the subsets of length k of a given set
// Input: (a) The std::set of which we need the subsets
//        (b) The index k indicating the size of the subsets
// Output: A vector with all the subsets of certain size
//
template < class TargetClass >
std::set< std::set<TargetClass> > SetOperation<TargetClass>::subsetsSizeK(const std::set<TargetClass> &set1,
                                                    			int k)
{
    int combinations;
    int f1,f2,f3;
    int i;
    typename std::set< std::set<TargetClass> > powerset;

    //compute the total number of subsets of size k
    f1 = Factorial(set1.size());
    f2 = Factorial(k);
    f3 = Factorial(set1.size()-k);
    combinations = f1 / (f2*f3);
    
    for ( i=0; i<combinations; i++ )
    {
    	powerset.insert( IthSubsetSizeK(set1, k, i) );
    }
    
    return powerset;
}

// Goal: Get all the subsets of length k of a given set
// Input: (a) The std::set of which we need the subsets
//        (b) The index k indicating the size of the subsets
//        (c) The index i of the combinatic (i_th combination with k elements)
// Output: A vector with all the subsets of certain size
//
// Algorithm by James McCaffrey Combinatic
// https://msdn.microsoft.com/en-us/library/aa289166.aspx
// index = Choose(c1, k) + Choose(c2, k-1) + ... +Choose(c_k, 1)
// set = { set1.element(c1), set1.element(c2), ..., set1.element(c_k) }
//
template < class TargetClass >
std::set<TargetClass> SetOperation<TargetClass>::IthSubsetSizeK(const std::set<TargetClass> &set1,
															int k,
															int index)
{
	typename std::set<TargetClass> result;
	typename std::set<TargetClass>::iterator it;
	int combinations;
	int combinadics[k];
	int i=k, element;
	int combinadic = set1.size();
	int f1,f2,f3;

	while ( index > 0 )
	{
		// compute C(c, i) = c!/(i!*(c-i)!)
		f1 = Factorial(combinadic);
		f2 = Factorial(i);
		f3 = Factorial(combinadic-i);
		combinations = f1 / (f2*f3);

		// if combinations is the largest value less than index - insert the combinatic
		if ( index > combinations )
		{
			// 1 <= i <= k
			combinadics[i-1] = combinadic;
			i--;
			index -= combinations;
		}

		combinadic --;
	}

	i = 0;
	element = 1;

	//construct the resulting set by inserting the picked combinatics
	for (it=set1.begin(); it != set1.end() && i<k; it++)
	{
		if (element == combinadics[i])
		{
			result.insert(*it);
			i++;
		}

		element++;
	}

	return result;
}

// Goal: Compute the factorial of a given number
template < class TargetClass >
int SetOperation<TargetClass>::Factorial(int n)
{
  return (n == 1 || n == 0) ? 1 : Factorial(n - 1) * n;
}

// Goal: Compute the factorial starting from s to n, i.e. s*s+1*s+2*...*n
template < class TargetClass >
int SetOperation<TargetClass>::RangeFactorial(int s, int n)
{
  return (n == 0) ? 1 : ((s == n) ? s : RangeFactorial(s, n - 1) * n);
}

#endif //#ifndef SETOPERATION_CPP
