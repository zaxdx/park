#ifndef __ZX_COMPUTER_VISION_HPP__
#define __ZX_COMPUTER_VISION_HPP__ 1

#include <vector>
#include "defs.hpp"

namespace zx::vw {

    void init (const su32) noexcept;
    void size (const su32) noexcept;

    void stop (void) noexcept;
    void exec (void) noexcept;

    void norm (const graph&)               noexcept;
    void copy (const graph&, const graph&) noexcept;
    f32  diff (const graph&, const graph&) noexcept;
    f32  diff (const graph&, const graph&, const ru32&) noexcept;
    void fill (const graph&, const ru32&,  const u08  ) noexcept;
    void rect (const zx::graph&, const ru32&, const u08) noexcept ;
    void quad (const zx::graph&, const ru32&, const u08) noexcept ;

    void remap  (void) noexcept;
    void update (void) noexcept;
    void check  (void) noexcept;

    void print  (void) noexcept;

    const std::vector<zx::match>& lots    (void)      noexcept ;

    const zx::graph& data (void) noexcept;

}

#endif
