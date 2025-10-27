#include <chrono>
#include <thread>

#include "view.hpp"
#include "srvr.hpp"

int main (void) noexcept
{
    const zx::su32 size {640, 480};
    const zx::u32  fps  {16};

    zx::vw::init(size);
    zx::sv::init(    );

    bool running = true;

    while ( running )
    {
        zx::vw::exec();
        zx::sv::proc();

        if ( zx::sv::comm() ) {

            switch ( zx::sv::info().command ) {

                case zx::sv::scmd::QUIT : {
                    running = false;
                } break;

                case zx::sv::scmd::UPDATE : {
                    zx::vw::update();
                } break;

                case zx::sv::scmd::CHECK   : {
                    zx::vw::check();
                } break;

                default: break;
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(fps));
    }

    zx::sv::stop();
    zx::vw::stop();

    return 0;
}
