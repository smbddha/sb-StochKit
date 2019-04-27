#include "wavetable.hpp"

namespace rack {
    float wrap(float in, float lb, float ub) {
      float out = in;
      if (in > ub) out = lb;
      else if (in < lb) out = ub;
      return out;
    }

    float mirror(float in, float lb, float ub) {
      float out = in;

      if (in > ub) {
        out = out - (in - ub);
      }
      else if (in < lb) {
        out = out - (in - lb);
      }
      
      return out;
    }
}
