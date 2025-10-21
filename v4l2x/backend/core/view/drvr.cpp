#include <fcntl.h>
#include <linux/videodev2.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cassert>
#include <cstring>
#include <string>
#include "defs.hpp"
#include "drvr.hpp"

namespace core {
    static zx::i32         fd      {};
    static zx::u32         bpline  {};
    static zx::u32         nbuffer {};
    static zx::wc::buffer *buffers {};
    static zx::graph       graph   {}; // GRAYSCALE
    static zx::frame       frame   {}; // YUYVIMAGE
}

///////////////////////////////////////
//// INTERNAL IOCTL - WAIT FOR IRQ ////
///////////////////////////////////////
static int xioctl(int fh, unsigned long int request, void *arg) noexcept {
    int r = ioctl(fh, request, arg);
    while (-1 == r && EINTR == errno) {
        r = ioctl(fh, request, arg);
    }
    return r;
}

///////////////////////////////////////
//// LOW LEVEL DEVICE MEMORY MAP   ////
///////////////////////////////////////
static void init_mmap (void) noexcept {

    struct v4l2_requestbuffers req {};

    std::memset(&req, 0, sizeof(req));

    req.count  = 4;
    req.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    if (-1 == xioctl(core::fd, VIDIOC_REQBUFS, &req)) {
        if (EINVAL == errno) {
            std::fprintf(stderr, "erro: camera nao suporta mapeamento de memoria\n");
        } else {
            std::fprintf(stderr, "erro: camera nao suporta stream de video\n");
        }
    }

    if (req.count < 2) {
        std::fprintf(stderr, "erro: buffer de memoria insuficiente\n");
    }

    core::buffers = (zx::wc::buffer*) calloc(req.count, sizeof(zx::wc::buffer));

    if (nullptr == core::buffers) {
        std::fprintf(stderr, "erro: memoria insuficiente\n");
    }

    for (core::nbuffer = 0; core::nbuffer < req.count; ++core::nbuffer) {

        struct v4l2_buffer buf {};

        std::memset(&buf, 0, sizeof(buf));

        buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory      = V4L2_MEMORY_MMAP;
        buf.index       = core::nbuffer;

        if (-1 == xioctl(core::fd, VIDIOC_QUERYBUF, &buf))
            std::fprintf(stderr, "erro: mapeamento de buffer\n");

        core::buffers[core::nbuffer].size = buf.length;
        core::buffers[core::nbuffer].data = mmap(nullptr, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, core::fd, buf.m.offset);

        if (MAP_FAILED == core::buffers[core::nbuffer].data)
            std::fprintf(stderr, "erro: mapeamento de buffer corrompido\n");
    }
}

///////////////////////////////////////
//// SET CAPTURE FRAMES            ////
///////////////////////////////////////
static void link_buffers (void) noexcept {

    enum v4l2_buf_type type {};
    unsigned int i {0};

    for (i = 0; i < core::nbuffer; ++i) {

        struct v4l2_buffer buf {};

        std::memset(&buf, 0, sizeof(buf));

        buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index  = i;

        if (-1 == xioctl(core::fd, VIDIOC_QBUF, &buf))
            std::fprintf(stderr, "erro: falha ao copiar o buffer\n");
    }

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (-1 == xioctl(core::fd, VIDIOC_STREAMON, &type))
        std::fprintf(stderr, "erro: falha de stream\n");
}

///////////////////////////////////////
//// CONVERT YUYV:4:2:2 TO RGB     ////
///////////////////////////////////////
//#define CLIP(color) (unsigned char)(((color) > 0xFF) ? 0xff : (((color) < 0) ? 0 : (color)))
[[maybe_unused]]
static void convert(const zx::u08 *src, zx::u08 *dst, zx::u32 w, zx::u32 h, zx::u32 stride) noexcept {
    int j = 0;
    int height = int(h);
    int width  = int(w);

    auto clamp = [](const int value) {
        return (unsigned char)(((value) > 0xFF) ? 0xFF : (((value) < 0) ? 0 : (value)));
    };

    while (--height >= 0)
    {
        for (j = 0; j + 1 < width; j += 2)
        {
            const int u = src[1];
            const int v = src[3];
            const int u1 = (((u - 128) << 7) + (u - 128)) >> 6;
            const int rg = (((u - 128) << 1) + (u - 128) + ((v - 128) << 2) + ((v - 128) << 1)) >> 3;
            const int v1 = (((v - 128) << 1) + (v - 128)) >> 1;

            *dst++ = clamp(src[0] + v1);
            *dst++ = clamp(src[0] - rg);
            *dst++ = clamp(src[0] + u1);
            *dst++ = 255;
            *dst++ = clamp(src[2] + v1);
            *dst++ = clamp(src[2] - rg);
            *dst++ = clamp(src[2] + u1);
            *dst++ = 255;

            src += 4;
        }
        src += stride - (w * 2);
    }
}

///////////////////////////////////////
//// CONVERT YUYV:4:2:2 TO GRAY    ////
///////////////////////////////////////
static void convert(const zx::u08* src, const zx::graph& graph) noexcept {

    zx::i32 j = 0;

    zx::i32 height = zx::i32(graph.height);
    zx::i32 width  = zx::i32(graph.width);
    zx::i32 stride = zx::i32(core::bpline);

    zx::u08 *dst = graph.data;

    while (--height >= 0)
    {
        for (j = 0; j + 1 < width; j += 2)
        {
            *dst++ = zx::u08(src[0]); //// YUYV4:2>2 [Y][U][Y][V]
            *dst++ = zx::u08(src[2]); //// GRAYSCALE [Y] x [Y] x
             src  += 4;
        }
        src += stride - (width * 2);
    }
}

bool
zx::wc::cam_init_impl (const char* device) noexcept {

    struct stat            st   {}; // DEVICE STATUS
    struct v4l2_capability cap  {}; // DEVICE CAPABILITIES
    struct v4l2_cropcap    ccap {}; // CROP CAPABILITIES
    struct v4l2_crop       crop {}; // CROP STATUS
    struct v4l2_format     fmt  {}; // VIDEO FORMAT

    std::memset(&ccap, 0, sizeof(ccap));
    std::memset(&fmt,  0, sizeof(fmt));

    if (-1 == stat(device, &st)) {
        std::fprintf(stderr, "%s: nao identificada %s:%s\n", device, std::to_string(errno).c_str(), strerror(errno));
    }

    if (!S_ISCHR(st.st_mode)) {
        std::fprintf(stderr, "%s: webcam nao encontrada %s:%s\n", device, std::to_string(errno).c_str(), strerror(errno));
    }

    core::fd = open(device, O_RDWR | O_NONBLOCK, 0);

    if (-1 == core::fd) {
        std::fprintf(stderr, "%s: conexao negada %s:%s\n", device, std::to_string(errno).c_str(), strerror(errno));
    }

    if (-1 == xioctl(core::fd, VIDIOC_QUERYCAP, &cap)) {
        if (EINVAL == errno) {
            std::fprintf(stderr, "erro: v4l2 nao suportado\n");
        } else {
            std::fprintf(stderr, "erro: video query nao suportado\n");
        }
    }

    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        std::fprintf(stderr, "erro: captura de video nao suportada\n");
    }

    if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
        std::fprintf(stderr, "erro: captura nao suporta stream\n");
    }

    ccap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (0 == xioctl(core::fd, VIDIOC_CROPCAP, &ccap)) {
        crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        crop.c = ccap.defrect; /* reset to default */
        if (-1 == xioctl(core::fd, VIDIOC_S_CROP, &crop)) {
            switch (errno) {
                case EINVAL:
                    std::fprintf(stderr, "info: crop nao suportado\n");
                    break;
                default: break;
            }
        }
    }

    //// FORMATO DE VIDEO/IMAGEM - REQUEST PACKET
    fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width       = 640; // VALOR MAXIMOS DO DISPOSITIVO TESTADO (WIDTH)
    fmt.fmt.pix.height      = 480; // VALOR MAXIMOS DO DISPOSITIVO TESTADO (HEIGHT)
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    fmt.fmt.pix.field       = V4L2_FIELD_INTERLACED;

    if (-1 == xioctl(core::fd, VIDIOC_S_FMT, &fmt))
        std::fprintf(stderr, "erro: formato de video nao suportado (640x480 :INTERLACED: YUYV 4:2:2)\n");

    if (fmt.fmt.pix.pixelformat != V4L2_PIX_FMT_YUYV)
        std::fprintf(stderr, "erro: formato de imagem nao suportado (640x480 :INTERLACED: YUYV 4:2:2)\n");

    ////       0  X  1  Y
    //// YUYV [Y][U][Y][V] 2bpp
    core::bpline = fmt.fmt.pix.bytesperline;

    //// FORMATO DE VIDEO/IMAGEM - RESPONSE
    //// RGB FRAME
    core::frame.width  = fmt.fmt.pix.width;
    core::frame.height = fmt.fmt.pix.height;
    core::frame.stride = core::frame.width * 4;
    core::frame.size   = core::frame.width * core::frame.height * 4; /// rgba:4
    core::frame.data   = new zx::u08[core::frame.size];
    //// GRAY GRAPH
    core::graph.width  = fmt.fmt.pix.width;
    core::graph.height = fmt.fmt.pix.height;
    core::graph.size   = core::graph.width * core::graph.height;
    core::graph.data   = new zx::u08[core::graph.size];

    init_mmap();

    link_buffers();

    return true;
}

bool
zx::wc::cam_stop_impl (void) noexcept {

    enum v4l2_buf_type type {};
    unsigned int i {0};

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (-1 == xioctl(core::fd, VIDIOC_STREAMOFF, &type))
        std::fprintf(stderr, "erro: falha ao parar captura\n");

    for (i = 0; i < core::nbuffer; ++i)
        if (-1 == munmap(core::buffers[i].data, core::buffers[i].size))
            std::fprintf(stderr, "erro: falha ao desmapear memoria da camera\n");

    for (i = 0; i < core::nbuffer; ++i)
        core::buffers[i].data = nullptr;

    free(core::buffers);

    core::buffers = nullptr;

    if (-1 == close(core::fd))
        std::fprintf(stderr, "erro: falha ao fechar driver\n");

    core::fd = -1;

    delete [] core::frame.data;
    delete [] core::graph.data;

    core::frame.data = nullptr;
    core::graph.data = nullptr;

    return true;
}

bool
zx::wc::cam_read_impl (void) noexcept {

    struct v4l2_buffer buf {};

    std::memset(&buf, 0, sizeof(buf));

    buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;

    //// queue this buffer
    if (-1 == xioctl(core::fd, VIDIOC_DQBUF, &buf)) {
        switch (errno) {
            case EAGAIN:
                return true;
            case EIO:
                default:
                    std::fprintf(stderr, "erro: falha ao copiar o buffer\n");
        }
    }

    assert(buf.index < core::nbuffer);

    //convert((zx::u08 *) core::buffers[buf.index].data, core::frame.data, core::frame.width, core::frame.height, core::frame.length);
    convert((zx::u08 *) core::buffers[buf.index].data, core::graph);

    //// queue next buffer
    if (-1 == xioctl(core::fd, VIDIOC_QBUF, &buf))
        std::fprintf(stderr, "erro: falha ao copiar o buffer\n");

    return false;
}

const zx::frame&
zx::wc::cam_data_impl (void) noexcept {
    return core::frame;
}

const zx::graph&
zx::wc::cam_gray_impl (void) noexcept {
    return core::graph;
}

const zx::su32
zx::wc::cam_info_impl (void) noexcept {
    return { core::frame.width, core::frame.height };
}

#if 0

const zx::u08 r1 = CLIP(src[0] + v1);
const zx::u08 g1 = CLIP(src[0] - rg);
const zx::u08 b1 = CLIP(src[0] + u1);
const zx::u08 r2 = CLIP(src[2] + v1);
const zx::u08 g2 = CLIP(src[2] - rg);
const zx::u08 b2 = CLIP(src[2] + u1);

if (r1 > b1 and r1 > g1) {
    *dst++ = r1; *dst++ = g1; *dst++ = b1; *dst++ = 255;
} else {
    *dst++ =  0; *dst++ =  0; *dst++ =  0; *dst++ = 255;
}

if (r2 > b2 and r2 > g2) {
    *dst++ = r2; *dst++ = g2; *dst++ = b2; *dst++ = 255;
} else {
    *dst++ =  0; *dst++ =  0; *dst++ =  0; *dst++ = 255;
}

if ((g-diff) > b and (g-diff) > r) {
    *dst++ = r; *dst++ = g; *dst++ = b; *dst++ = 255;
} else {
    *dst++ = 0; *dst++ = 0; *dst++ = 0; *dst++ = 255;
}

*dst++ = CLIP(src[0] + v1);
*dst++ = CLIP(src[0] - rg);
*dst++ = CLIP(src[0] + u1);
*dst++ = 255;
*dst++ = CLIP(src[2] + v1);
*dst++ = CLIP(src[2] - rg);
*dst++ = CLIP(src[2] + u1);
*dst++ = 255;

#endif
