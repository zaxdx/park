#include <chrono>
#include <thread>

#include "xorg.hpp"
#include "view.hpp"
#include "srvr.hpp"

int main (void) noexcept
{
    const zx::su32 size {640, 480};
    const zx::u32  fps  {16};

    zx::wx::init(size);
    zx::vw::init(size);
    zx::sv::init(    );

    while ( zx::wx::exec() ) {

        //const auto t_init = std::chrono::high_resolution_clock::now();

        if ( zx::wx::done() ) {
            zx::vw::size( zx::wx::size() );
        }

        zx::vw::exec();
        zx::wx::push( zx::vw::data() );
        zx::sv::proc();

        // create a communication protocol:
        // server store parking lots
        // view respond to network update calls

        if ( zx::sv::comm() ) {
            zx::sv::push( zx::vw::lots() );
          //zx::vw::read( zx::sv::info() );
        }

         zx::wx::swap();

        std::this_thread::sleep_for(std::chrono::milliseconds(fps));

        //const auto t_stop = std::chrono::high_resolution_clock::now();
        //const auto t_time = std::chrono::duration<double, std::milli>(t_stop - t_init).count();
        //std::fprintf(stderr,"\e[00;31mruntime: %4.4f ms\e[00m;\n", t_time);
    }

    zx::sv::stop();
    zx::vw::stop();
    zx::wx::stop();

    return 0;
}
