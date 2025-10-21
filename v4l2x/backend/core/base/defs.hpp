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

    struct vi32 final { zx::i32 x{0}, y{0}, z{0}; };
    struct vu32 final { zx::u32 x{0}, y{0}, z{0}; };
    struct wi32 final { zx::i32 x{0}, y{0}, z{0}, w{0}; };
    struct wu32 final { zx::u32 x{0}, y{0}, z{0}, w{0}; };
    struct vf32 final { zx::f32 x{0}, y{0}, z{0}; };
    struct wf32 final { zx::f32 x{0}, y{0}, z{0}, w{0}; };
    struct vf64 final { zx::f64 x{0}, y{0}, z{0}; };
    struct wf64 final { zx::f64 x{0}, y{0}, z{0}, w{0}; };
    struct mat3 final { zx::f32 m[3][3] { {0,0,0},  {0,0,0},  {0,0,0}             }; };
    struct mat4 final { zx::f32 m[4][4] { {0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0} }; };

    struct vec2 final { zx::f32 x{0}, y{0}; };
    struct vec3 final { zx::f32 x{0}, y{0}, z{0}; };
    struct vec4 final { zx::f32 x{0}, y{0}, z{0}, w{0}; };
    struct vtx2 final { vec2 pos{}; vec2 uvs{}; vec4 rgb{}; };
    struct vtx3 final { vec3 pos{}; vec2 uvs{}; vec4 rgb{}; };

    struct p2dn final { u32 x{0}, y{0}; };             // point 2d normal
    struct p2da final { u32 x{0}, y{0}; f32 k{0}; };   // point 2d anti-alias

    // rectangle: int32_t :: ( x, y, w, h )
    struct ri32 final { zx::i32 x{0}, y{0}, w{0}, h{0}; };
    // size:  int32_t :: ( w, h )
    struct si32 final { zx::i32 w{0}, h{0}; };
    // point: int32_t :: ( x, y )
    struct pi32 final { zx::i32 x{0}, y{0}; };
    // rectangle: int16_t :: ( x, y, w, h )
    struct ri16 final { zx::i16 x{0}, y{0}, w{0}, h{0}; };
    // size:  int16_t :: ( w, h )
    struct si16 final { zx::i16 w{0}, h{0}; };
    // point: int16_t :: ( x, y )
    struct pi16 final { zx::i16 x{0}, y{0}; };

    // rectangle: uint32_t :: ( x, y, w, h )
    struct ru32 final { zx::u32 x{0}, y{0}, w{0}, h{0}; };
    // size:  uint32_t :: ( w, h )
    struct su32 final { zx::u32 w{0}, h{0}; };
    // point: uint32_t :: ( x, y )
    struct pu32 final { zx::u32 x{0}, y{0}; };
    // rectangle: uint16_t :: ( x, y, w, h )
    struct ru16 final { zx::u16 x{0}, y{0}, w{0}, h{0}; };
    // size:  uint16_t :: ( w, h )
    struct su16 final { zx::u16 w{0}, h{0}; };
    // point: uint16_t :: ( x, y )
    struct pu16 final { zx::u16 x{0}, y{0}; };

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
        ru32 area  {};
    };

    struct frame final {
        u32  width  {0};
        u32  height {0};
        u32  stride {0}; // rgba stride
        u32  size   {0}; // rgba size
        u08 *data   {nullptr};
    };
}

namespace zx::types
{
    enum struct direction : u08 {
        dx  = 1 << 0,
        dy  = 1 << 1,
        dxy = 1 << 2
    };
}

#endif


#if 0

 struct resvx final {
        u32  width  {};
        u32  height {};
        u32  size   {};
        f32 *data   {};
    };

    struct intvx final {
        u32  width  {};
        u32  height {};
        u32  size   {};
        int *data   {};
    };

    struct block final {
        u32 width  {};
        u32 height {};
        u32 stride {};
        u32 size   {};
        u32 reswd  {};
        u32 resht  {};
        u32 ressz  {};
        u32 resst  {};
        zx::i32 *area_1 {nullptr};
        zx::i32 *area_2 {nullptr};
        zx::f32 *result {nullptr};
    };

    struct objmap final {
        pu32 site {};
        su32 size {};
        u32  pnts {};
    };

    struct object final {
        u32 pid {0};
        u32 vao {0};
        u32 vbo {0};
        u32 ebo {0};
        u32 tex {0};
    };

namespace zx::state
{
enum class focus : u16 {
none = 0,
have = 1,
want = 2,
lost = 3
};

enum class mouse : u16 {
none  = 0,
enter = 1,
leave = 2,
move  = 3,
press = 4,
lease = 5,
click = 6
};

enum class window : u16 {
none    = 0,
ready   = 1, // no state
update  = 2,
finish  = 3, // one pass
dragin  = 4, // drag start   : drag_in  drag_on
dragon  = 5, // drag on
dragof  = 6, // drag stop    : drag_off
hidden  = 7,  // hidden
visible = 8
};

enum class action : u16 {
none   = 0, // no action
render = 1, // render image
upload = 2, // upload image
change = 3, // change focus
locate = 4  // change location
};

//// ACTION TO WINDOW MANAGER
//  bypass
//  single
//  valid
//  invalid
//  visible
}

namespace zx::type
{
enum class background : u08 {
none     = 0,
color    = 1,
gradient = 2,
image    = 3
};

enum class round : u08 {
none = 0x00,
tplf = 1 << 0,
tprt = 1 << 1,
btlf = 1 << 2,
btrt = 1 << 3,
tplr = tplf | tprt,
btlr = btlf | btrt,
tblf = tplf | btlf,
tbrt = tprt | btrt,
full = tplf | tprt | btlf | btrt
};

enum class align : u32 {
top_left      = 0,
top_right     = 1,
bottom_left   = 2,
bottom_right  = 3,
center        = 4,
center_left   = 5,
center_right  = 6,
center_top    = 7,
center_bottom = 8
};

enum class growth : u32 {
shrink = 0,
width  = 1,
height = 2,
expand = 3,
};

// xorg events, d-sync
enum class input : u32 {
none         =  0,
mouse_drag   =  1,
mouse_move   =  2,
mouse_lclick =  3,
mouse_mclick =  4,
mouse_rclick =  5,
mouse_press  =  6,
mouse_lease  =  7,
keybd_press  =  8,
keybd_lease  =  9,
scroll_up    = 10,
scroll_dw    = 11,
win_resize   = 12
};

enum class window : u08 {
none     = 0,
fixed    = 1,
floating = 2
};

enum class window_button : u08 {
none  = 0,
close = 1 << 0,
min   = 1 << 1,
max   = 1 << 2,
all   = min | max | close
};

enum class object : u32 {
none   = 0,
widget = 1,
window = 2,
layout = 3
};

enum class widget : u32 {
none      = 0,
button    = 1,  // button
textbar   = 2,  // textbar
textbox   = 3,  // textbox
vscrol    = 4,  // vertical scrollbar
viewport  = 5,  // viewport
stock     = 6,   // stock button
canvas    = 7,
string    = 8,
docker    = 9,
winbar    = 10
};

enum class layout : u16 {
none = 0,
vbox = 1,
hbox = 2,
tabs = 3
};

enum class mode : u08 {
normal   = 0,
lighter  = 1,
darker   = 2,
gradient = 3
};

enum class draw : u08 {
none  = 0,
rect  = 1,
image = 2
};
}

#endif

#if 0


    // struct image final {
    //     u32  width  {0};
    //     u32  height {0};
    //     u32  size   {0};
    //     u32  stride {0};
    //     u32 *data   {nullptr};
    // };


// namespace zx::flags
// {
//     namespace corner {
//         enum : u08 {
//             top_left     = 1 << 0,
//             top_right    = 1 << 1,
//             bottom_left  = 1 << 2,
//             bottom_right = 1 << 3,
//             rounded      = 1 << 7
//         };
//     }
//}


constexpr vtx3<float> s = { {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 0.0f, 0.0f} };

struct alignas(u32) rgba final {
u08 r{0}, g{0}, b{0}, a{0};
rgba() {}
rgba(const u08 N, const u08 A): r(N), g(N), b(N), a(A) {}
rgba(const u08 R, const u08 G, const u08 B, const u08 A): r(R), g(G), b(B), a(A) {}
rgba(const u32 A) noexcept: r((A & 0x00FF0000) >> 16), g((A & 0x0000FF00) >>  8),
b((A & 0x000000FF) >>  0),  a((A & 0xFF000000) >> 24) {}
inline u32 to_u32(void) const noexcept { return u32((a << 24) | (b << 16) | (g << 8) | r); }
};

struct point final {
i32 x{0}, y{0};
point() {}
point(const i32 X, const i32 Y): x(X), y(Y){}
//point& operator+(const i32 n) { x+=n; y+=n; return *this;}
//point operator+(const point p, const i32 n) { return { p.x+n, p.y+n}; }
bool   operator==(const point& p) { return ( x == p.x and y == p.y ); }
point& operator+=(const point& p) { x += p.x; y += p.y; return *this; }
point& operator-=(const point& p) { x -= p.x; y -= p.y; return *this; }
};
struct size final {
i32 w{0}, h{0};
size() {}
//size(const i32 N) : w(N), h(N){}
size(const i32 W, const i32 H): w(W), h(H) {}
size& operator+=(const size s) { w+=s.w; h+=s.h; return *this;}
};
struct rect final {
i32 x{0}, y{0}, w{0}, h{0};
rect() {}
rect(const i32 N) noexcept: x{N}, y{N}, w{N}, h(N) {}
rect(const i32 X, const i32 Y, const i32 W, const i32 H) noexcept: x(X), y(Y), w(W), h(H) {}
rect(const point p, const size s) noexcept: x(p.x), y(p.y), w(s.w), h(s.h) {}
//rect& operator+=(const i32 n) { x -= n; y -= n; w += n; h += n; return *this; }
//rect& operator-=(const i32 n) { x += n; y += n; w -= n; h -= n; return *this; }
inline rect absolute()  const noexcept { return { x, y, w + x, h + y }; }
inline bool hit(const point& p) const noexcept { return (p.x >= x and p.x < w and p.y >= y and p.y < h); }
};

location cell request
struct rqst final {
zx::ru32   inner {};
zx::ru32   outer {};
type::fill fill  {type::fill::shrink};
type::cell cell  {type::cell::center};
};

struct cell final {
zx::ru32   inner {};
zx::ru32   outer {};
zx::su32   innsz {};
zx::su32   outsz {};
type::fill fill  {type::fill::shrink};
type::cell cell  {type::cell::center};
};

location cell docking
struct dock final {
zx::ru32 padding { }; // distance from child  to parent [ child -> parent ]
zx::ru32 margin  { }; // distance from parent to child  [ parent -> child ]
zx::u32  spacing {0}; // [cell] spacing | spacing [cell]
zx::u32  border  {0}; // distance from outer to inner cell area
zx::u32  radius  {0}; // corner radius
zx::u32  alpha   {0}; // avoid memory padding... so...
};

widget base info
struct data final {
zx::dock  dock { };
zx::rqst  rqst { };
zx::ru32  area { };
zx::ru32  util { };
zx::pi32  spos { };
zx::su32  font { };
zx::su32  size { };
zx::u64   id   {0};
};

struct pack final {} packing info, border...
fix request, subdivide data
rethink area vs used: layouts return a bigger area ( winbar -> round ex)
rollback object, use data and look and feel

enum color : u08 {
foreground  = 0,
    background  = 1,
font_color  = 2,
font_shadow = 3,
border      = 4
};

struct style final {
rgba color[5] = {
{0xFF778899}, // foreground  {r,g,b,a}
{0xDD111317}, // background  {0xaarrggbb}
{0xFFCCBBAA}, // font conlor
{0xFF000000}, // font shadow
{0xFF313337}  // border color
};
};


inline area operator-(const area& a, const i32 da) noexcept { return { a.sx + da, a.sy + da, a.ex - da, a.ey - da }; }
inline area operator+(const area& a, const i32 da) noexcept { return { a.sx - da, a.sy - da, a.ex + da, a.ey + da }; }

style.color[zx::color::background] = 0xDD111317; // 0xARGB
style.color[zx::color::border]     = 0xFF313337;

struct ctrl final {
zx::i32 padding {0};
zx::i32 spacing {0};
zx::i32 border  {0};
zx::i32 radius  {0};
zx::i32 margin  {0};
zx::f32 alpha   {0};
type::draw  func  { type::draw::rect   }; // render rectangle/image
type::mode  mode  { type::mode::normal }; // normal, hover, click render mode
type::round round { type::round::none  }; // rectangle round corners
type::fill  fill  { type::fill::none   }; // layoutr fill type
};


struct conf final {
zx::u64     id        {0}; // usar u32 ou u16
zx::rect    area      { };
zx::rect    used      { };
zx::ftinfo  font      { };
zx::point   position  { };
zx::size    size      { };
zx::i32     padding   {0};
zx::i32     spacing   {0};
zx::i32     border    {0};
zx::i32     radius    {0};
zx::i32     margin    {0};
zx::f32     alpha     {0};
zx::rqst    request   { };
};

zx::u32     absolute  {0};
type::draw  func      { type::draw::rect   }; // render rectangle/image
type::mode  mode      { type::mode::normal }; // normal, hover, click render mode
type::round round     { type::round::none  }; // rectangle round corners
type::fill  fill      { type::fill::shrink }; // layoutr fill type

struct dragwin final {
zx::globj   object   {};
zx::point   position {};
zx::size    size     {};
type::icon  icon     {};
};

rgba(const u32 A) {
a = static_cast<u08>( ( A & 0xFF000000 ) >> 24 );
r = static_cast<u08>( ( A & 0x00FF0000 ) >> 16 );
g = static_cast<u08>( ( A & 0x0000FF00 ) >>  8 );
b = static_cast<u08>( ( A & 0x000000FF )       );
}


struct uiobj final {
type::object object {type::object::none};
type::layout layout {type::layout::none};
type::widget widget {type::widget::none};
type::window window {type::window::none};
};

#endif
