#ifndef __ZX_NORMALIZED_CROSS_CORRELATION_TEMPLATE_MATCH_HPP__
#define __ZX_NORMALIZED_CROSS_CORRELATION_TEMPLATE_MATCH_HPP__ 1

#include "defs.hpp"
#include <vector>

namespace zx::tm
{
    [[maybe_unused]]
    const u08 kernel[2][25] = {
    { 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0xFF, 0xFF, 0xFF, 0xFF,
      0x00, 0xFF, 0x00, 0x00, 0x00,
      0x00, 0xFF, 0x00, 0x00, 0x00,
      0x00, 0xFF, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x00, 0xFF, 0x00,
      0x00, 0x00, 0x00, 0xFF, 0x00,
      0x00, 0x00, 0x00, 0xFF, 0x00,
      0xFF, 0xFF, 0xFF, 0xFF, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00 },
    };

    void init (const su32,  const su32) noexcept ;
    void proc (const graph&, const f32) noexcept ;
    void stop (void) noexcept ;
    void load (void) noexcept ;
    void site (void) noexcept ;

    const zx::graph&              glyph   (const u32) noexcept ;
    const std::vector<zx::match>& matches (void)      noexcept ;
    const std::vector<zx::match>& lots    (void)      noexcept ;
}

#endif
