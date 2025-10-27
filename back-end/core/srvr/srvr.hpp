#ifndef __ZX_SOCKET_SERVER_HPP__
#define __ZX_SOCKET_SERVER_HPP__ 1

#include <vector>
#include <string>
#include "defs.hpp"

namespace zx::sv {

    enum struct scmd : u08 {
        REMAP  = 0,
        UPDATE = 1,
        GET    = 2,
        LOCK   = 3,
        QUIT   = 4,
        ERROR  = 5,
        CLIENT = 6,
        CHECK  = 7,
        NONE   = 8
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

