
//#include <cstdio>
#include <algorithm>
//#include <chrono>
#include "view.hpp"
#include "drvr.hpp"
#include "nccp.hpp"

namespace core {
    static zx::graph graph {};    // full image
    static zx::graph gecho {};    // previous lower resolution image
    static zx::graph deres {};    // actual   lower resoultion image
    static zx::su32  vsize {};    // view   size
    static zx::su32  csize {};    // camera size
    static zx::su32  block {2,2}; // block  size
    static zx::su32  glyph {15,15}; // glyph size
    static zx::su32  lower {};    // lower resolution image size (640/blockx480/block);
    static zx::f32   diff  {};

  //static zx::u32   proc  {5};
}

void
zx::vw::init (const su32 size) noexcept {

    wc::cam_init_impl("/dev/video2");

    core::vsize = size;
    core::csize = wc::cam_info_impl();
    core::lower = { core::csize.w / core::block.w, core::csize.h / core::block.h };

    core::graph.width  = size.w;
    core::graph.height = size.h;
    core::graph.size   = size.w * size.h;
    core::graph.data   = new u08[core::graph.size];

    core::gecho.width  = size.w;
    core::gecho.height = size.h;
    core::gecho.size   = size.w * size.h;
    core::gecho.data   = new u08[core::graph.size];

    core::deres.width  = core::lower.w;
    core::deres.height = core::lower.h;
    core::deres.size   = core::lower.w * core::lower.h;
    core::deres.data   = new u08[core::deres.size];

    core::gecho.width  = core::lower.w;
    core::gecho.height = core::lower.h;
    core::gecho.size   = core::lower.w * core::lower.h;
    core::gecho.data   = new u08[core::gecho.size];

    tm::init(core::glyph, core::lower);//{core::deres.width, core::deres.height});

    //std::fprintf(stderr, "camera [%dx%d] block [%dx%d] lower [%dx%d]\n",
    //             core::csize.w, core::csize.h, core::block.w, core::block.h, core::lower.w, core::lower.h);
}

void
zx::vw::size (const su32 size) noexcept {
    core::vsize = size;

    if (nullptr != core::graph.data)
        delete [] core::graph.data;

    core::graph.width  = size.w;
    core::graph.height = size.h;
    core::graph.size   = size.w * size.h;
    core::graph.data   = new u08[core::graph.size];
}

void
zx::vw::stop (void) noexcept {

    wc::cam_stop_impl();

    delete [] core::graph.data;
    delete [] core::deres.data;
    delete [] core::gecho.data;

    core::graph.data = nullptr;
    core::deres.data = nullptr;
    core::gecho.data = nullptr;

    tm::stop();
}

void
zx::vw::exec (void) noexcept {

    if ( wc::cam_read_impl() ) return;         // skip when busy

    copy( core::deres, core::gecho);           // camera buffer echo   -> low res

    copy( wc::cam_gray_impl(), core::deres);   // camera buffer        -> low res

    norm(core::deres);

    test( core::deres, core::gecho);           // test image variation -> low res

    //const auto t_init = std::chrono::high_resolution_clock::now();
    //if (core::proc) {

    //if (core::diff > 10.0f)
        tm::proc(core::deres, 0.80f);              // low res -> image detection

    //--core::proc;
    //} else {
    //tm::find();
    //}
    //const auto t_stop
    //const std::chrono::time_point<std::chrono::system_clock> t_stop = std::chrono::high_resolution_clock::now();
    //const time_point (aka const time_point<std::chrono::system_clock, duration<long, ratio<1, 1000000000>>>)
    //const auto t_time = std::chrono::duration<double, std::milli>(t_stop - t_init).count();
    //std::fprintf(stderr,"\e[01;32mproc time: %4.4f ms\n", t_time);

    // for (const zx::match& m : tm::matches()) { // render detection positon
    //     rect(core::deres, m.area, 255);
    // }
    //
    // for (const zx::match& m : tm::lots()) { // render detection positon
    //     rect(core::deres, m.area, 0);
    // }

    //std::fprintf(stderr, "match count: %ld\n", tm::matches().size());
    //copy(tm::glyph(3), core::deres);
    //if (core::proc) {
        // for (const zx::match& m : tm::matches()) { // render detection positon
        //     rect(core::deres, m.area, 255);
        // }

    copy(core::deres, core::graph);            // low res -> to window buffer


    for (const zx::match& m : tm::lots()) { // render detection positon
        const u32 da = core::block.w;
        const ru32 area { m.area.x * da, m.area.y * da, (m.area.w + core::glyph.w) * da, (m.area.h + core::glyph.h) *da};

        quad(core::graph, area, 0);
    }

    for (const zx::match& m : tm::matches()) { // render detection positon
        const u32 da = core::block.w;
        const ru32 area { m.area.x * da, m.area.y * da, m.area.w * da, m.area.h * da};
        rect(core::graph, area, 255);
    }


    // } else {
    //     copy(wc::cam_gray_impl(), core::graph);
    //     for (const zx::match& m : tm::matches()) { // render detection positon
    //         const ru32 area = { m.area.x * 3, m.area.y * 3, m.area.w * 3 , m.area.h * 3 };
    //         rect(core::graph, area, 255);
    //     }
    // }
    //copy(wc::cam_gray_impl(), core::graph);

    //std::fprintf(stderr, "image variation: %f\n", core::diff);
}

const zx::graph&
zx::vw::data (void) noexcept {
    return core::graph;
}

void
zx::vw::rect(const zx::graph& graph, const ru32& area, const u08 color) noexcept {
    const u32 sx = area.x;
    const u32 sy = area.y;
    const u32 ex = area.w + sx;
    const u32 ey = area.h + sy;

    for (u32 i = sx; i < ex; ++i) {
        graph.data[ (sy+0) * graph.width + i ] = color;
        graph.data[ (ey-1) * graph.width + i ] = color;
    }

    for (u32 j = sy; j < ey; ++j) {
        graph.data[ j * graph.width + (sx+0) ] = color;
        graph.data[ j * graph.width + (ex-1) ] = color;
    }
}


void
zx::vw::quad(const zx::graph& graph, const ru32& area, const u08 color) noexcept {
    const u32 sx = area.x;
    const u32 sy = area.y;
    const u32 ex = area.w;
    const u32 ey = area.h;

    for (u32 i = sx; i < ex; ++i) {
        graph.data[ (sy+0) * graph.width + i ] = color;
        graph.data[ (ey-1) * graph.width + i ] = color;
    }

    for (u32 j = sy; j < ey; ++j) {
        graph.data[ j * graph.width + (sx+0) ] = color;
        graph.data[ j * graph.width + (ex-1) ] = color;
    }
}

void
zx::vw::lerp(const zx::graph& src, const zx::graph& dst) noexcept {

    const f32 x_ratio = (f32(src.width)  - 1.0f) / (f32(dst.width)  - 1.0f);
    const f32 y_ratio = (f32(src.height) - 1.0f) / (f32(dst.height) - 1.0f);

    for (u32 y = 0; y < dst.height; y++) {
        const f32 src_y = f32(y) * y_ratio;
        const u32 y0 = u32(src_y);
        const u32 y1 = std::min(y0 + 1, src.height - 1);
        const f32 dy = src_y - f32(y0);

        for (u32 x = 0; x < dst.width; x++) {
            const f32 src_x = f32(x) * x_ratio;
            const u32 x0 = u32(src_x);
            const u32 x1 = std::min(x0 + 1, src.width - 1);
            const f32 dx = src_x - f32(x0);

            const u08 p00 = src.data[y0 * src.width + x0];
            const u08 p10 = src.data[y0 * src.width + x1];
            const u08 p01 = src.data[y1 * src.width + x0];
            const u08 p11 = src.data[y1 * src.width + x1];

            const f32 top    = f32(p00) * (1 - dx) + f32(p10) * dx;
            const f32 bottom = f32(p01) * (1 - dx) + f32(p11) * dx;
            const f32 value  = top * (1 - dy) + bottom * dy;

            dst.data[y * dst.width + x] = u08(value);
        }
    }
}

void
zx::vw::copy(const zx::graph& src, const zx::graph& dst) noexcept {
    for (u32 y = 0; y < dst.height; y++) {
        u32 src_y = y * src.height / dst.height;
        for (u32 x = 0; x < dst.width; x++) {
            u32 src_x = x * src.width / dst.width;
            dst.data[y * dst.width + x] = src.data[src_y * src.width + src_x];
        }
    }
}


void
zx::vw::test(const zx::graph& src, const zx::graph& dst) noexcept {
   core::diff = 0.0f;
   for (u32 i = 0; i < src.size; ++i) {
        core::diff += f32(dst.data[i] - src.data[i]);
   }
   core::diff = std::abs(core::diff / f32(src.size));
}


void
zx::vw::draw(const zx::graph& src, const u08 color) noexcept {
    for (u32 i = 0; i < src.size; ++i) src.data[i] = color;
}

void
zx::vw::norm(const zx::graph& src) noexcept {

    u08 max = 0, min = 255;
    for (u32 i = 0; i < src.size; ++i) {
        const u08 value = src.data[i];
        max = value > max ? value : max;
        min = value < min ? value : min;
    }

    for (u32 i = 0; i < src.size; ++i) {
        src.data[i] = u08((src.data[i] - min) * 255 / (max - min + 1));
    }
}

const std::vector<zx::match>&
zx::vw::lots(void) noexcept {
    return tm::lots();
}

