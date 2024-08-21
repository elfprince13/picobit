#pragma once

#ifndef _cplusplus
#error "C++-only header"
#else
#include <cassert>
#include <cstdlib>
#include <cstdint>
uint8_t fetchMenu(const char* title, uint8_t nOptions, const char** options);

template<typename StringType, uint8_t N>
uint8_t fetchMenu(const char* const title, const StringType (&options)[N], uint8_t filled) {
    const char* options_[N];
    for(uint8_t i = 0; i < N; ++i) {
        options_[i] = options[i];
    }
    assert(filled <= N);
    return fetchMenu(title, filled, options_);
}
#endif