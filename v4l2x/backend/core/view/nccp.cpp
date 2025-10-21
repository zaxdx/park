
#include <cstdio>
#include <vector>
//#include <cstdlib>
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
    static zx::graph              glyphs[2] {}; // expanded glyphs
    static std::vector<zx::match> matches   {}; // image detection matches
    static std::vector<zx::match> lots      {}; // parking lots
}

void
zx::tm::init(const su32 gsize, const su32 fsize) noexcept {

    if (gsize.w > core::ksize.w and gsize.h > core::ksize.h and gsize.w == gsize.h) {
        core::gsize = gsize;
    } else {
        core::gsize = core::ksize;
    }

    core::matches.clear();

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

    for (u32 g = 0; g < 2; ++g)
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
        //std::fprintf(stderr, "glyph[%d] mean: %f deviation: %f\n", g, core::glyphs[g].mean, core::glyphs[g].stdv);
    }

    //std::fprintf(stderr, "init done...\n");
}


[[maybe_unused]]
static zx::f32
compute(const zx::graph& src, const zx::graph& tpl, zx::u32 sx, zx::u32 sy) {

    using zx::u32, zx::f32, zx::u08;

    const u32 tw = tpl.width;
    const u32 th = tpl.height;
    const u32 sz = tw * th;

    f32 sum_src = 0.0f;
    for (u32 y = 0; y < th; ++y) {
        for (u32 x = 0; x < tw; ++x) {
            sum_src += f32(src.data[(sy+y) * src.width + (sx+x)]);
        }
    }

    const f32 src_mean = sum_src / f32(sz);
    const f32 tpl_mean = tpl.mean;

    f32 numer = 0.0f;
    f32 denom_src_sq = 0.0f;
    for (u32 y = 0; y < th; ++y) {
        for (u32 x = 0; x < tw; ++x) {
            const f32 s = f32(src.data[(sy+y) * src.width + (sx+x)]) - src_mean;
            const f32 t = f32(tpl.data[  y    * tpl.width +    x  ]) - tpl_mean;
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
    for (u32 g = 0; g < 2; ++g) {
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

    for (u32 g = 0; g < 2; ++g)
    {
        for (u32 y = 0; y < out_h; ++y)
        {
            for (u32 x = 0; x < out_w; ++x)
            {
                if (core::bypass.data[y * core::fsize.w + x] == 1) continue;

                f32 score = compute(graph, core::glyphs[g], x, y);

                if (score >= min)
                {
                    for (u32 j = (y-1); j < (y+core::gsize.h+1); ++j) {
                        for (u32 i = (x-1); i < (x+core::gsize.w+1); ++i) {
                            core::bypass.data[j * core::fsize.w + i] = 1;
                        }
                    }
                    core::matches.emplace_back(zx::match{ g, score, {x,y,core::gsize.w,core::gsize.h} });
                }
            }
        }
    }

    site();
}

void
zx::tm::site(void) noexcept {

    std::vector<zx::pu32> tlv {};
    //std::vector<zx::pu32> trv {};
    //std::vector<zx::pu32> blv {};
    std::vector<zx::pu32> brv {};

    core::lots.clear();

    for (const zx::match& m : core::matches) {
        switch (m.mask) {
            case 0: tlv.emplace_back(pu32{m.area.x,m.area.y}); break;
            //case 1: trv.emplace_back(pu32{m.area.x,m.area.y}); break;
            //case 2: blv.emplace_back(pu32{m.area.x,m.area.y}); break;
            case 1: brv.emplace_back(pu32{m.area.x,m.area.y}); break;
        }
    }

    for (const zx::pu32& tl : tlv)    {
    //const pu32 tl = tlv[0];
        // u32  tlbld = 0xFFFFFFFF;
        // pu32 tlblp {};
        // for (const zx::pu32& bl : blv) {
        //     const u32 dist = (tl.x - bl.x) * (tl.x - bl.x) + (tl.y - bl.y) * (tl.y - bl.y);
        //     if (dist < tlbld) {
        //         tlbld = dist;
        //         tlblp = bl;
        //     }
        // }
        //
        // u32  tltrd = 0xFFFFFFFF;
        // pu32 tltrp {};
        // for (const zx::pu32& tr : trv) {
        //     const u32 dist = (tl.x - tr.x) * (tl.x - tr.x) + (tl.y - tr.y) * (tl.y - tr.y);
        //     if (dist < tltrd) {
        //         tltrd = dist;
        //         tltrp = tr;
        //     }
        // }

        u32  tlbrd = 0xFFFFFFFF;
        pu32 tlbrp {};
        for (const zx::pu32& br : brv) {
            const u32 dist = (tl.x - br.x) * (tl.x - br.x) + (tl.y - br.y) * (tl.y - br.y);
            if (tlbrd > dist and br.x > tl.x and br.y > tl.y) {
                tlbrd = dist;
                tlbrp = br;
            }
        }

        core::lots.emplace_back(zx::match{0,0,{tl.x - 1, tl.y - 1, tlbrp.x + 1, tlbrp.y + 1}});
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

const std::vector<zx::match>&
zx::tm::lots(void) noexcept {
    return core::lots;
}

