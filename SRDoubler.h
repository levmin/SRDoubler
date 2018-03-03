/*
   Sample Rate Doubler

   This program takes a file with a digital audio stream and produces a file with the same stream having a doubled sampling frequency

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

using namespace std;

constexpr double PI = 3.14159265358979323846264338327950288L;
constexpr double EPS = 10E-16;

#define TEST_FOR_CONSTEXPR_SUPPORT 

#if !defined(TEST_FOR_CONSTEXPR_SUPPORT) || (defined(__cpp_constexpr) && __cpp_constexpr >= 201603)

#define USE_CONSTEXPR 1
#define CONSTEXPR constexpr

template<class _Ty,  size_t _Size>
   class fixed_size_array
{	
public:
   
   constexpr fixed_size_array(const _Ty& _Value) { for (_Ty& elem : _Elems) elem = _Value; }

   constexpr _Ty * begin()
   {	
      return _Elems;
   }

   constexpr _Ty * end()
   {	
      return _Elems +_Size;
   }

   constexpr size_t size() const
   {
      return _Size;
   }

   constexpr const _Ty&  operator[](size_t _Pos) const
   {	
      return (_Elems[_Pos]);
   }

private:

   _Ty _Elems[_Size];
};


inline constexpr double powerOfTen(int num) {
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
}

inline constexpr double squareRoot(double a)
{
   /*
   find more detail of this method on wiki methods_of_computing_square_roots

   *** Babylonian method cannot get exact zero but approximately value of the square_root
   */
   double z = a;
   double rst = 0.0;
   int max = 8;	// to define maximum digit 
   int i = 0;
   double j = 1.0;
   for (i = max; i > 0; i--) {
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

   for (i = 0; i >= 0 - max; i--) {
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

inline constexpr double sine_taylor(double x)
{
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
constexpr inline  void assign(_FwdIt _First, _FwdIt _Last, _Fn0& _Func)
{	
   for (; _First != _Last; ++_First)
      *_First = _Func();
}

#define SQRT squareRoot
#define SIN  sine_taylor
#define GENERATE assign
#else
//Compiler constexpr support is not sufficient
#if defined (__cpp_constexpr)
#pragma message("\n")
#pragma message("WARNING!!!!!!!!!!!!!!!!!!!!!!!!!\n")
#pragma message("constexpr lambda expressions are not available. Compiling without constexpr\n")
#pragma message("\n")
#else
#pragma message("\n")
#pragma message("WARNING!!!!!!!!!!!!!!!!!!!!!!!!!\n")
#pragma message("__cpp_constexpr is not defined. Compiling without constexpr\n")
#pragma message("\n")

#endif

#define USE_CONSTEXPR 0
#define CONSTEXPR 
#define SQRT sqrt
#define SIN  sin
#define fixed_size_array array
#define GENERATE generate
#endif

inline void RaiseEx(const char * message)
{
   throw std::exception(message);
}

/* Functions necessary for the KEISER_FILTER */

// zero-th order modified Bessel function of the first kind
inline CONSTEXPR double I0(double z)
{
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
inline CONSTEXPR  double Kaiser(double x, double alpha)
{
   if (x < 0.)
      return 1.;
   else if (x > 1.)
      return 0.;
   else
      return I0(alpha*SQRT(1. - x*x)) / I0(alpha);
}


//A standard Keiser function goes from 1 to 0 when its argument goes from 0 to 1
//The mapped function does this when its argument goes from n0 to n1 
inline CONSTEXPR  double KaiserMappedOverIntegerRange(double x, double alpha, size_t n0, size_t n1)
{
   if (n0 != n1)
      return Kaiser((x - n0) / (n1 - n0), alpha);
   else
   {
      RaiseEx("Wrong KaiserMappedOverIntegerRange params");
      return 0.;
   }
}

inline CONSTEXPR double sinc(double x)
{
   if (x == 0.)
      return 1.;
   else
      return SIN(PI*x) / (PI*x);
}

/* Keiser window filter
The class allocates a 16-byte aligned double array and fills
it with the values of a Keiser window function mapped over the range
from 0 to halfWidth+1, for arguments from 0.5 to halfWidth-0.5,
multiplied by the values of a sinc function for the same arguments.
*/

template <size_t table_width> class CFilter : public fixed_size_array<double, table_width>
{
public:
   CONSTEXPR CFilter(double alpha)  
#if USE_CONSTEXPR
       : fixed_size_array<double, table_width> (0)
#endif
   {
      using array_type = fixed_size_array<double, table_width>;
      static_assert(table_width % 2 == 0, "Table_width should be an even number");
      size_t halfWidth = table_width / 2;

      //calculate the coefficients
      size_t i = 0;
      auto  lambda = [&]() 
      {
         size_t dist = (i < halfWidth) ? (halfWidth - i - 1) : (i - halfWidth);
         i++;
         return KaiserMappedOverIntegerRange(dist + 0.5, alpha, 0, halfWidth + 1)*sinc(dist + 0.5);
      };
      GENERATE(array_type::begin(),array_type::end(),lambda);
   }
};

template<typename SampleFormat, uint8_t numChannels, size_t table_width> class SRDoubler
{
public:
   struct SampleFrame : public array<SampleFormat, numChannels>
   {
      using Array = array<SampleFormat, numChannels>;
      using typename Array::size_type;
      using Array::size;
      using Array::at;

      SampleFrame() : Array()
      {
      }

      SampleFrame operator* (const double factor) const
      {
         SampleFrame outFrame;

         for (size_type index = 0; index < size(); index++)
            outFrame[index] = at(index)*factor;

         return outFrame;
      }

      SampleFrame& operator+= (const SampleFrame& frame)
      {
         for (size_type index = 0; index < size(); index++)
            at(index) += frame[index];

         return *this;
      }
   };
   using FrameSpan = gsl::span<SampleFrame>;
   using FrameVector = vector<SampleFrame>;
   using KeiserFilterType = CFilter<table_width>;
   using size_type = typename FrameVector::size_type;
   using index_type = typename FrameSpan::index_type;

   SRDoubler(const FrameSpan& in_span, const KeiserFilterType& filter) : m_in_span{ in_span }, m_filter{ filter }
   {
   }


private:

   const FrameSpan& m_in_span;
   const KeiserFilterType& m_filter;

   const int halfWidth = table_width / 2;

   const SampleFrame& getInputFrame(ptrdiff_t index) const
   {
      if (index >= 0 && index < m_in_span.size())
      {
         return m_in_span[index];
      }
      else
      {
         static const SampleFrame null_frame;
         return null_frame;
      }
   }

   SampleFrame getInterpolatedFrame(ptrdiff_t index) const
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

   FrameVector Run()
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
};

