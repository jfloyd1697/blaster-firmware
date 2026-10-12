#include <cassert>
#include "core/lights/LightOutputCurve.h"

int main() {
    assert((LightOutputCurve::correct({0, 0, 0}) == LightColor{0, 0, 0}));
    assert((LightOutputCurve::correct({255, 255, 255}) == LightColor{94, 59, 88}));
    const auto middle = LightOutputCurve::correct({128, 128, 128});
    assert(middle[0] < 30 && middle[1] < 20 && middle[2] < 30);
    for (int x = 1; x <= 255; ++x) {
        const auto low = LightOutputCurve::correct({static_cast<unsigned char>(x - 1),
                                                     static_cast<unsigned char>(x - 1),
                                                     static_cast<unsigned char>(x - 1)});
        const auto high = LightOutputCurve::correct({static_cast<unsigned char>(x),
                                                      static_cast<unsigned char>(x),
                                                      static_cast<unsigned char>(x)});
        for (int i = 0; i < 3; ++i) assert(high[i] >= low[i]);
    }
}
