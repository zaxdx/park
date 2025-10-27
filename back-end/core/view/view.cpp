
#include <cstdio>
#include <cmath>
#include <fstream>

#include "view.hpp"
#include "drvr.hpp"
#include "nccp.hpp"

namespace core {

    using parking = std::vector<zx::match>;

    static zx::graph graph {};      // full image
    static zx::graph gecho {};      // previous lower resolution image
    static zx::graph deres {};      // actual   lower resoultion image
    static parking   lots  {};
    static parking   sets  {};
    static zx::su32  vsize {};      // view   size
    static zx::su32  csize {};      // camera size
    static zx::su32  block { 2, 2}; // block  size
    static zx::su32  glyph {15,15}; // glyph size
    static zx::su32  lower {};      // lower resolution image size (640/blockx480/block);
    static zx::f32   diff  {};
    static zx::state state {};
    static bool      print {};
}

void
zx::vw::init (const su32 size) noexcept {

    wc::cam_init_impl("/dev/video0");

    core::vsize = size;
    core::csize = wc::cam_info_impl();
    core::lower = { core::csize.w / core::block.w, core::csize.h / core::block.h };

    core::graph.width  = size.w;
    core::graph.height = size.h;
    core::graph.size   = size.w * size.h;
    core::graph.data   = new u08[core::graph.size];

    core::deres.width  = core::lower.w;
    core::deres.height = core::lower.h;
    core::deres.size   = core::lower.w * core::lower.h;
    core::deres.data   = new u08[core::deres.size];

    core::gecho.width  = core::lower.w;
    core::gecho.height = core::lower.h;
    core::gecho.size   = core::lower.w * core::lower.h;
    core::gecho.data   = new u08[core::gecho.size];

    core::state        = zx::state::NONE;

    tm::init(core::glyph, core::lower);
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

    core::lots.clear();
    core::sets.clear();

    tm::stop();
}

void
zx::vw::remap(void) noexcept {
    core::state = zx::state::REMAP;
}

void
zx::vw::update(void) noexcept {
    core::state = zx::state::UPDATE;
}

void
zx::vw::check(void) noexcept {
    core::state = zx::state::CHECK;
}

void
zx::vw::exec (void) noexcept {

    if ( wc::cam_read_impl() ) return;         // skip when busy

    copy( wc::cam_gray_impl(), core::deres);   // camera buffer        -> low res
    norm( core::deres );                       // normaliza cores n..m -> 0..255

    if ( core::state == zx::state::UPDATE ) {

        core::diff = diff( core::deres, core::gecho); // test image variation -> low res

        copy(core::deres, core::gecho);               // camera buffer echo -> low res

        if (core::diff > 0.5f) return;                // ignore camera bounce

        tm::proc(core::deres, 0.80f);                 // low res -> image detection

        const u64 sets = tm::matches().size();
        const u64 lots = tm::lots().size();

        if ( sets and lots and lots == sets / 2 ) {
            core::lots.resize( lots );
            core::sets.resize( sets );
            std::copy(tm::matches().begin(), tm::matches().end(), core::sets.begin());
            std::copy(tm::lots().begin(),    tm::lots().end(),    core::lots.begin());

            core::print = true;
        }

    } else if ( core::state == zx::state::CHECK ) {

        for (u32 i = 0; i < core::lots.size(); ++i ) {

            zx::match& lot = core::lots[i];

            ru32 area = lot.area;
            area.w += core::glyph.w;
            area.h += core::glyph.h;

            lot.score = diff( core::deres, core::gecho, area);

            if (lot.score > 0.25f) {
                if (lot.busy == 0) core::print = true;
                lot.busy = 1;
                fill( core::deres, area, 100 );
            } else {
                if (lot.busy == 1) core::print = true;
                lot.busy = 0;
            }
        }

        if (core::print) {
            print();
            core::print = false;
        }
    }

    if ( core::sets.size() and core::lots.size() ) {
        for (const zx::match& m : core::sets) {
            const ru32 area { m.area.x, m.area.y, m.area.w, m.area.h};
            rect(core::deres, area, 255);
        }

        for (const zx::match& m : core::lots) {
            const ru32 area { m.area.x,  m.area.y, (m.area.w + core::glyph.w), (m.area.h + core::glyph.h)};
            quad(core::deres, area, 200);
        }
    }

    copy(core::deres, core::graph);
}

const zx::graph&
zx::vw::data (void) noexcept {
    return core::graph;
}

void
zx::vw::fill(const zx::graph& dst, const ru32& area, const u08 color) noexcept {
    for (u32 j = area.y; j < area.h; ++j) {
        for (u32 i = area.x; i < area.w; ++i) {
            const u32 index = j * dst.width + i;
            dst.data[index] = color;
        }
    }
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
zx::vw::copy(const zx::graph& src, const zx::graph& dst) noexcept {
    for (u32 y = 0; y < dst.height; y++) {
        u32 src_y = y * src.height / dst.height;
        for (u32 x = 0; x < dst.width; x++) {
            u32 src_x = x * src.width / dst.width;
            dst.data[y * dst.width + x] = src.data[src_y * src.width + src_x];
        }
    }
}

zx::f32
zx::vw::diff(const zx::graph& src, const zx::graph& dst, const ru32& area) noexcept {

    f64 mean1 = 0.0f, mean2 = 0.0f;
    f64 numerator = 0.0f;
    f64 denom1 = 0.0f, denom2 = 0.0f;
    u32 size  = 0;

    for (u32 j = area.y; j < area.h; ++j) {
        for (u32 i = area.x; i < area.w; ++i) {
            const u32 index = j * src.width + i;
            mean1 += src.data[index];
            mean2 += dst.data[index];
            ++size;
        }
    }

    mean1 /= size;
    mean2 /= size;

    for (u32 j = area.y; j < area.h; ++j) {
        for (u32 i = area.x; i < area.w; ++i) {
            const u32 index = j * src.width + i;
            const f64 diff1 = src.data[index] - mean1;
            const f64 diff2 = dst.data[index] - mean2;
            numerator += diff1 * diff2;
            denom1    += diff1 * diff1;
            denom2    += diff2 * diff2;
        }
    }

    const f64 denominator = std::sqrt(denom1 * denom2);

    if (denominator == 0.0) return 0.0f;

    f64 ncc = numerator / denominator;

    return (1.0f - f32(ncc));
}

zx::f32
zx::vw::diff(const zx::graph& src, const zx::graph& dst) noexcept {

    f64 mean1 = 0.0f, mean2 = 0.0f;
    f64 numerator = 0.0f;
    f64 denom1 = 0.0f, denom2 = 0.0f;

    for (size_t i = 0; i < src.size; ++i) {
        mean1 += src.data[i];
        mean2 += dst.data[i];
    }
    mean1 /= src.size;
    mean2 /= dst.size;

    for (size_t i = 0; i < src.size; ++i) {
        const f64 diff1 = src.data[i] - mean1;
        const f64 diff2 = dst.data[i] - mean2;
        numerator += diff1 * diff2;
        denom1    += diff1 * diff1;
        denom2    += diff2 * diff2;
    }

    const f64 denominator = std::sqrt(denom1 * denom2);

    if (denominator == 0.0) return 0.0f;

    f64 ncc = numerator / denominator;

    return ( 1.0f - f32(ncc) );
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


void
zx::vw::print (void) noexcept {

    std::ofstream ofs("/usr/share/nginx/html/face/image.svg", std::ios::binary);

    std::string svg;

    const u32 wd = core::deres.width;
    const u32 ht = core::deres.height;
    const u32 h3 = 3 * (ht / 4);

    svg.reserve(768);

    svg = std::format ( R"(<svg width="{}" height="{}" viewBox="0 0 {} {}" xmlns="http://www.w3.org/2000/svg">)",
        wd, ht, wd, ht
    );
    svg += "\n";

    svg += std::format( R"( <rect width="{}" height="{}" fill="#222" />)",
        wd, ht
    );
    svg += "\n";

    svg += std::format( R"( <line x1="100" y1="{}" x2="{}" y2="{}" stroke="#fff" stroke-width="3" stroke-dasharray="10,10"/>)",
        h3, wd, h3
    );
    svg += "\n";

    for (const auto m : core::lots) {

        const u32 sx = m.area.x + 2;
        const u32 sy = m.area.y + 2;
        const u32 ex = m.area.w - sx - 2;
        const u32 ey = m.area.h - sy - 2;

        svg += std::format(
            R"(  <rect x="{}"  y="{}"  width="{}" height="{}" fill="{}"/>)",
            sx, sy, ex, ey, (1 == m.busy ? "#A42F3E" : "#34A359")
        );
        svg += "\n";

        svg += std::format(
            R"(  <text x="{}" y="{}" font-size="8" text-anchor="middle" fill="#F1F5F7" dominant-baseline="middle" font-family="Roboto, sans-serif">{}</text>)",
            (sx+(ex/2)), (sy+(ey/2)), (1 == m.busy ? "Ocupada" : "Livre")
        );
        svg += "\n";
    }

    svg += "</svg>\n";

    ofs << svg;
}





#if 0
    svg += "<svg  width=\""+wid+"\" height=\""+hgt+"\" viewBox=\"0 0 "+wid+" "+hgt+"\" xmlns=\"http://www.w3.org/2000/svg\">\n";
    svg += "<rect width=\""+wid+"\" height=\""+hgt+"\" fill=\"#222\" />\n";
    svg += "  <line x1=\"0\" y1=\""+ hfh +"\" x2=\""+ wid +"\" y2=\""+ hfh +"\" stroke=\"#fff\" stroke-width=\"3\" stroke-dasharray=\"10,10\"/>\n";

    svg += "  <rect x=\""+ ssx +"\"  y=\""+ ssy +"\"  width=\""+ sex +"\" height=\""+ sey +"\" fill=\""+ color +"\"/>\n";
    std::fprintf(stderr, "%d %d x %d %d\n", sx, sy, ex, ey);
    std::fprintf(stderr, "%d %d x %d %d\n", m.area.x, m.area.y, m.area.w, m.area.h);
    std::fprintf(stderr, "%s %s x %s %s\n\n", ssx.c_str(), ssy.c_str(), sex.c_str(), sey.c_str() );

    svg += "  <rect x=\"50\"  y=\"80\"  width=\"100\" height=\"80\" fill=\"#A42F3E\"/>\n";
    svg += "  <rect x=\"200\" y=\"80\"  width=\"100\" height=\"80\" fill=\"#A42F3E\"/>\n";
    svg += "  <rect x=\"350\" y=\"80\"  width=\"100\" height=\"80\" fill=\"#A42F3E\"/>\n";
    svg += "  <rect x=\"500\" y=\"80\"  width=\"100\" height=\"80\" fill=\"#A42F3E\"/>\n";

    svg += "  <rect x=\"50\"  y=\"330\" width=\"100\" height=\"80\" fill=\"#34A359\"/>\n";
    svg += "  <rect x=\"200\" y=\"330\" width=\"100\" height=\"80\" fill=\"#34A359\"/>\n";
    svg += "  <rect x=\"350\" y=\"330\" width=\"100\" height=\"80\" fill=\"#34A359\"/>\n";
    svg += "  <rect x=\"500\" y=\"330\" width=\"100\" height=\"80\" fill=\"#34A359\"/>\n";

    svg += "  <text x=\"100\" y=\"120\" font-size=\"16\" text-anchor=\"middle\" fill=\"#F1F5F7\" dominant-baseline=\"middle\" font-family=\"Roboto, sans-serif\">Ocupada</text>\n";
    svg += "  <text x=\"250\" y=\"120\" font-size=\"16\" text-anchor=\"middle\" fill=\"#F1F5F7\" dominant-baseline=\"middle\" font-family=\"Roboto, sans-serif\">Ocupada</text>\n";
    svg += "  <text x=\"400\" y=\"120\" font-size=\"16\" text-anchor=\"middle\" fill=\"#F1F5F7\" dominant-baseline=\"middle\" font-family=\"Roboto, sans-serif\">Ocupada</text>\n";
    svg += "  <text x=\"550\" y=\"120\" font-size=\"16\" text-anchor=\"middle\" fill=\"#F1F5F7\" dominant-baseline=\"middle\" font-family=\"Roboto, sans-serif\">Ocupada</text>\n";

    svg += "  <text x=\"100\" y=\"370\" font-size=\"16\" text-anchor=\"middle\" fill=\"#313537\" dominant-baseline=\"middle\" font-family=\"Roboto, sans-serif\">Livre</text>\n";
    svg += "  <text x=\"250\" y=\"370\" font-size=\"16\" text-anchor=\"middle\" fill=\"#313537\" dominant-baseline=\"middle\" font-family=\"Roboto, sans-serif\">Livre</text>\n";
    svg += "  <text x=\"400\" y=\"370\" font-size=\"16\" text-anchor=\"middle\" fill=\"#313537\" dominant-baseline=\"middle\" font-family=\"Roboto, sans-serif\">Livre</text>\n";
    svg += "  <text x=\"550\" y=\"370\" font-size=\"16\" text-anchor=\"middle\" fill=\"#313537\" dominant-baseline=\"middle\" font-family=\"Roboto, sans-serif\">Livre</text>\n";

    std::fprintf(stderr, "svg size: %ld\n", svg.size() );
    const u32 count = u32( core::block.size() );
    std::string json;
    json.reserve(512); // preallocate a bit
    json += "{\n";
    json += "  \"count\": " + std::to_string(count) + ",\n";
    json += "  \"block\": [\n";
    u32 i = 0;
    for (const auto& data : core::block) {
        json += "    {\n";
        json += "      \"used\": " + std::string(data.busy ? "true" : "false") + ",\n";
        json += "      \"posx\": " + std::to_string(data.area.x) + ",\n";
        json += "      \"posy\": " + std::to_string(data.area.y) + ",\n";
        json += "      \"posw\": " + std::to_string(data.area.w) + ",\n";
        json += "      \"posh\": " + std::to_string(data.area.h) + "\n";
        json += "    }";
        if (i < count - 1)  json += ",";
        json += "\n";
        ++i;
    }
    json += "  ]\n";
    json += "}\n";
    std::fprintf(stdout, "%s", json.c_str());

#endif
