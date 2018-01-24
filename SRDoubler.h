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

inline void RaiseEx(const char * message)
{
   throw std::exception(message);
}

/* Functions necessary for the KEISER_FILTER */

// zero-th order modified Bessel function of the first kind
inline  double I0(double z)
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
inline  double Kaiser(double x, double alpha)
{
   if (x < 0.)
      return 1.;
   else if (x > 1.)
      return 0.;
   else
      return I0(alpha*sqrt(1. - x*x)) / I0(alpha);
}

//A standard Keiser function goes from 1 to 0 when its argument goes from 0 to 1
//The mapped function does this when its argument goes from n0 to n1 
inline  double KaiserMappedOverIntegerRange(double x, double alpha, size_t n0, size_t n1)
{
   if (n0 != n1)
      return Kaiser((x - n0) / (n1 - n0), alpha);
   else
   {
      RaiseEx("Wrong KaiserMappedOverIntegerRange params");
      return 0.;
   }
}

inline double sinc(double x)
{
   if (x == 0.)
      return 1.;
   else
      return sin(PI*x) / (PI*x);
}

/* Keiser window filter
The class allocates a 16-byte aligned double array and fills
it with the values of a Keiser window function mapped over the range
from 0 to halfWidth+1, for arguments from 0.5 to halfWidth-0.5,
multiplied by the values of a sinc function for the same arguments.
*/

template <size_t table_width> class CFilter : public array<double, table_width>
{
public:
   CFilter(double alpha)
   {
      using array_type = array<double, table_width>;
      static_assert(table_width % 2 == 0, "Table_width should be an even number");
      size_t halfWidth = table_width / 2;

      //calculate the coefficients
      size_t i = 0;

      auto lambda = [&]
      {
         size_t dist = (i < halfWidth) ? (halfWidth - i - 1) : (i - halfWidth);
         i++;
         return KaiserMappedOverIntegerRange(dist + 0.5, alpha, 0, halfWidth + 1)*sinc(dist + 0.5);
      };

      generate(array_type::begin(), array_type::end(), lambda);

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

