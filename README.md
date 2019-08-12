# mandelbrot_avx
A toy example of porting a mandelbrot renderer to AVX2. Please note that if you wanted to do this real fast for some reason, have the decency to rewrite it in CUDA or something using GPGPU.

Compile either with clang and `-mavx -O3` or with MSVC and `/arch:AVX2`.
