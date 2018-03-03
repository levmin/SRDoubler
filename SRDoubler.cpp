/*
   Sample Rate Doubler

   This program takes a file with a digital audio stream and produces a file with the same stream having a doubled sampling frequency

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
#include "SRDoubler.h"
#include <chrono>
#include <iostream>
#include <libsndfile\include\sndfile.h>

static_assert(true);
constexpr size_t TABLE_WIDTH = 3200;         //width of a filter table
constexpr double ALPHA = 9;                  //parameter of a Kaiser function

int main(int argc, char ** argv)
{
   if (argc != 3)
   {
      cout << "Usage: SrDoubler <input file> <output file>\n";
      return 0;
   }

   //open input file
   SF_INFO info_in{ 0 };
   SNDFILE * in = sf_open(argv[1], SFM_READ, &info_in);
   if (!in)
   {
      cout << "Failure to open an input file\n";
      return -1;
   }
   if (info_in.channels!=2)
   {
      sf_close(in);
      cout << "SRDoubler can process only stereo files\n";
      return -1;
   }

   using SRDoublerType = SRDoubler<double, 2 , TABLE_WIDTH>;
   using SampleFrame = SRDoublerType::SampleFrame;
   using FrameSpan = SRDoublerType::FrameSpan;
   using FrameVector = SRDoublerType::FrameVector;

   vector<SampleFrame> input{static_cast<size_t>(info_in.frames)};
   sf_count_t rc = sf_readf_double(in, &input[0][0], info_in.frames);
   if (rc != info_in.frames)
   {
      sf_close(in);
      cout << "Failure to read all the expected audio data\n";
      return -1;
   }

   cout << info_in.frames << " audio frames read\n";

   //create an appropriate Keiser window filter
   CONSTEXPR static CFilter<TABLE_WIDTH> KEISER_FILTER{ ALPHA};

   FrameSpan sine_wave_span { input };
   SRDoublerType doubler{ sine_wave_span ,KEISER_FILTER };

   cout << "About to start upsampling...\n";

   chrono::steady_clock clock; 

   auto t0 = clock.now();

   //upsample the input
   FrameVector upsampled_signal = doubler.Run();

   auto t1 = clock.now();

   auto time_diff = t1 - t0;

   using milliseconds_type=chrono::duration<double,std::milli>;
   
   cout << "Upsampling took " << chrono::duration_cast<milliseconds_type>(time_diff).count() << " milliseconds\n";

   //save the upsampled signal into an output file
   SF_INFO info_out { info_in };
   info_out.samplerate *= 2;
   SNDFILE * out = sf_open(argv[2], SFM_WRITE, &info_out);
   info_out.frames = info_in.frames*2;

   rc = sf_writef_double(out, &upsampled_signal[0][0], info_out.frames);
   sf_close(out);
   if (rc != info_out.frames)
   {
       cout << "Failure to save upsampled data\n";
      return -1;
   }
   else
   {
      cout << info_out.frames << " audio frames written\n";
   }
   return 0;
}
