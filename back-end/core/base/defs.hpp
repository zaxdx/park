#pragma once
#ifndef __ZX_UI_DEFINES_HPP__
#define __ZX_UI_DEFINES_HPP__ 1

#include <cstdint>

namespace zx
{
    using i08 = std::int8_t;
    using u08 = std::uint8_t;
    using i16 = std::int16_t;
    using u16 = std::uint16_t;
    using i32 = std::int32_t;
    using u32 = std::uint32_t;
    using i64 = std::int64_t;
    using u64 = std::uint64_t;
    using f32 = float;
    using f64 = double;

    struct ru32 final { zx::u32 x{0}, y{0}, w{0}, h{0}; };
    struct su32 final { zx::u32 w{0}, h{0}; };
    struct pu32 final { zx::u32 x{0}, y{0}; };

    struct graph final {
        u32  width  {0};
        u32  height {0};
        u32  size   {0};
        f32  mean   {0}; // mean value
        f32  vari   {0}; // std  variation
        f32  stdv   {0}; // std  deviation
        u08 *data   {nullptr};
    };

    struct match final {
        u32  mask  {};
        f32  score {};
        u32  busy  {};
        ru32 area  {};
    };

    struct frame final {
        u32  width  {0};
        u32  height {0};
        u32  stride {0}; // rgba stride
        u32  size   {0}; // rgba size
        u08 *data   {nullptr};
    };

    enum struct state : u08 {
        NONE   = 0,
        CHECK  = 1,
        UPDATE = 2,
        REMAP  = 3,
        READY  = 4
    };
}

#endif
