#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <chrono>

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

    using byte = unsigned char;
    byte* pixels = (byte*) malloc(PIXEL_COUNT * CHANNELS * sizeof(byte));

    auto start = std::chrono::high_resolution_clock::now();
    // c.f. http://www.bealto.com/mp-mandelbrot_simple-c.html
    int n = 0;
    for (int py = 0; py < IMG_H; py++) {
        const float v = (float(py) / float(IMG_H)) - .5f;

        for (int px = 0; px < IMG_W; px++) {
            const float u = (float(px) / float(IMG_W)) - .5f;

            const float cx = view_center_x + view_width * u;
            const float cy = view_center_y + view_height * v;

            float x = 0;
            float y = 0;

            int iter = 0;
            for (; iter <= MAX_ITER; iter++) {
                float x2 = x * x;
                float y2 = y * y;
                if (x2 + y2 > 4)
                    break;
                float twoxy = 2 * x * y;
                // Z = Z^2 + c
                x = x2 - y2 + cx;
                y = twoxy + cy;
            }

            const float t = float(iter) / float(MAX_ITER);
            pixels[n] = byte(t * 255.99);
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