
#include <cstdio>
#include <vector>
#include <cstdlib>
#include <cmath>
#include <cstring>

#include "nccp.hpp"

namespace core {
    static zx::su32  ksize {5,5}; // kernel size
    static zx::su32  gsize {0,0}; // glyph  size
    static zx::su32  fsize {0,0}; // frame  size

    // static zx::graph skipres  {};
    // static zx::intvx imgint1  {}; // base frame integral
    // static zx::intvx imgint2  {}; // base frame integral ** 2
    // static zx::intvx imgsqrd  {}; // base frame squared
    // static zx::resvx results  {}; // base frame match results
    static zx::graph              bypass    {};
    static zx::graph              glyphs[4] {}; // expanded glyphs
    static std::vector<zx::match> matches   {}; // image detection matches
}

void
zx::tm::init(const su32 gsize, const su32 fsize) noexcept {

    if (gsize.w > core::ksize.w and gsize.h > core::ksize.h and gsize.w == gsize.h) {
        core::gsize = gsize;
    } else {
        core::gsize = core::ksize;
    }

    core::fsize = fsize;

    core::bypass.width  = core::fsize.w;
    core::bypass.height = core::fsize.h;
    core::bypass.size   = core::fsize.w * core::fsize.h;
    core::bypass.data   = new u08[core::bypass.size];

    load();
}

void
zx::tm::load(void) noexcept {

    const u32 wd = core::gsize.w, WD = core::ksize.w;
    const u32 ht = core::gsize.h, HT = core::ksize.h;

    for (u32 g = 0; g < 4; ++g)
    {
        core::glyphs[g].mean   = 0.0f;
        core::glyphs[g].stdv   = 0.0f;
        core::glyphs[g].vari   = 0.0f;

        core::glyphs[g].width  = core::gsize.w;
        core::glyphs[g].height = core::gsize.h;
        core::glyphs[g].size   = core::gsize.w * core::gsize.h;
        core::glyphs[g].data   = new u08[core::glyphs[g].size];

        for (u32 j = 0; j < ht; ++j) {
            u32 dj = j * HT / ht;
            for (u32 i = 0; i < wd; i++) {
                u32 di = i * WD / wd;
                core::glyphs[g].data[j * core::gsize.w + i] = tm::kernel[g][dj * core::ksize.w + di];
            }
        }

        const u32 size = core::glyphs[g].size;

        f32 sum = 0.0f;
        for (u32 i = 0; i < size; ++i)
            sum += f32(core::glyphs[g].data[i]);

        core::glyphs[g].mean = sum / f32(core::glyphs[g].size);

        f32 var = 0.0f;
        for (u32 i = 0; i < size; ++i) {
            const f32 v = f32(core::glyphs[g].data[i]) - core::glyphs[g].mean;
            var += v * v;
        }

        core::glyphs[g].stdv = (var <= 0.0f) ? 0.0f : std::sqrt(var);

        std::fprintf(stderr, "glyph[%d] mean: %f deviation: %f\n", g, core::glyphs[g].mean, core::glyphs[g].stdv);
    }

    std::fprintf(stderr, "init done...\n");
}


[[maybe_unused]]
static zx::f32 compute_ncc_at(const zx::graph& src, const zx::graph& tpl, zx::u32 sx, zx::u32 sy) {

    using zx::u32, zx::f32, zx::u08;

    const u32 tw = tpl.width;
    const u32 th = tpl.height;
    const u08 *src_data = src.data;
    const u08 *tpl_data = tpl.data;
    const u32 sw = src.width;
    const u32 N = tw * th;

    f32 sum_src = 0.0f;
    for (u32 y = 0; y < th; ++y) {
        for (u32 x = 0; x < tw; ++x) {
            sum_src += f32(src.data[(sy+y) * src.width + (sx+x)]);
        }
    }
    f32 src_mean = sum_src / f32(N);

    // numerator and denom part for source
    f32 tpl_mean = tpl.mean;
    f32 numer = 0.0f;
    f32 denom_src_sq = 0.0f;
    for (u32 y = 0; y < th; ++y) {
        const uint8_t* src_row = src_data + (size_t)(sy + y) * sw + sx;
        const uint8_t* tpl_row = tpl_data + (size_t)y * tw;
        for (u32 x = 0; x < tw; ++x) {

            f32 s = (f32)src_row[x] - src_mean;
            f32 t = (f32)tpl_row[x] - tpl_mean;
            numer += s * t;
            denom_src_sq += s * s;
        }
    }

    f32 denom = denom_src_sq * (tpl.stdv * tpl.stdv);
    if (denom <= 0.0) {
        return 0.0;
    }

    return numer / std::sqrt(denom);
}

void
zx::tm::stop(void) noexcept {
    for (u32 g = 0; g < 4; ++g) {
        delete [] core::glyphs[g].data;
        core::glyphs[g].data = nullptr;
    }
}

void
zx::tm::proc(const zx::graph& graph, const f32 min) noexcept
{
    const u32 out_w = graph.width  - core::gsize.w + 1;
    const u32 out_h = graph.height - core::gsize.h + 1;

    core::matches.clear();

    std::memset(core::bypass.data, 0, core::bypass.size);

    #pragma omp parallel for





    for (u32 g = 0; g < 4; ++g) {

        for (u32 y = 0; y < out_h; ++y) {
            for (u32 x = 0; x < out_w; ++x) {

                if (core::bypass.data[y * core::fsize.w + x] == 1) continue;

                f32 score = compute_ncc_at(graph, core::glyphs[g], x, y);

                if (score > min) {

                    for (u32 j = (y-2); j < (y + core::gsize.h + 2); ++j) {
                        for (u32 i = (x-2); i < (x + core::gsize.w + 2); ++i) {
                            core::bypass.data[j * core::fsize.w + i] = 1;
                        }
                    }

                    core::matches.emplace_back(zx::match{0,score,{x,y,core::gsize.w,core::gsize.h}});
                }
            }
        }
    }
}

const zx::graph&
zx::tm::glyph(const u32 index) noexcept {
    return core::glyphs[index];
}

const std::vector<zx::match>&
zx::tm::matches(void) noexcept {
    return core::matches;
}

#if 0

// ncc_template_match.cpp
// Simple NCC template matching (no integrals) for grayscale images.
// Snake_case naming, double precision for accumulation.
//
// Usage:
//  - Build Image objects (width, height, data pointer).
//  - Call ncc_match_template(src, tpl, dst_scores, match_x, match_y, match_score).
//  - dst_scores will be sized (src_w - tpl_w + 1) * (src_h - tpl_h + 1) with row-major order.

#include <vector>
#include <cstdint>
#include <cmath>
#include <limits>
#include <algorithm>
#include <cstring> // for memcpy

struct Image {
    int width;
    int height;
    // pointer to width*height bytes (uint8_t), row-major, no padding
    const uint8_t* data;
};

// compute mean and standard-deviation of a template (no sliding)
static void compute_mean_and_std_template(const Image& tpl, double &mean, double &stddev) {
    const int w = tpl.width;
    const int h = tpl.height;
    const uint8_t* d = tpl.data;
    const int n = w * h;
    if (n == 0) { mean = 0.0; stddev = 0.0; return; }

    double sum = 0.0;
    for (int i = 0; i < n; ++i) sum += (double)d[i];
    mean = sum / n;

    double var = 0.0;
    for (int i = 0; i < n; ++i) {
        double v = (double)d[i] - mean;
        var += v * v;
    }
    stddev = (var <= 0.0) ? 0.0 : std::sqrt(var);
}

// Evaluate NCC at one location (top-left of patch in src at (sx, sy))
static double compute_ncc_at(const Image& src, const Image& tpl, const double tpl_mean, const double tpl_std,
                             int sx, int sy) {
    const int tw = tpl.width;
    const int th = tpl.height;
    const uint8_t* src_data = src.data;
    const uint8_t* tpl_data = tpl.data;
    const int sw = src.width;

    const int N = tw * th;
    if (N == 0) return 0.0;

    // compute source patch mean
    double sum_src = 0.0;
    for (int y = 0; y < th; ++y) {
        const uint8_t* src_row = src_data + (size_t)(sy + y) * sw + sx;
        for (int x = 0; x < tw; ++x) sum_src += (double)src_row[x];
    }
    double src_mean = sum_src / N;

    // numerator and denom part for source
    double numer = 0.0;
    double denom_src_sq = 0.0;
    for (int y = 0; y < th; ++y) {
        const uint8_t* src_row = src_data + (size_t)(sy + y) * sw + sx;
        const uint8_t* tpl_row = tpl_data + (size_t)y * tw;
        for (int x = 0; x < tw; ++x) {
            double s = (double)src_row[x] - src_mean;
            double t = (double)tpl_row[x] - tpl_mean;
            numer += s * t;
            denom_src_sq += s * s;
        }
    }

    double denom = denom_src_sq * (tpl_std * tpl_std);
    if (denom <= 0.0) {
        // degenerate: zero variance in template or source patch -> undefined correlation
        // return 0.0 (no correlation) — could also return NAN
        return 0.0;
    }
    return numer / std::sqrt(denom);
}

// Main function: computes response map and best match
// - dst_scores will be resized to (out_w * out_h). If you don't want the map, pass an empty vector and it will be filled.
// - match_x, match_y are top-left coordinates of best match in src.
// - match_score is the NCC score in [-1,1].
static void ncc_match_template(const Image& src, const Image& tpl,
                               std::vector<double>& dst_scores,
                               int& match_x, int& match_y, double& match_score) {
    // size checks
    if (tpl.width <= 0 || tpl.height <= 0 || src.width < tpl.width || src.height < tpl.height) {
        dst_scores.clear();
        match_x = match_y = -1;
        match_score = 0.0;
        return;
    }

    const int out_w = src.width - tpl.width + 1;
    const int out_h = src.height - tpl.height + 1;
    dst_scores.assign((size_t)out_w * out_h, 0.0);

    // precompute template mean and stddev
    double tpl_mean = 0.0, tpl_std = 0.0;
    compute_mean_and_std_template(tpl, tpl_mean, tpl_std);

    // if template variance is zero, all template pixels equal -> special case:
    // NCC reduces to comparing patch mean to template value; but for simplicity return zero map.
    if (tpl_std == 0.0) {
        // Option: set high score where patch mean equals tpl pixel value
        // For now produce zero scores and return.
        match_x = match_y = -1;
        match_score = 0.0;
        return;
    }

    match_score = -1.0; // lowest possible NCC is -1
    match_x = match_y = 0;

    for (int y = 0; y < out_h; ++y) {
        for (int x = 0; x < out_w; ++x) {
            double score = compute_ncc_at(src, tpl, tpl_mean, tpl_std, x, y);
            dst_scores[y * out_w + x] = score;
            if (score > match_score) {
                match_score = score;
                match_x = x;
                match_y = y;
            }
        }
    }
}

#include <iostream>
#include <vector>
#include <cstdint>
#include <cstring>

// Include the ncc_match_template implementation from above here.
// (functions: Image, compute_mean_and_std_template, compute_ncc_at, ncc_match_template)

int main() {
    // Example: 10x10 source image filled with 0 (black)
    std::vector<uint8_t> src_data(10 * 10, 0);

    // Draw a 3x3 white square at position (4,4)
    for (int y = 4; y < 7; ++y) {
        for (int x = 4; x < 7; ++x) {
            src_data[y * 10 + x] = 255;
        }
    }

    Image src {10, 10, src_data.data()};

    // Template: 3x3 white square
    std::vector<uint8_t> tpl_data(3 * 3, 255);
    Image tpl {3, 3, tpl_data.data()};

    // Run NCC template matching
    std::vector<double> scores;
    int match_x, match_y;
    double match_score;

    ncc_match_template(src, tpl, scores, match_x, match_y, match_score);

    // Output result
    std::cout << "Best match at (x=" << match_x << ", y=" << match_y << ")\n";
    std::cout << "NCC score = " << match_score << std::endl;

    // Optional: print score map
    int out_w = src.width - tpl.width + 1;
    int out_h = src.height - tpl.height + 1;
    std::cout << "Score map:\n";
    for (int y = 0; y < out_h; ++y) {
        for (int x = 0; x < out_w; ++x) {
            std::cout << (scores[y * out_w + x] > 0.9 ? "X " : ". ");
        }
        std::cout << "\n";
    }

    return 0;
}

#endif

#if 0
namespace core {
    static zx::su32  ksize {5,5}; // kernel size
    static zx::su32  gsize {0,0}; // glyph  size
    static zx::su32  fsize {0,0}; // frame  size
    static zx::graph mask[4]  {}; // expanded glyphs
    static zx::graph skipres  {};
    static zx::intvx imgint1  {}; // base frame integral
    static zx::intvx imgint2  {}; // base frame integral ** 2
    static zx::intvx imgsqrd  {}; // base frame squared
    static zx::resvx results  {}; // base frame match results

    static std::vector<zx::match> matches {}; // image detection matches
}

void
zx::tm::init(const su32 size, const su32 fsize) noexcept {

    if (size.w < core::ksize.w or size.h < core::ksize.h) {
        core::gsize = core::ksize;
    } else {
        core::gsize = size;
    }

    core::fsize = fsize;

    core::imgint2.width  = core::imgint1.width  = fsize.w + 1;
    core::imgint2.height = core::imgint1.height = fsize.h + 1;
    core::imgint1.size   = core::imgint2.size   = core::imgint1.width * core::imgint1.height;

    core::results.width  = core::fsize.w - core::gsize.w + 1;
    core::results.height = core::fsize.h - core::gsize.h + 1;
    core::results.size   = core::results.width * core::results.height;
    core::skipres.width  = core::results.width;
    core::skipres.height = core::results.height;
    core::skipres.size   = core::results.size;

    core::imgsqrd.width  = fsize.w;
    core::imgsqrd.height = fsize.h;
    core::imgsqrd.size   = fsize.w * fsize.h;

    core::imgint1.data   = new i32[core::imgint1.size];
    core::imgint2.data   = new i32[core::imgint2.size];
    core::imgsqrd.data   = new i32[core::imgsqrd.size];
    core::results.data   = new f32[core::results.size];
    core::skipres.data   = new u08[core::skipres.size];

    //std::fprintf(stderr, "glyph: [%d x %d] image: [%d x %d]\n", core::gsize.w, core::gsize.h, core::fsize.w, core::fsize.h);

    std::memset(core::skipres.data, 0, core::skipres.size);
    std::memset(core::imgint1.data, 0, core::imgint1.size * sizeof(i32));
    std::memset(core::imgint2.data, 0, core::imgint2.size * sizeof(i32));
    std::memset(core::imgsqrd.data, 0, core::imgsqrd.size * sizeof(i32));
    std::memset(core::results.data, 0, core::results.size * sizeof(f32));

    mgen();
}

void
zx::tm::mgen(void) noexcept {

    for (u32 i = 0; i < 4; ++i) {

        core::mask[i].mean   = 0.0f; // mean image color
        core::mask[i].stdv   = 0.0f; // standard deviation
        core::mask[i].vari   = 0.0f; // standard variation

        core::mask[i].width  = core::gsize.w;
        core::mask[i].height = core::gsize.h;
        core::mask[i].size   = core::gsize.w * core::gsize.h;
        core::mask[i].data   = new u08[core::mask[i].size];

        f32 sum = 0.0f, sq_sum = 0.0f;

        for (u32 y = 0; y < core::gsize.h; ++y) {

            u32 srcY = (y * core::ksize.h) / core::gsize.h;
            if (srcY >= core::ksize.h) srcY = core::ksize.h - 1;

            for (u32 x = 0; x < core::gsize.w; ++x) {

                u32 srcX = (x * core::ksize.w) / core::gsize.w;
                if (srcX >= core::ksize.w) srcX = core::ksize.w - 1;

                const u08 value = tm::kernel[i][srcY * core::ksize.w + srcX];

                core::mask[i].data[y * core::mask[i].width + x] = value;

                sum    += f32(value);
                sq_sum += f32(value * value);
            }
        }

        core::mask[i].mean = sum / f32(core::mask[i].size);
        core::mask[i].vari = sq_sum / f32(core::mask[i].size) - core::mask[i].mean * core::mask[i].mean;
        core::mask[i].stdv = std::sqrt(std::max(1e-6f, core::mask[i].vari));
    }
}

void
zx::tm::stop(void) noexcept {

    delete [] core::imgint1.data;
    delete [] core::imgint2.data;
    delete [] core::results.data;
    delete [] core::skipres.data;

    core::imgint1.data = nullptr;
    core::imgint2.data = nullptr;
    core::results.data = nullptr;
    core::skipres.data = nullptr;

    for (u32 i = 0; i < 4; ++i) {
        delete [] core::mask[i].data;
        core::mask[i].data = nullptr;
    }
}

static inline int
rectsum(const zx::intvx& image, zx::u32 x, zx::u32 y, zx::u32 w, zx::u32 h) {
    return image.data[(y+h) * image.width + (x+w)] -
    image.data[( y ) * image.width + (x+w)] -
    image.data[(y+h) * image.width + ( x )] +
    image.data[( y ) * image.width + ( x )] ;
}

static inline void
search(const zx::graph& graph, const zx::graph& mask) noexcept {

    using zx::u32, zx::i32, zx::u08, zx::f32;

    const u32 WD = graph.width, HT = graph.height;
    const u32 wd = mask.width,  ht = mask.height;

    std::memset(core::imgint1.data, 0, core::imgint1.size * sizeof(i32));
    std::memset(core::imgint2.data, 0, core::imgint1.size * sizeof(i32));

    for (u32 j = 0; j < graph.height; ++j) { // calc squared values
        for (u32 i = 0; i < graph.width; ++i) {

            const u08 value = graph.data[j * graph.width + i];
            core::imgsqrd.data[ j * core::imgsqrd.width + i ] = value * value;
        }
    }

    for (u32 j = 1; j <= graph.height; ++j) { // calc sub frame values
        for (u32 i = 1; i <= graph.width; ++i) {

            core::imgint1.data[j * core::imgint1.width + i] =
            graph.data[(j-1) * graph.width + (i-1)] +
            core::imgint1.data[(j-1) * core::imgint1.width +   i  ] +
            core::imgint1.data[  j   * core::imgint1.width + (i-1)] -
            core::imgint1.data[(j-1) * core::imgint1.width + (i-1)] ;

            core::imgint2.data[j * core::imgint2.width + i] =
            core::imgsqrd.data[(j-1) * core::imgsqrd.width + (i-1)] +
            core::imgint2.data[(j-1) * core::imgint2.width +   i  ] +
            core::imgint2.data[  j   * core::imgint2.width + (i-1)] -
            core::imgint2.data[(j-1) * core::imgint2.width + (i-1)] ;
        }
    }

    for (u32 y = 0; y <= HT-ht; y++) { // main frame
        for (u32 x = 0; x <= WD-wd; x++) {

            const f32 p_sum    = f32(rectsum(core::imgint1, x, y, wd, ht));
            const f32 p_sq_sum = f32(rectsum(core::imgint2, x, y, wd, ht));
            const f32 p_mean   = p_sum / f32(wd * ht);
            const f32 p_var    = p_sq_sum / f32(wd * ht) - p_mean * p_mean;
            const f32 p_std    = std::sqrt(std::max(1e-6f, p_var));

            float numerator = 0.0f;
            for (u32 yy = 0; yy < ht; yy++) { // mask match
                for (u32 xx = 0; xx < wd; xx++) {

                    const f32 Iv = f32(graph.data[(y+yy) * graph.width +(x+xx)]) - p_mean;
                    const f32 Tv = f32(mask.data[yy * mask.width + xx]) - mask.mean;
                    numerator += Iv * Tv;

                }
            }

            const f32 denom = f32(wd * ht - 1) * mask.stdv * p_std;
            core::results.data[y * core::results.width + x] = numerator/denom;
        }
    }
}

void
zx::tm::proc(const zx::graph& graph, const f32 min) noexcept {

    core::matches.clear();

    std::memset(core::skipres.data, 0, core::skipres.size);

    for (u32 i = 0; i < 4; i++) { // masks

        search(graph, core::mask[i]);

        for (u32 k = 0; k < 5; ++k) { // searches

            f32 match = std::numeric_limits<f32>::min();
            u32 bestX = 0, bestY = 0;
            for (u32 y = 0; y < core::results.height; y++) { // base frame results
                for (u32 x = 0; x < core::results.width; x++) {
                    if (core::skipres.data[y * core::skipres.width + x] == 1) continue;
                    const f32 value = core::results.data[y * core::results.width +x];
                    if ( value > match) {
                        match = value;
                        bestX = x;
                        bestY = y;
                    }
                }
            }

            for (u32 j = bestY-2; j < (bestY+core::gsize.h); ++j) { // skips
                for (u32 i = bestX-2; i < (bestX+core::gsize.w); ++i) {
                    const u32 index = j * core::skipres.width + i;
                    if ( index < core::skipres.size ) core::skipres.data[index] = 1;
                }
            }

            if (match > min) { // test
                core::matches.emplace_back(zx::match{i, match, {bestX, bestY, core::gsize.w, core::gsize.h}});
            }
        }
    }
}

const zx::graph&
zx::tm::mask(const u32 index) noexcept {
    return core::mask[index];
}

const std::vector<zx::match>&
zx::tm::matches(void) noexcept {
    return core::matches;
}

#endif


