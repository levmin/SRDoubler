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
   using typename CFilter<table_width>::array_type;
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


constexpr bool demo()
{
   //create an appropriate Keiser window filter
   constexpr CFilter<TABLE_WIDTH> KEISER_FILTER{ ALPHA };

   using SRDoublerType = SRDoubler<double, 1, TABLE_WIDTH>;
   using SampleFrame = SRDoublerType::SampleFrame;
   using FrameSpan = SRDoublerType::FrameSpan;

   SampleFrame input[DEMO_INPUT_SAMPLES];

   double x = 0;
   //fill in the input with a sine wave
   for (SampleFrame& frame : input)
   {
      frame[0] = constexpr_funcs::sin(x);
      x += SINE_WAVE_STEP;
   }

   FrameSpan sine_wave_span{ input };
   SRDoublerType doubler{ sine_wave_span ,KEISER_FILTER };

   SampleFrame upsampled_signal[DEMO_INPUT_SAMPLES * 2];
   FrameSpan upsampled_signal_span{ upsampled_signal };

   doubler.Run(upsampled_signal_span);

   //explicitly generate a sine wave with a doubled sample rate
   double doubled_sine_wave[DEMO_INPUT_SAMPLES * 2]{ 0 };
   x = 0;
   for (double& sample : doubled_sine_wave)
   {
      sample = constexpr_funcs::sin(x);
      x += SINE_WAVE_STEP / 2;
   }

   //Compare the middle DEMO_SOUND_DURATION-2 seconds of the upsampled and generated signals. 
   //They should closely match

   bool match = true;
   for (size_t i = FIRST_SAMPLE_TO_COMPARE; i < (FIRST_SAMPLE_TO_COMPARE + SAMPLES_TO_COMPARE); i++)
   {
      if (constexpr_funcs::abs(upsampled_signal[i][0] - doubled_sine_wave[i]) > 10E-7) //-140db
      {
         match = false;
         break;
      }
   }

   return match;
}

constexpr bool match = demo();
static_assert(match, "Upsampling was not accurate!");

int main(int argc, char ** argv)
{
   //nothing to be done here, everything is done at the compile time!
}
