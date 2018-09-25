/*
   ConstExprDemo

   This program shows how to use constexpr C++ functionality for compile-time calculation of digital filter coefficients. 

   Copyright © 2018 Lev Minkovsky

   This software is licensed under the MIT License (MIT).

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
   THE SOFTWARE.

*/
#include "ConstExprDemo.h"
#include <chrono>
#include <iostream>
using std::cout;

constexpr size_t TABLE_WIDTH = 3200;         //width of a filter table
constexpr double ALPHA = 9;                  //parameter of a Kaiser function

namespace non_constexpr_funcs {

   inline double sinc(double x)
   {
      if (x == 0.)
         return 1.;
      else
         return std::sin(PI*x) / (PI*x);
   }

   inline double Kaiser(double x, double alpha)
   {
      if (x < 0.)
         return 1.;
      else if (x > 1.)
         return 0.;
      else
         return I0(alpha*std::sqrt(1. - x * x)) / I0(alpha);
   }

   inline double KaiserMappedOverIntegerRange(double x, double alpha, size_t n0, size_t n1)
   {
      if (n0 != n1)
         return Kaiser((x - n0) / (n1 - n0), alpha);
      else
         throw std::runtime_error("Wrong KaiserMappedOverIntegerRange params");
   }

}

template <size_t table_width> class CDemoFilter :public CFilter<table_width>
{
public:
   using array_type = typename CFilter<table_width>::array_type;
   CDemoFilter(double alpha) :CFilter<table_width>(alpha) {}

   void init(double alpha) //does the same as the constructor but without constexpr functions
   {
      size_t halfWidth = table_width / 2;

      //calculate the coefficients
      size_t i = 0;
      auto  lambda = [&]()
      {
         size_t dist = (i < halfWidth) ? (halfWidth - i - 1) : (i - halfWidth);
         i++;
         return non_constexpr_funcs::KaiserMappedOverIntegerRange(dist + 0.5, alpha, 0, halfWidth + 1)*non_constexpr_funcs::sinc(dist + 0.5);
      };
      std::generate(array_type::begin(), array_type::end(), lambda);
   }
};

constexpr unsigned DEMO_SOUND_FREQUENCY = 441; //
constexpr unsigned SAMPLES_PER_CYCLE = 100;
constexpr double   SINE_WAVE_STEP = 2. * PI / SAMPLES_PER_CYCLE;
constexpr unsigned CD_SAMPLING_RATE = 44100;
static_assert(DEMO_SOUND_FREQUENCY*SAMPLES_PER_CYCLE == CD_SAMPLING_RATE);
constexpr unsigned DEMO_SOUND_DURATION = 10;
static_assert(DEMO_SOUND_DURATION > 2);
constexpr unsigned DEMO_INPUT_SAMPLES = DEMO_SOUND_DURATION * CD_SAMPLING_RATE;
constexpr size_t FIRST_SAMPLE_TO_COMPARE = CD_SAMPLING_RATE * 2;
constexpr size_t SAMPLES_TO_COMPARE = CD_SAMPLING_RATE * 2 * (DEMO_SOUND_DURATION - 2);


int main(int argc, char ** argv)
{
   //create an appropriate Keiser window filter
#ifdef __ICL //Intel compiler does not handle constexpr sufficiently well
#define CONSTEXPR  
#else
#define CONSTEXPR constexpr
#endif
   CONSTEXPR static CFilter<TABLE_WIDTH> KEISER_FILTER{ ALPHA };
#undef CONSTEXPR 

   static_assert(sizeof(KEISER_FILTER));

   //create another, runtime constructed filter to compare the coefficients to
   
   CDemoFilter<TABLE_WIDTH> test_filter{ ALPHA };
   test_filter.init(ALPHA);
   bool match = true;
   for (size_t i = 0; i < TABLE_WIDTH; i++)
   {
      if (KEISER_FILTER[i] == 0)
      {
         cout << "Zero Keiser filter coefficient found\n";
         return -1;
      }

      if (abs((test_filter[i] - KEISER_FILTER[i]) / KEISER_FILTER[i]) > 10E-8)
      {//test filter and Keiser filter parameters differ by more then 0.000001%; report mismatch
         match = false;
         break;
      }
   }
   if (!match)
   {
      cout << "Compiletime and runtime filters DO NOT match\n";
      return -1;
   }

   cout << "Compiletime and runtime filters match\n";

   /*
      Create a sample signal and upsample it
   */

   using SRDoublerType = SRDoubler<double, 1, TABLE_WIDTH>;
   using SampleFrame = SRDoublerType::SampleFrame;
   using FrameSpan = SRDoublerType::FrameSpan;
   using FrameVector = SRDoublerType::FrameVector;
   
   cout << "Prepare sample input\n";

   std::vector<SampleFrame> input{ DEMO_INPUT_SAMPLES };
   double x = 0;
   //fill in the input with a sine wave
   for (SampleFrame& frame:input)
   {
      frame[0] = std::sin(x);
      x += SINE_WAVE_STEP;
   }


   FrameSpan sine_wave_span{ input };
   SRDoublerType doubler{ sine_wave_span ,KEISER_FILTER };

   cout << "About to start upsampling...\n";

   using namespace std::chrono;

   steady_clock clock;

   auto t0 = clock.now();

   //upsample the input
   FrameVector upsampled_signal = doubler.Run();

   auto t1 = clock.now();

   auto time_diff = t1 - t0;

   using milliseconds_type = duration<double, std::milli>;

   cout << "Upsampling took " << duration_cast<milliseconds_type>(time_diff).count() << " milliseconds\n";


   //explicitly generate a sine wave with a doubled sample rate
   static double doubled_sine_wave[DEMO_INPUT_SAMPLES * 2]{ 0 };

   x = 0;

   for (double& sample : doubled_sine_wave)
   {
      sample = std::sin(x);
      x += SINE_WAVE_STEP/2;
   }

   //Compare the middle DEMO_SOUND_DURATION-2 seconds of the upsampled and generated signals. 
   //They should closely match

   match = true;
   for (size_t i = FIRST_SAMPLE_TO_COMPARE; i < (FIRST_SAMPLE_TO_COMPARE + SAMPLES_TO_COMPARE); i++)
   {
      if (abs(upsampled_signal[i][0] - doubled_sine_wave[i]) > 10E-7) //-140db
      {
         match = false;
         break;
      }
   }

   cout << ((match)? "Upsampling accuracy confirmned\n" : "Upsampling isn't accurate\n" ) ;
}
