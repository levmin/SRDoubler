/*
   constexprDemo

   This program shows how to use constexpr C++ functionality for compile-time calculation of digital filter coefficients

   Copyright © 2018 Lev Minkovsky

   // This software is licensed under the MIT License (MIT).

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
   THE SOFTWARE.

*/

#include <gsl\span>
#include <vector>
#include <array>
#include <cmath>
#include <algorithm>
#include <stdexcept>

constexpr double PI = 3.14159265358979323846264338327950288L;

/*
   Compiler analysis
*/
#ifndef TEST_FOR_constexpr_SUPPORT 
   #ifdef __FUNCDNAME__ //don't test with Microsoft compiler
      #define TEST_FOR_constexpr_SUPPORT 0
   #else
      #define TEST_FOR_constexpr_SUPPORT 1
   #endif
#endif

#if TEST_FOR_constexpr_SUPPORT 
   #if defined(__cpp_constexpr)
      #if __cpp_constexpr < 201603 //Compiler constexpr support is not sufficient
         #pragma message("\n")
         #pragma message("WARNING!!!!!!!!!!!!!!!!!!!!!!!!!\n")
         #pragma message("constexpr lambda expressions are not available. Compiling without the lambdas\n")
         #pragma message("\n")
         #define USE_constexpr_LAMBDA 0
      #else
         #define USE_constexpr_LAMBDA 1
      #endif
   #else
      #error "__cpp_constexpr is not defined!"
   #endif
#else
   #define USE_constexpr_LAMBDA 0
#endif


/*
   constexpr functions
*/

namespace constexpr_funcs {

   inline constexpr double abs(double x)
   {
      return (x >= 0) ? x : -x;
   }

#if !USE_constexpr_LAMBDA
   inline constexpr double powerOfTen(int num)
   {
      double rst = 1.0;
      if (num >= 0) {
         for (int i = 0; i < num; i++) {
            rst *= 10.0;
         }
      }
      else {
         for (int i = 0; i < (0 - num); i++) {
            rst *= 0.1;
         }
      }
      return rst;
   };
#endif

   inline constexpr double sqrt(double a)
   {
      /*
      find more detail of this method on wiki methods_of_computing_square_roots

      *** Babylonian method cannot get exact zero but approximately value of the square_root
      */
      double z = a;
      double rst = 0.0;
      int max = 8;	// to define maximum digit 
      double j = 1.0;

#if USE_constexpr_LAMBDA
      constexpr auto powerOfTen = [](int num) -> double
      {
         double rst = 1.0;
         if (num >= 0) {
            for (int i = 0; i < num; i++) {
               rst *= 10.0;
            }
         }
         else {
            for (int i = 0; i < (0 - num); i++) {
               rst *= 0.1;
            }
         }
         return rst;
      };

#endif
      
      for (int i = max; i > 0; i--) {
         // value must be bigger then 0
         if (z - ((2 * rst) + (j * powerOfTen(i)))*(j * powerOfTen(i)) >= 0)
         {
            while (z - ((2 * rst) + (j * powerOfTen(i)))*(j * powerOfTen(i)) >= 0)
            {
               j++;
               if (j >= 10) break;

            }
            j--; //correct the extra value by minus one to j
            z -= ((2 * rst) + (j * powerOfTen(i)))*(j * powerOfTen(i)); //find value of z

            rst += j * powerOfTen(i);	// find sum of a

            j = 1.0;

         }

      }

      for (int i = 0; i >= 0 - max; i--) {
         if (z - ((2 * rst) + (j * powerOfTen(i)))*(j * powerOfTen(i)) >= 0)
         {
            while (z - ((2 * rst) + (j * powerOfTen(i)))*(j * powerOfTen(i)) >= 0)
            {
               j++;
               if (j >= 10) break;
            }
            j--;
            z -= ((2 * rst) + (j * powerOfTen(i)))*(j * powerOfTen(i)); //find value of z

            rst += j * powerOfTen(i);	// find sum of a			
            j = 1.0;
         }
      }
      // find the number on each digit
      return rst;
   }

   inline constexpr double fract(double x)
   {
      return x - (int)x ;
   }

   inline constexpr double sin(double x)
   {
      // Normalize the x to be in [-pi, pi]
      x += PI;
      x *= 1 / (2 * PI);
      x = fract(fract(x) + 1);
      x *= PI * 2;
      x -= PI;

      // the algorithm works for [-pi/2, pi/2], so we change the values of x, to fit in the interval,
      // while having the same value of sin(x)
      if (x < -(PI/2))
         x = -PI - x;
      else if (x > PI/2)
         x = PI - x;
      // useful to pre-calculate
      double x2 = x * x;
      double x4 = x2 * x2;

      // Calculate the terms
      // As long as abs(x) < sqrt(6), which is 2.45, all terms will be positive.
      // Values outside this range should be reduced to [-pi/2, pi/2] anyway for accuracy.
      // Some care has to be given to the factorials.
      // They can be pre-calculated by the compiler,
      // but the value for the higher ones will exceed the storage capacity of int.
      // so force the compiler to use unsigned long longs (if available) or doubles.
      double t1 = x * (1.0 - x2 / (2 * 3));
      double x5 = x * x4;
      double t2 = x5 * (1.0 - x2 / (6 * 7)) / (1.0 * 2 * 3 * 4 * 5);
      double x9 = x5 * x4;
      double t3 = x9 * (1.0 - x2 / (10 * 11)) / (1.0 * 2 * 3 * 4 * 5 * 6 * 7 * 8 * 9);
      double x13 = x9 * x4;
      double t4 = x13 * (1.0 - x2 / (14 * 15)) / (1.0 * 2 * 3 * 4 * 5 * 6 * 7 * 8 * 9 * 10 * 11 * 12 * 13);
      // add some more if your accuracy requires them.
      // But remember that x is smaller than 2, and the factorial grows very fast
      // so I doubt that 2^17 / 17! will add anything.
      // Even t4 might already be too small to matter when compared with t1.

      // Sum backwards
      double result = t4;
      result += t3;
      result += t2;
      result += t1;

      return result;
   }

   template<class _FwdIt, class _Fn0>
   constexpr inline void generate(_FwdIt _First, _FwdIt _Last, _Fn0& _Func)
   {
      for (; _First != _Last; ++_First)
         *_First = _Func();
   }
}

/* Functions necessary for the KEISER_FILTER */

// zero-th order modified Bessel function of the first kind
inline constexpr double I0(double z)
{
   const double EPS = 10E-16;

   double zz4 = z*z / 4.;
   double k = 0.;       //summing by k from 0 to infinity
   double zz4_in_k_degree = 1.;
   double kfact = 1.;   //k factorial
   double I0_sum_member = 1.; //the sum member for k = 0
   double I0_sum = I0_sum_member;
   do
   {
      k++; //increment k
      kfact *= k; //calculate k factorial
      zz4_in_k_degree *= zz4;
      I0_sum_member = zz4_in_k_degree / (kfact*kfact);
      I0_sum += I0_sum_member;
   }  while (I0_sum_member >= EPS);
   return I0_sum;
}

//Keiser window function for a floating point argument
inline constexpr  double Kaiser(double x, double alpha)
{
   if (x < 0.)
      return 1.;
   else if (x > 1.)
      return 0.;
   else
      return I0(alpha*constexpr_funcs::sqrt(1. - x*x)) / I0(alpha);
}


//A standard Keiser function goes from 1 to 0 when its argument goes from 0 to 1
//The mapped function does this when its argument goes from n0 to n1 
inline constexpr  double KaiserMappedOverIntegerRange(double x, double alpha, size_t n0, size_t n1)
{
   if (n0 != n1)
      return Kaiser((x - n0) / (n1 - n0), alpha);
   else
      throw std::runtime_error("Wrong KaiserMappedOverIntegerRange params");
}

inline constexpr double sinc(double x)
{
   if (x == 0.)
      return 1.;
   else
      return constexpr_funcs::sin(PI*x) / (PI*x);
}

/* Keiser window filter
The class allocates a 16-byte aligned double array and fills
it with the values of a Keiser window function mapped over the range
from 0 to halfWidth+1, for arguments from 0.5 to halfWidth-0.5,
multiplied by the values of a sinc function for the same arguments.
*/

template <size_t table_width> class CFilter : public std::array<double, table_width> 
{
public:
   using array_type = std::array <double, table_width>;
   constexpr CFilter(double alpha) : array_type()
   {
      static_assert(table_width % 2 == 0, "Table_width should be an even number");
      size_t halfWidth = table_width / 2;

#if USE_constexpr_LAMBDA
      //calculate the coefficients
      size_t i = 0;
      auto  lambda = [&]()
      {
#ifdef __FUNCDNAME__   //Microsoft compiler detected, a walkaround for a bug with tetriary operators in constexpr lambdas
         size_t dist = 0;
         if (i < halfWidth)
            dist = halfWidth - i - 1;
         else
            dist = i - halfWidth;
#else
         size_t dist = (i < halfWidth) ? (halfWidth - i - 1) : (i - halfWidth);
#endif
         i++;
         return KaiserMappedOverIntegerRange(dist + 0.5, alpha, 0, halfWidth + 1)*sinc(dist + 0.5);
      };
      constexpr_funcs::generate(array_type::begin(), array_type::end(), lambda);
#else //don't use a lambda for the coefficients
      //calculate the coefficients
      for (size_t i = 0; i < table_width; i++)
      {
         size_t dist = (i < halfWidth) ? (halfWidth - i - 1) : (i - halfWidth);
         _Elems[i] = KaiserMappedOverIntegerRange(dist + 0.5, alpha, 0, halfWidth + 1)*sinc(dist + 0.5);
      };

#endif
   }
};

template<typename SampleFormat, uint8_t numChannels, size_t table_width> class SRDoubler
{
public:
   using Array = std::array<SampleFormat, numChannels>;
   struct SampleFrame : public Array
   {
      using typename Array::size_type;
      using Array::size;
      using Array::at;

      constexpr SampleFrame() : Array()
      {
      }

      constexpr SampleFrame operator* (const double factor) const
      {
         SampleFrame outFrame;

         for (size_type index = 0; index < size(); index++)
            outFrame[index] = at(index)*factor;

         return outFrame;
      }

      constexpr SampleFrame& operator+= (const SampleFrame& frame)
      {
         for (size_type index = 0; index < size(); index++)
            at(index) += frame[index];

         return *this;
      }
   };
   using FrameSpan = gsl::span<SampleFrame>;
   using FrameVector = std::vector<SampleFrame>;
   using KeiserFilterType = CFilter<table_width>;
   using size_type = typename FrameVector::size_type;
   using index_type = typename FrameSpan::index_type;

   constexpr SRDoubler(const FrameSpan& in_span, const KeiserFilterType& filter) : m_in_span{ in_span }, m_filter{ filter }
   {
   }

private:

   const FrameSpan& m_in_span;
   const KeiserFilterType& m_filter;

   const int halfWidth = table_width / 2;

   SampleFrame m_null_frame {};

   constexpr const SampleFrame& getInputFrame(ptrdiff_t index)  const
   {
      if (index >= 0 && index < m_in_span.size())
      {
         return m_in_span[index];
      }
      else
      {
         return m_null_frame;
      }
   }

   constexpr const SampleFrame getInterpolatedFrame(ptrdiff_t index) const
   {
      SampleFrame outFrame;

      ptrdiff_t i = index + 1 - halfWidth;

      for (size_t j = 0; j<m_filter.size(); j++)
      {
         outFrame += getInputFrame(i++)*m_filter[j];
      }

      return outFrame;
   }

public:

   FrameVector Run() const
   {
      FrameVector output(2 * m_in_span.size());

      size_type j = 0;

      for (index_type i = 0; i < m_in_span.size(); i++)
      {
         //alternate input and interpolated samples
         output[j++] = getInputFrame(i);
         output[j++] = getInterpolatedFrame(i);
      }

      return output;
   }

   constexpr void Run(FrameSpan& out_span)
   {
      size_type j = 0;

      for (index_type i = 0; i < m_in_span.size(); i++)
      {
         //alternate input and interpolated samples
         out_span[j++] = getInputFrame(i);
         out_span[j++] = getInterpolatedFrame(i);
      }

   }
};

