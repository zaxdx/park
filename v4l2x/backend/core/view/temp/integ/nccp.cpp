
#include <vector>
#include <cstring>
#include <cmath>

#include "nccp.hpp"

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

#if 0

// computeIntegral(const zx::graph& graph) noexcept {
//     const zx::u32 wd = graph.width;
//     const zx::u32 ht = graph.height;
//     std::vector<std::vector<int>> integral(ht+1, std::vector<int>(wd+1, 0));
//
//     //std::memset(core::imgint1.data, 0, core::imgint1.size * sizeof(i32));
//
//     for (zx::u32 y = 1; y <= ht; y++) {
//         for (zx::u32 x = 1; x <= wd; x++) {
//             integral[y][x] = graph.data[(y-1) * graph.width + (x-1)] +
//             integral[y-1][x] +
//             integral[y][x-1] -
//             integral[y-1][x-1];
//         }
//     }
//     return integral;
// }

//auto integral = computeIntegral(graph);
// std::vector<std::vector<int>> imgSq(graph.height, std::vector<int>(graph.width));
// for (u32 y = 0; y < HT; y++)
//     for (u32 x = 0; x < WD; x++)
//         imgSq[y][x] = graph.data[y * graph.width +x] * graph.data[y * graph.width + x];
// auto integralSq = computeIntegral(imgSq);
// aqui
// for (u32 j = 1; j <= graph.height; ++j) {
//     for (u32 i = 1; i <= graph.width; ++i) {
//
//         core::imgint2.data[j * core::imgint2.width + i] =
//         core::imgsqrd.data[(j-1) * core::imgsqrd.width + (i-1)] +
//         core::imgint2.data[(j-1) * core::imgint2.width +   i  ] +
//         core::imgint2.data[  j   * core::imgint2.width + (i-1)] -
//         core::imgint2.data[(j-1) * core::imgint2.width + (i-1)] ;
//     }
// }
// Output NCC map
//std::vector<std::vector<float>> result(HT-ht+1, std::vector<float>(WD-wd+1));
    // auto compare = [](const zx::match& a, const zx::match& b) noexcept {
    //     return f32(a.score > b.score);
    // };
    // std::priority_queue<zx::match, std::vector<zx::match>, decltype(compare)> best {};
    // for (u32 j = y; j < core::gsize.h; ++j) {
    //     for (u32 i =  x; i < core::gsize.w; ++i) {
    //         core::skip.data[j * graph.width + i] = 1;
    //     }
    // }
    // if (best.size() < 5) {
    //     best.push({ value, {bestX,bestY,core::gsize.w, core::gsize.h} });//core::size.w,core::size.h}});
    // } else if (value > best.top().score) {
    //     best.pop();
    //     best.push({ value, {bestX,bestY,core::gsize.w, core::gsize.h} });
    // }
    // while(!best.empty()) {
    //     core::matches.push_back(best.top());
    //     //std::fprintf(stderr,"top 20: %f [%dx%d]\n",pos.top().score, pos.top().area.x, pos.top().area.y);
    //     best.pop();
    // }


    // Precompute template stats
    // float t_sum = 0, t_sq_sum = 0;
    // for (u32 yy = 0; yy < ht; yy++) {
    //     for (u32 xx = 0; xx < wd; xx++) {
    //         float val = mask.data[yy * mask.width + xx];
    //         t_sum    += val;
    //         t_sq_sum += val * val;
    //     }
    // }
    //
    // float t_mean = t_sum / f32(wd * ht);
    // float t_var = t_sq_sum / f32(wd * ht) - t_mean * t_mean;
    // float t_std = std::sqrt(std::max(1e-6f, t_var));
    //const f32 t_mean = mask.mean;
    //const f32 t_var  = mask.vari;
    //const f32 t_std  = mask.stdv;

    //std::fprintf(stderr, "mask mean: %f variance: %f deviation: %f\n", t_mean, t_var, t_std);


void
zx::tm::proc(const zx::graph& graph) noexcept {

    auto compare = [](const zx::match& a, const zx::match& b) noexcept {
        return (a.score > b.score);
    };

    std::priority_queue<zx::match, std::vector<zx::match>, decltype(compare)> pos {};

    core::matches.clear();

    //double bestScore = -2.0f;

    for (u32 y = 0; y < (graph.height - core::size.h -1); y+=core::size.h) {
        for (u32 x = 0; x < (graph.width - core::size.w -1); x+=core::size.w) {

            double meanI = 0.0, stdI = 0.0, corr = 0.0;

            for (u32 j = 0; j < core::size.h; j++) {
                for (u32 i = 0; i < core::size.w; i++) {
                    meanI += f64(graph.data[(y+j) * graph.width + (x+i)]);
                }
            }

            meanI /= f64(core::msksz);

            // Compute correlation and variance
            for (u32 j = 0; j < core::size.h; j++) {
                for (u32 i = 0; i < core::size.w; i++) {
                    double imgPix = graph.data[(y+i) * graph.width + (x+i)];//getPixel(image, x + i, y + j);
                    double patPix = core::mask[0].data[j * core::mask[0].width + i];//pattern[j * pw + i];
                    corr += (imgPix - meanI) * (patPix - core::mask[0].mean);
                    stdI += (imgPix - meanI) * (imgPix - meanI);
                }
            }

            stdI = std::sqrt(stdI);

            double score = corr / (stdI * core::mask[0].step + 1e-9); // avoid div by zero

            if (pos.size() < core::max) {
                pos.push({score,{x,y,core::size.w,core::size.h}});
            } else if (score > pos.top().score) {
                pos.pop();
                pos.push({score,{x,y,core::size.w, core::size.h}});
            }

            // if (score > bestScore) {
            //     bestScore = score;
            //     core::match = {x, y, core::size.w, core::size.h };
            // }

        }
    }

    while(!pos.empty()) {
        core::matches.push_back(pos.top());
        //std::fprintf(stderr,"top 20: %f [%dx%d]\n",pos.top().score, pos.top().area.x, pos.top().area.y);
        pos.pop();
    }


}



void
zx::tm::mask(const su32 size) noexcept {


    const u08 bg = 0x00, fg = 0xFF;

    for (u32 i = 0; i < 4; ++i) {                // locação de memoria
        core::mean[i] = 0.0f;
        core::stdp[i] = 0.0f;
        core::mask[i].width  = size.w;
        core::mask[i].height = size.h;
        core::mask[i].size   = size.w * size.h;
        core::mask[i].data   = new u08[core::mask[i].size];
        for (u32 k = 0; k < core::mask[i].size; ++k) {
            core::mask[i].data[k] = bg;
        }
    }

    for (u32 i = size.w/2; i < size.w; ++i) {
        const u32 i0 = (size.h/2) * size.w + i;
        core::mask[0].data[i0] = fg;
        core::mask[2].data[i0] = fg;
    }

    for (u32 i = 0; i < size.w/2; ++i) {
        const u32 i0 = (size.h/2) * size.w + i;
        core::mask[1].data[i0] = fg;
        core::mask[3].data[i0] = fg;
    }


    for (u32 j = 0; j < (size.h/2+1); ++j) {
        const u32 i0 = j * size.w + (size.w/2);
        core::mask[2].data[i0] = fg;
        core::mask[3].data[i0] = fg;

    }

    for (u32 j = size.h/2; j < size.h; ++j) {
        const u32 i0 = j * size.w + (size.w/2);
        core::mask[0].data[i0] = fg;
        core::mask[1].data[i0] = fg;
    }


    // for (u32 i = 0; i < size.w; ++i) {
    //     const u32 i0 = i;                        // geraçao de mascaras de tamanho qualquer
    //     const u32 i1 = (size.h -1) * size.w + i; // XXXX (mask 0) XXXX (mask 1)
    //     core::mask[0].data[i0] = 0x00;           // X                X
    //     core::mask[1].data[i0] = 0x00;           //
    //     core::mask[2].data[i1] = 0x00;           // X    (mask 2     X (mask 3)
    //     core::mask[3].data[i1] = 0x00;           // XXXX          XXXX
    // }
    //
    // for (u32 j = 0; j < size.h; ++j) {
    //     const u32 i0 = j * size.w;
    //     const u32 i1 = j * size.w + (size.w -1);
    //     core::mask[0].data[i0] = 0x00;
    //     core::mask[1].data[i1] = 0x00;
    //     core::mask[2].data[i0] = 0x00;
    //     core::mask[3].data[i1] = 0x00;
    // }

    for (u32 j = 0; j < 4; ++j) {
        for (u32 i = 0; i < core::mask[j].size; ++i) {
            core::mean[j] += f32(core::mask[j].data[i]);
        }
        core::mean[j] /= f32(core::mask[j].size);
    }

    for (u32 j = 0; j < 4; ++j) {
        for (u32 i = 0; i < core::mask[j].size; ++i) {
            const f32 pixel = f32(core::mask[j].data[i]);
            core::stdp[j]  += (pixel - core::mean[j]) * (pixel - core::mean[j]);
        }
        core::stdp[j] = std::sqrt(core::stdp[j]);
    }

    for (u32 j = 0; j < 4; ++j) {
        std::fprintf(stderr, "mean[%d]: %f stdp[%d]: %f\n", j, core::mean[j], j, core::stdp[j]);
    }
}

const zx::graph&
zx::tm::mask(const u32 index) noexcept {
    return core::mask[index];
}

zx::ru32
zx::tm::mask (void) noexcept {
    return core::match;
}

const std::vector<zx::match>&
zx::tm::matches(void) noexcept {
    return core::matches;
}



std::vector<std::vector<int>> computeIntegral(const std::vector<std::vector<uint8_t>>& img) {
    zx::u64 H = img.size(), W = img[0].size();
    std::vector<std::vector<int>> integral(H+1, std::vector<int>(W+1, 0));
    for (zx::u32 y = 1; y <= H; y++) {
        for (zx::u32 x = 1; x <= W; x++) {
            integral[y][x] = img[y-1][x-1] + integral[y-1][x] + integral[y][x-1] - integral[y-1][x-1];
        }
    }
    return integral;
}

std::vector<std::vector<int>> computeIntegral(const std::vector<std::vector<int>>& img) {
    zx::u64 H = img.size(), W = img[0].size();
    std::vector<std::vector<int>> integral(H+1, std::vector<int>(W+1, 0));
    for (zx::u32 y = 1; y <= H; y++) {
        for (zx::u32 x = 1; x <= W; x++) {
            integral[y][x] = img[y-1][x-1] + integral[y-1][x] + integral[y][x-1] - integral[y-1][x-1];
        }
    }
    return integral;
}


inline int rectSum(const std::vector<std::vector<int>>& integral, zx::u32 x, zx::u32 y, zx::u32 w, zx::u32 h) {
    return integral[y+h][x+w] - integral[y][x+w] - integral[y+h][x] + integral[y][x];
}

std::vector<std::vector<float>> matchTemplateNCC(
    const std::vector<std::vector<uint8_t>>& img,
    const std::vector<std::vector<uint8_t>>& tpl) {

    int H = img.size(), W = img[0].size();
    int h = tpl.size(), w = tpl[0].size();

    // Precompute template stats
    float t_sum = 0, t_sq_sum = 0;
    for (int yy = 0; yy < h; yy++) {
        for (int xx = 0; xx < w; xx++) {
            float val = tpl[yy][xx];
            t_sum += val;
            t_sq_sum += val * val;
        }
    }

    float t_mean = t_sum / (w * h);
    float t_var = t_sq_sum / (w * h) - t_mean * t_mean;
    float t_std = std::sqrt(std::max(1e-6f, t_var));

    // Precompute integral images for sum and sum of squares
    auto integral = computeIntegral(img);
    std::vector<std::vector<int>> imgSq(img.size(), std::vector<int>(img[0].size()));
    for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++)
            imgSq[y][x] = img[y][x] * img[y][x];
    auto integralSq = computeIntegral(imgSq);

    // Output NCC map
    std::vector<std::vector<float>> result(H-h+1, std::vector<float>(W-w+1));

    // Sliding window
    for (int y = 0; y <= H-h; y++) {
        for (int x = 0; x <= W-w; x++) {
            // Patch stats
            float p_sum = rectSum(integral, x, y, w, h);
            float p_sq_sum = rectSum(integralSq, x, y, w, h);
            float p_mean = p_sum / (w * h);
            float p_var = p_sq_sum / (w * h) - p_mean * p_mean;
            float p_std = std::sqrt(std::max(1e-6f, p_var));

            // Compute numerator: sum( (I-meanI)*(T-meanT) )
            float numerator = 0.0f;
            for (int yy = 0; yy < h; yy++) {
                for (int xx = 0; xx < w; xx++) {
                    float Iv = img[y+yy][x+xx] - p_mean;
                    float Tv = tpl[yy][xx] - t_mean;
                    numerator += Iv * Tv;
                }
            }
            float denom = (w * h - 1) * t_std * p_std;
            result[y][x] = numerator / denom;
        }
    }
    return result;
}


int proc2() {
    // Example: random image 480x640
    int H = 480, W = 640;
    std::vector<std::vector<uint8_t>> image(H, std::vector<uint8_t>(W));
    for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++)
            image[y][x] = rand() % 256;

    // 4 random 5x5 templates
    std::vector<std::vector<std::vector<uint8_t>>> templates;
    for (int t = 0; t < 4; t++) {
        std::vector<std::vector<uint8_t>> tpl(5, std::vector<uint8_t>(5));
        for (int yy = 0; yy < 5; yy++)
            for (int xx = 0; xx < 5; xx++)
                tpl[yy][xx] = rand() % 256;
        templates.push_back(tpl);
    }

    // Run matching
    for (int i = 0; i < 4; i++) {
        auto ncc_map = matchTemplateNCC(image, templates[i]);

        // Find best match
        float bestVal = -std::numeric_limits<float>::infinity();
        int bestX = 0, bestY = 0;
        for (int y = 0; y < (int)ncc_map.size(); y++) {
            for (int x = 0; x < (int)ncc_map[0].size(); x++) {
                if (ncc_map[y][x] > bestVal) {
                    bestVal = ncc_map[y][x];
                    bestX = x;
                    bestY = y;
                }
            }
        }
        std::cout << "Template " << i
        << " best NCC score = " << bestVal
        << " at (" << bestX << ", " << bestY << ")\n";
    }
}

#endif

#if 0


std::vector<std::vector<int>> computeIntegral(const std::vector<std::vector<uint8_t>>& img) {
    zx::u64 H = img.size(), W = img[0].size();
    std::vector<std::vector<int>> integral(H+1, std::vector<int>(W+1, 0));
    for (zx::u32 y = 1; y <= H; y++) {
        for (zx::u32 x = 1; x <= W; x++) {
            integral[y][x] = img[y-1][x-1] + integral[y-1][x] + integral[y][x-1] - integral[y-1][x-1];
        }
    }
    return integral;
}

inline int rectSum(const std::vector<std::vector<int>>& integral, zx::u32 x, zx::u32 y, zx::u32 w, zx::u32 h) {
    return integral[y+h][x+w] - integral[y][x+w] - integral[y+h][x] + integral[y][x];
}


#include <iostream>
#include <vector>
#include <cmath>
#include <limits>

// Build integral image (for fast sum computation)
std::vector<std::vector<int>> computeIntegral(const std::vector<std::vector<uint8_t>>& img) {
    int H = img.size(), W = img[0].size();
    std::vector<std::vector<int>> integral(H+1, std::vector<int>(W+1, 0));

    for (int y = 1; y <= H; y++) {
        for (int x = 1; x <= W; x++) {
            integral[y][x] = img[y-1][x-1]
                           + integral[y-1][x]
                           + integral[y][x-1]
                           - integral[y-1][x-1];
        }
    }
    return integral;
}

// Compute sum of region using integral image
inline int rectSum(const std::vector<std::vector<int>>& integral, int x, int y, int w, int h) {
    return integral[y+h][x+w] - integral[y][x+w] - integral[y+h][x] + integral[y][x];
}

// NCC for one template
std::vector<std::vector<float>> matchTemplateNCC(
        const std::vector<std::vector<uint8_t>>& img,
        const std::vector<std::vector<uint8_t>>& tpl) {

    int H = img.size(), W = img[0].size();
    int h = tpl.size(), w = tpl[0].size();

    // Precompute template stats
    float t_sum = 0, t_sq_sum = 0;
    for (int yy = 0; yy < h; yy++) {
        for (int xx = 0; xx < w; xx++) {
            float val = tpl[yy][xx];
            t_sum += val;
            t_sq_sum += val * val;
        }
    }
    float t_mean = t_sum / (w * h);
    float t_var = t_sq_sum / (w * h) - t_mean * t_mean;
    float t_std = std::sqrt(std::max(1e-6f, t_var));

    // Precompute integral images for sum and sum of squares
    auto integral = computeIntegral(img);
    std::vector<std::vector<int>> imgSq(img.size(), std::vector<int>(img[0].size()));
    for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++)
            imgSq[y][x] = img[y][x] * img[y][x];
    auto integralSq = computeIntegral(imgSq);

    // Output NCC map
    std::vector<std::vector<float>> result(H-h+1, std::vector<float>(W-w+1));

    // Sliding window
    for (int y = 0; y <= H-h; y++) {
        for (int x = 0; x <= W-w; x++) {
            // Patch stats
            float p_sum = rectSum(integral, x, y, w, h);
            float p_sq_sum = rectSum(integralSq, x, y, w, h);
            float p_mean = p_sum / (w * h);
            float p_var = p_sq_sum / (w * h) - p_mean * p_mean;
            float p_std = std::sqrt(std::max(1e-6f, p_var));

            // Compute numerator: sum( (I-meanI)*(T-meanT) )
            float numerator = 0.0f;
            for (int yy = 0; yy < h; yy++) {
                for (int xx = 0; xx < w; xx++) {
                    float Iv = img[y+yy][x+xx] - p_mean;
                    float Tv = tpl[yy][xx] - t_mean;
                    numerator += Iv * Tv;
                }
            }
            float denom = (w * h - 1) * t_std * p_std;
            result[y][x] = numerator / denom;
        }
    }
    return result;
}

int main() {
    // Example: random image 480x640
    int H = 480, W = 640;
    std::vector<std::vector<uint8_t>> image(H, std::vector<uint8_t>(W));
    for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++)
            image[y][x] = rand() % 256;

    // 4 random 5x5 templates
    std::vector<std::vector<std::vector<uint8_t>>> templates;
    for (int t = 0; t < 4; t++) {
        std::vector<std::vector<uint8_t>> tpl(5, std::vector<uint8_t>(5));
        for (int yy = 0; yy < 5; yy++)
            for (int xx = 0; xx < 5; xx++)
                tpl[yy][xx] = rand() % 256;
        templates.push_back(tpl);
    }

    // Run matching
    for (int i = 0; i < 4; i++) {
        auto ncc_map = matchTemplateNCC(image, templates[i]);

        // Find best match
        float bestVal = -std::numeric_limits<float>::infinity();
        int bestX = 0, bestY = 0;
        for (int y = 0; y < (int)ncc_map.size(); y++) {
            for (int x = 0; x < (int)ncc_map[0].size(); x++) {
                if (ncc_map[y][x] > bestVal) {
                    bestVal = ncc_map[y][x];
                    bestX = x;
                    bestY = y;
                }
            }
        }
        std::cout << "Template " << i
                  << " best NCC score = " << bestVal
                  << " at (" << bestX << ", " << bestY << ")\n";
    }
}

#endif


#if 0
[[maybe_unused]]
static std::vector<std::vector<int>>
computeIntegral(const zx::graph& graph) noexcept {
    const zx::u32 wd = graph.width;
    const zx::u32 ht = graph.height;
    std::vector<std::vector<int>> integral(ht+1, std::vector<int>(wd+1, 0));

  //std::memset(core::imgint1.data, 0, core::imgint1.size * sizeof(i32));

    for (zx::u32 y = 1; y <= ht; y++) {
        for (zx::u32 x = 1; x <= wd; x++) {
            integral[y][x] = graph.data[(y-1) * graph.width + (x-1)] +
                             integral[y-1][x] +
                             integral[y][x-1] -
                             integral[y-1][x-1];
        }
    }
    return integral;
}

[[maybe_unused]]
static std::vector<std::vector<int>>
computeIntegral(const std::vector<std::vector<int>>& img) {
    zx::u64 H = img.size(), W = img[0].size();
    std::vector<std::vector<int>> integral(H+1, std::vector<int>(W+1, 0));
    for (zx::u32 y = 1; y <= H; y++) {
        for (zx::u32 x = 1; x <= W; x++) {
            integral[y][x] = img[y-1][x-1] + integral[y-1][x] + integral[y][x-1] - integral[y-1][x-1];
        }
    }
    return integral;
}
inline int
rectSum(const std::vector<std::vector<int>>& integral, zx::u32 x, zx::u32 y, zx::u32 w, zx::u32 h) {
    return integral[y+h][x+w] -
           integral[y][x+w] -
           integral[y+h][x] +
           integral[y][x];
}
#endif
