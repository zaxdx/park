#ifndef __ZX_SOCKET_SERVER_HPP__
#define __ZX_SOCKET_SERVER_HPP__ 1

#include <vector>
#include "defs.hpp"

namespace zx::sv {

    enum struct scmd : u08 {
        REMAP  = 0,
        UPDATE = 1,
        GET    = 2,
        QUIT   = 3
    };

    struct cmds final {
        bool update  {false};
        scmd command {};
    };

    void init (void) noexcept ;
    void stop (void) noexcept ;
    void proc (void) noexcept ;

    bool comm (void) noexcept ;
    void push (const std::vector<zx::match>&) noexcept ;
    cmds info (void) noexcept ;
}

#endif

