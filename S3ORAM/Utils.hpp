/*
 * Utils.hpp
 *
 *  Created on: Mar 15, 2017
 *      Author: ceyhunozkaptan, thanghoang
 */
 
#ifndef UTILS_HPP
#define UTILS_HPP

#include "config.h"

class Utils
{
public:
    Utils();
    ~Utils();

	#if defined(NTL_LIB)
		static sp_inverse PINV;
	#endif

    static int write_list_to_file(string filename, string path, vector<unsigned long int> lst);
	static int write_list_to_file(string filename, string path, unsigned long int* lst, int lst_size);
	static void writeArrayToFile(string filename, string path, double* arr, TYPE_INDEX size);
    
	static inline unsigned long long mulmod(unsigned long long a, unsigned long long b) 
	{
		#if defined(NTL_LIB)
			b = b << Utils::PINV.shamt;
			unsigned long P_tmp = P << Utils::PINV.shamt;
			
			ll_type U;
			ll_imul(U, a, b);
			
			unsigned long H = ll_rshift_get_lo<NTL_SP_NBITS-2>(U);
			unsigned long Q = MulHiUL(H, Utils::PINV.inv);
			Q = Q >> NTL_POST_SHIFT;
			unsigned long L = ll_get_lo(U);
			long r = L - Q*P_tmp;  // r in [0..2*n)
			return (sp_CorrectExcess(r, P_tmp) >> Utils::PINV.shamt);
		#else
			unsigned long long res = 0;
			unsigned long long temp_b;
			if (a > b)
			{
				temp_b = a;
				a = b;
				b = temp_b;
			}
			/* Only needed if b may be >= m */
			if (b >= P) {
				if (P > UINT64_MAX / 2u)
					b -= P;
				else
					b %= P;
			}
			while (a != 0) {
				if (a & 1) {
					/* Add b to res, modulo m, without overflow */
					if (b >= P - res) /* Equiv to if (res + b >= m), without overflow */
						res -= P;
					res += b;
				}
				a >>= 1;

				/* Double b, modulo m */
				temp_b = b;
				if (b >= P - b)       /* Equiv to if (2 * b >= m), without overflow */
					temp_b -= P;
				b += temp_b;
			}
			return res;
		#endif
	}

	static inline unsigned long long _LongRand()
	{
		unsigned char MyBytes[4];
		unsigned long long MyNumber = 0;
		unsigned char * ptr = (unsigned char *) &MyNumber;

		MyBytes[0] = rand() % 256; //0-255
		MyBytes[1] = rand() % 256; //256 - 65535
		MyBytes[2] = rand() % 256; //65535 -
		MyBytes[3] = rand() % 256; //16777216

		memcpy (ptr+0, &MyBytes[0], 1);
		memcpy (ptr+1, &MyBytes[1], 1);
		memcpy (ptr+2, &MyBytes[2], 1);
		memcpy (ptr+3, &MyBytes[3], 1);

		return(MyNumber);
	}

	static inline unsigned long long RandBound(unsigned long long bound) 
	{
	#if defined(NTL_LIB)
		return RandomBnd(bound);
	#else
		return _LongRand() % bound;
	#endif
	}
};

#endif // UTILS_HPP
