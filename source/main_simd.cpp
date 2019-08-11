#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <chrono>

#if defined(__clang__)
    #include <x86intrin.h>
#elif defined(_MSC_VER)
    #include <intrin.h>
#endif

int main() {  
    constexpr int IMG_W = 3200;
    constexpr int IMG_H = 3200;
    constexpr int PIXEL_COUNT = IMG_H * IMG_W;
    constexpr int CHANNELS = 1;

    static_assert(IMG_W % 8 == 0, "Width must be divisible by 8.");
    static_assert(IMG_H % 8 == 0, "Height must be divisible by 8.");

    constexpr int MAX_ITER = 256;
    constexpr float view_width = .2f;
    constexpr float view_height = .2f;
    constexpr float view_center_x = -.1f;
    constexpr float view_center_y = .8f;
    constexpr float view_botleft_x = view_center_x - view_width/2;
    constexpr float view_botleft_y = view_center_y - view_height/2;

    using byte = unsigned char;
    byte* pixels = (byte*) malloc(PIXEL_COUNT * CHANNELS * sizeof(byte));

    auto start = std::chrono::high_resolution_clock::now();
    // c.f. http://www.bealto.com/mp-mandelbrot_simple-c.html

    const __m256 pixel_delta = _mm256_set_ps(7, 6, 5, 4, 3, 2, 1, 0);
    constexpr float STRETCH_W = view_width / float(IMG_W);
    constexpr float STRETCH_H = view_height / float(IMG_H);

    int n = 0;
    for (int py = 0; py < IMG_H; py++) {
        const __m256 pys = _mm256_set1_ps(float(py));
        const __m256 v = _mm256_mul_ps(pys, _mm256_set1_ps(STRETCH_H));
        const __m256 cy = _mm256_add_ps(_mm256_set1_ps(view_botleft_y), v);

        for (int px = 0; px < IMG_W; px += 8) {
            // with the typical image sizes , it should be safe to do this add in FP
            const __m256 pxs = _mm256_add_ps(pixel_delta, _mm256_set1_ps(float(px)));
            const __m256 u = _mm256_mul_ps(pxs, _mm256_set1_ps(STRETCH_W));
            const __m256 cx = _mm256_add_ps(_mm256_set1_ps(view_botleft_x), u);

            __m256 x = _mm256_setzero_ps();
            __m256 y = _mm256_setzero_ps();

            __m256 iters = _mm256_setzero_ps();
            const __m256 ones = _mm256_set1_ps(1);
            for (int iter = 0; iter <= MAX_ITER; iter++) {
                const __m256 x2 = _mm256_mul_ps(x, x);
                const __m256 y2 = _mm256_mul_ps(y, y);
                const __m256 mask = _mm256_cmp_ps(
                    _mm256_add_ps(x2, y2),
                    _mm256_set1_ps(4),
                    _CMP_LE_OS
                );
                // if all > 4, break
                if (_mm256_testz_ps(mask, mask))
                    break;
                // otherwise, increase the iterations where needed
                iters = _mm256_blendv_ps(
                    iters,
                    _mm256_add_ps(iters, ones),
                    mask
                );

                // Z = Z^2 + c
                const __m256 xy = _mm256_mul_ps(x, y);
                x = _mm256_add_ps(cx, _mm256_sub_ps(x2, y2));
                y = _mm256_add_ps(cy, _mm256_add_ps(xy, xy));
            }

            const __m256 t = _mm256_mul_ps(iters, _mm256_set1_ps(255.99f/float(MAX_ITER)));
            const __m256i v = _mm256_cvttps_epi32(t);

            // Move the bytes around such that the lower 8 bytes are the pixel values.
            // This requires AVX2.
            const __m128i mask = _mm_set_epi64x(0xFFFFFFFFFFFFFFFF, 0xFFFFFFFF0C080400);
            const __m256i shuffle_mask = _mm256_set_m128i(mask, mask);
            const __m256i shuffled = _mm256_shuffle_epi8(v, shuffle_mask);
            const __m256i colors = _mm256_permutevar8x32_epi32(shuffled, 
                _mm256_set_epi32(0, 0, 0, 0, 0, 0, 4, 0)
            );
            
            ((long long*)pixels)[n] = _mm256_extract_epi64(colors, 0);
            n += CHANNELS;
        }
    }

    auto stop = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
    printf("Time taken: %f (microsec)\n", duration.count());
    printf("Pixels: %d\n", PIXEL_COUNT);
    printf("Pixels per microsec: %f\n", double(PIXEL_COUNT) / duration.count());

    stbi_write_png("out.png", IMG_W, IMG_H, CHANNELS, pixels, 0);
    free(pixels);
}