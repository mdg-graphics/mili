/*
 Released under the MIT License - https://opensource.org/licenses/MIT

 Permission is hereby granted, free of charge, to any person obtaining
 a copy of this software and associated documentation files (the "Software"),
 to deal in the Software without restriction, including without limitation
 the rights to use, copy, modify, merge, publish, distribute, sublicense,
 and/or sell copies of the Software, and to permit persons to whom the
 Software is furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included
 in all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
/*  Compile instructions
 To compile just this code and run tests
 icc -g -o alphanum_comp.o -c alphanum_comp.c
 icc -g alphanum_comp.o -o alphanum_comp

 */
/* $Header: /code/doj/alphanum.hpp,v 1.3 2008/01/28 23:06:47 doj Exp $ */
#include <stdio.h>
#include <stdlib.h>

int alphanum_isdigit(int c) {
	return isdigit(c);
}

/**
 compare l and r with strcmp() semantics, but using
 the "Alphanum Algorithm". This function is designed to read
 through the l and r strings only one time, for
 maximum performance. It does not allocate memory for
 substrings. It can either use the C-library functions isdigit()
 and atoi() to honour your locale settings, when recognizing
 digit characters when you "#define ALPHANUM_LOCALE=1" or use
 it's own digit character handling which only works with ASCII
 digit characters, but provides better performance.

 @param l NULL-terminated C-style string
 @param r NULL-terminated C-style string
 @return negative if l<r, 0 if l equals r, positive if l>r
 */
extern int alphanum_cmp(const void *in_l, const void *in_r) {
	const char *l = *(const char**) in_l;
	const char *r = *(const char**) in_r;

	//const char* l = *ll;
	//const char* r = *rr;

	enum mode_t {
		STRING, NUMBER
	} mode = STRING;

	//	  Bool_type test1 = (l == NULL);
	//	  Bool_type test2 = (r == NULL);

	//while(!test1 && !test2)
	while (*l && *r) {
		if (mode == STRING) {
			char l_char, r_char;
			while ((l_char = *l) && (r_char = *r)) {
				// check if this are digit characters
				int l_digit = alphanum_isdigit(l_char), r_digit =
						alphanum_isdigit(r_char);
				// if both characters are digits, we continue in NUMBER mode
				if (l_digit && r_digit) {
					mode = NUMBER;
					break;
				}
				// if only the left character is a digit, we have a result
				if (l_digit)
					return -1;
				// if only the right character is a digit, we have a result
				if (r_digit)
					return +1;
				// compute the difference of both characters
				const int diff = l_char - r_char;
				// if they differ we have a result
				if (diff != 0)
					return diff;
				// otherwise process the next characters
				++l;
				++r;
			}
		} else // mode==NUMBER
		{
			// get the left number
			char *end;
			unsigned long l_int = strtoul(l, &end, 0);
			l = end;

			// get the right number
			unsigned long r_int = strtoul(r, &end, 0);
			r = end;
			// if the difference is not equal to zero, we have a comparison result
			const long diff = l_int - r_int;
			if (diff != 0)
				return diff;

			// otherwise we process the next substring in STRING mode
			mode = STRING;
		}
		//	  test1 = (l == NULL);
		//	  test2 = (r == NULL);
	}

	//	  if(test2) return -1;
	//	  if(test1) return +1;
	if (*r)
		return -1;
	if (*l)
		return +1;
	return 0;
}

void compare(char* left, char* right) {
	int result = 0;

	result = alphanum_cmp((const void*) (&left), (const void*) (&right));

	if (result == 0) {
		fprintf(stderr, "%s is the same as %s.\n", left, right);
	} else if (result < 0) {
		fprintf(stderr, "%s is less than %s.\n", left, right);
	} else {
		fprintf(stderr, "%s is less than %s.\n", right, left);
	}
}

//int main()
//{
//  int i;
//  char* array[] = {"material122","material1222","blue10","blue7","12a3","12a3"};
//  int size = sizeof(array)/sizeof(char*);
//  for(i=0; i<6; i++)
//  {
//      fprintf(stderr, "%s\n", array[i]);
//  }
//
//  fprintf (stderr, "********************\n");
//  qsort(array, size, sizeof(char *), (void*)alphanum_cmp);
//
//  for(i=0; i<6; i++)
//  {
//      fprintf(stderr, "%s\n", array[i]);
//  }
//  compare("","") ;
//  compare("","a") ;
//  compare("a","") ;
//  compare("a","a") ;
//  compare("","9") ;
//  compare("9","") ;
//  compare("blue10","blue7") ;
//  compare("1","2") ;
//  compare("3","2") ;
//  compare("material1222","material122") ;
//  compare("a1","a2") ;
//  compare("a2","a1") ;
//  compare("a1a2","a1a3") ;
//  compare("a1a2","a1a0") ;
//  compare("134","122") ;
//  compare("12a3","12a3") ;
//  compare("12a1","12a0") ;
//  compare("12a1","12a2") ;
//  compare("a","aa") ;
//  compare("aaa","aa") ;
//  compare("Alpha 2","Alpha 2") ;
//  compare("Alpha 2","Alpha 2A") ;
//  compare("Alpha 2 B","Alpha 2") ;
//}
