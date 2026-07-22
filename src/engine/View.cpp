#include "rel/View.h"

namespace rel {

void View::setViewport(int32_t x, int32_t y, uint32_t width, uint32_t height) {
    mViewport = {x, y, width, height};
}

} // namespace rel
