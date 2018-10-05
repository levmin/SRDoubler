	This program utilises bandlimited interpolation (see https://ccrma.stanford.edu/~jos/resample/) to double the sampling rate of an incoming signal. Its primary application is high-fidelity computer audio playback. The implementation is written in modern C++ and has been tested on Windows with 
Microsoft Visual C++ 2017 and Clang 5. The Visual Studio solution file provides the following build configurations:

	File_Upsampler, Clang, Intel, MSVC, Clang_Extreme, MSVC_Extreme. 
	
File_Upsampler does not rely on constant expressions, all processing is done at run time. The Clang, Intel and MSVC configurations calculate filter coefficients at compile time and then generate a sample sine wave and upsample it at run time. The Clang_Extreme and MSVC_Extreme configurations attempt to both calculate filter coefficents and upsample a sine wave at compile time. 
	

