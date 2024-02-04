#pragma once
#include<cstdint>
//     constexpr double dd(uint32_t(0xFFFFFFFF));
// constexpr uint32_t ee(dd*0.005);
#define C_CONSTANT 0x147ae14
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define C_CONSTANT_STR TOSTRING(C_CONSTANT)
inline constexpr uint32_t c_constant = C_CONSTANT;
