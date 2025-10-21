
#include <asio.hpp>
#include "srvr.hpp"

namespace core {

    using asio::ip::tcp;

    static asio::io_context  context  {};
    static tcp::acceptor*    acceptor {nullptr};
    static tcp::socket*      socket   {nullptr};
    static std::thread       thread   {};
    static std::atomic<bool> running  {false};
    static asio::streambuf   buffer   {};
    static zx::sv::cmds      command  {};
}

namespace local {

static void        start_accept();
static void        start_read();
static std::string handle_get()    {   return "STATE: OK \n";  }
static std::string handle_update() {   return "UPDATED OK \n"; }
static std::string handle_quit()   {   return "Goodbye!\n";    }

static std::string
trim(const std::string& s) {
    auto start = s.find_first_not_of(" \r\n\t");
    auto end   = s.find_last_not_of(" \r\n\t");
    if (start == std::string::npos) return "";
    return s.substr(start, end - start + 1);
}

static void
command (const std::string& cmd) {

    std::string response {};

    if (cmd == "GET") {
        response = handle_get();
    } else if (cmd == "UPDATE") {
        response = handle_update();
    } else if (cmd == "QUIT") {
        response = handle_quit();
        asio::async_write(*core::socket, asio::buffer(response), [](const asio::error_code&, std::size_t) {});
        std::fprintf(stderr,"[server] client quit\n");
        core::socket->close();
        delete core::socket;
        core::socket = nullptr;
        start_accept();
        return;
    } else {
        response = "Unknown command\n";
    }

    asio::async_write(*core::socket, asio::buffer(response),
                      [](const asio::error_code& ec, std::size_t) {
                          if (ec) {
                              std::fprintf(stderr,"[server] write error: %s\n", ec.message().c_str());
                          }
                      });
}

static void
on_read (const asio::error_code& ec, std::size_t bytes) {

    (void) bytes;

    if (!ec) {
        std::istream is(&core::buffer);
        std::string line;
        std::getline(is, line);

        line = trim(line);
        std::fprintf(stderr,"[server] received: %s\n",line.c_str());

        if (!line.empty()) {
            command(line);
        }

        if (core::socket && core::socket->is_open()) {
            start_read();
        }
    } else {
        std::fprintf(stderr,"[server] read error: %s\n",ec.message().c_str());
        if (core::socket) {
            core::socket->close();
            delete core::socket;
            core::socket = nullptr;
        }
        start_accept();
    }
}

static void
start_read (void) {
    asio::async_read_until(*core::socket, core::buffer, '\n',
                           [](const asio::error_code& ec, std::size_t bytes) {
                               on_read(ec, bytes);
                           });
}

static void
on_accept (const asio::error_code& ec, core::tcp::socket socket) {
    if (!ec) {
        std::fprintf(stderr,"[server] client connected\n");
        core::socket = new core::tcp::socket(std::move(socket));
        start_read();
    } else {
        std::fprintf(stderr,"[server] accept error: %s\n", ec.message().c_str());
        start_accept();
    }
}

static void
start_accept (void) {
    core::acceptor->async_accept(
        [](const asio::error_code& ec, core::tcp::socket socket) {
            on_accept(ec, std::move(socket));
        });
}

static void
start_server (unsigned short port) {
    if (core::running) return;

    core::running  = true;
    core::acceptor = new core::tcp::acceptor(core::context, core::tcp::endpoint(core::tcp::v4(), port));

    start_accept();

    core::thread = std::thread([] { core::context.run(); });

    std::fprintf(stderr,"[server] started on port: %d \n", port );
}

static void
stop_server (void) {
    if (!core::running) return;

    core::running = false;

    // if (core::socket && core::socket->is_open()) {
    //     core::socket->close();
    //     delete core::socket;
    //     core::socket = nullptr;
    // }

    if (nullptr != core::acceptor) {
        core::acceptor->cancel();
        core::acceptor->close();

        if (nullptr != core::socket) {
            core::socket->cancel();
            core::socket->close();

            delete core::socket;
            core::socket = nullptr;
        }

        delete core::acceptor;
        core::acceptor = nullptr;
    }

    core::context.stop();

    if (core::thread.joinable())
        core::thread.join();

    std::fprintf(stderr,"[server] stopped\n");
}

}

void
zx::sv::proc(void) noexcept {

}

void
zx::sv::init(void) noexcept {
    core::command.update = false;
    local::start_server(12345);
}

void
zx::sv::stop(void) noexcept {
    core::command.update = false;
    local::stop_server();
}

zx::sv::cmds
zx::sv::info (void) noexcept {
    return core::command;
}

bool
zx::sv::comm (void) noexcept {
    return core::command.update;
}

void
zx::sv::push (const std::vector<zx::match>&) noexcept {
}
