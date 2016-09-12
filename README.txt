Code demonstrating how to load custom ISA on Intel Haswell GPUs via OpenGL.
Also includes various ISA utilities and benchmarks.

This code works on Windows 8.1 and the following OpenGL implementation:

GL_RENDERER: Intel(R) HD Graphics 4400
GL_VERSION: 4.3.0 - Build 10.18.14.4080

It is highly unlikely that it will be useful anywhere else.

I have used this project to advocate for better API support for warp/thread level programming on GPUs.

For more information, see my related blog posts:

GPU Ray-Tracing The Wrong Way: http://www.joshbarczak.com/blog/?p=1197
SPMD Is Not Intel's Cup of Tea: http://www.joshbarczak.com/blog/?p=1120
You Compiled This Driver, Trust Me: http://www.joshbarczak.com/blog/?p=1028