#include <cstdio>
#include <chrono>

#include "lens_blur.h"
#include "lens_blur_auto_schedule.h"

#include "halide_benchmark.h"
#include "HalideBuffer.h"
#include "halide_image_io.h"

using namespace Halide::Runtime;
using namespace Halide::Tools;

int main(int argc, char **argv) {
    if (argc < 7) {
        printf("Usage: ./process input.png slices focus_depth blur_radius_scale aperture_samples timing_iterations output.png\n"
               "e.g.: ./process input.png 32 13 0.5 32 3 output.png\n");
        return 0;
    }

    Buffer<uint8_t> left_im = load_image(argv[1]);
    Buffer<uint8_t> right_im = load_image(argv[1]);
    uint32_t slices = atoi(argv[2]);
    uint32_t focus_depth = atoi(argv[3]);
    float blur_radius_scale = atof(argv[4]);
    uint32_t aperture_samples = atoi(argv[5]);
    Buffer<float> output(left_im.width(), left_im.height(), 3);
    int timing_iterations = atoi(argv[6]);

    lens_blur(left_im, right_im, slices, focus_depth, blur_radius_scale,
              aperture_samples, output);

    // Timing code

    // Manually-tuned version
    double min_t_manual = benchmark(timing_iterations, 10, [&]() {
        lens_blur(left_im, right_im, slices, focus_depth, blur_radius_scale,
                  aperture_samples, output);
    });
    printf("Manually-tuned time: %gms\n", min_t_manual * 1e3);

    // Auto-scheduled version
    double min_t_auto = benchmark(timing_iterations, 10, [&]() {
        lens_blur_auto_schedule(left_im, right_im, slices, focus_depth,
                                blur_radius_scale, aperture_samples, output);
    });
    printf("Auto-scheduled time: %gms\n", min_t_auto * 1e3);

    convert_and_save_image(output, argv[7]);

    const halide_filter_metadata_t *md = lens_blur_metadata();
    // Only compare the performance if target has non-gpu features.
    if (!strstr(md->target, "cuda") &&
        !strstr(md->target, "opencl") &&
        !strstr(md->target, "metal") &&
        (min_t_auto > min_t_manual * 4.5)) {
        printf("Auto-scheduler is much much slower than it should be.\n");
        return -1;
    }

    return 0;
}