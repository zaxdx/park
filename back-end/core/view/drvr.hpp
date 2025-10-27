#ifndef __ZX_V4L2_DRIVER_HPP__
#define __ZX_V4L2_DRIVER_HPP__ 1

#include <cstddef>
#include "defs.hpp"

namespace zx::wc
{
    using buffer = struct buffer_s final {
        void        *data {nullptr};
        std::size_t  size {0};
    };

    bool cam_init_impl (const char*) noexcept ;  // low level kernel memory and drive acces
    bool cam_stop_impl (void) noexcept ;         // memory and kernel resources release
    bool cam_read_impl (void) noexcept ;         // memory acces sync - read-only

    const frame& cam_data_impl (void) noexcept ; // ready rgba image
    const graph& cam_gray_impl (void) noexcept ; // ready gray image
    const su32   cam_info_impl (void) noexcept ; // framebuffer info
}

#endif
