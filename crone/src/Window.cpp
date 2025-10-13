//
// Created by emb on 1/25/19.
//

#include "Window.h"

using namespace crone;

constexpr size_t Window::raisedCosShortLen;

const float Window::raisedCosShort[Window::raisedCosShortLen] = {
#include "cos_win.h"
};