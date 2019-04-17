#ifndef __GENDYOSC_HPP__
#define __GENDYOSC_HPP__

#include "util/common.hpp"
#include "dsp/digital.hpp"

#include "wavetable.hpp"

#define MAX_BPTS 50

namespace rack {
  struct GendyOscillator {
    float phase = 1.f;
    
    bool GRAN_ON = true;
      
    int num_bpts = 12;
    int min_freq = 30; 
    int max_freq = 1000;

    float mAmps[MAX_BPTS] = {0.f};
    float mDurs[MAX_BPTS] = {0.f};
    float mOffs[MAX_BPTS] = {0.f};

    int index = 0;
    float amp = 0.0; 
    float amp_next = mAmps[0];
    
    float max_amp_step = 0.05;
    float max_dur_step = 0.0;
    float speed = 0.0;
    float rate = 0.0;

    float freq_mul = 1.0;

    // vars for grain offsets
    float off = 0.0;
    float off_next = 0.0;

    float g_idx = 0.f;
    float g_idx_next = 0.5f;

    float g_amp = 0.f;
    float g_amp_next = 0.f;
    float g_rate = 1.f;

    Wavetable sample;
    Wavetable env = Wavetable(0); 
    Wavetable env_next = Wavetable(0);

    float amp_out = 0.f;

    // only true when just reached last break point
    bool last_flag = false;

    void process(float deltaTime) {
      last_flag = false;
      if (phase >= 1.0) {
        
        //debug("-- PHASE: %f ; G_IDX: %f ; G_IDX_NEXT: %f", phase, g_idx, g_idx_next);
        phase -= 1.0;

        amp = amp_next;
        index = (index + 1) % num_bpts;
       
        last_flag = index == num_bpts - 1;

        /* adjust vals */
        mAmps[index] = wrap(mAmps[index] + (max_amp_step * randomNormal()), -1.0f, 1.0f); 
        mDurs[index] = mDurs[index] + (max_dur_step * randomNormal());
     
        amp_next = mAmps[index];
        rate = mDurs[index];

        /* step/adjust grain sample offsets */
        off = off_next;
        off_next = mOffs[index];
    
        g_idx = g_idx_next;
        g_idx_next = 0.0;

        speed = ((max_freq - min_freq) * rate + min_freq) * deltaTime * num_bpts; 
        speed *= freq_mul;
      }
     
      if (GRAN_ON) {
        //printf("SAMPLE: %f, off: %f\n", sample.get(off), off);
        g_amp = amp + (env.get(g_idx) * sample.get(off));
        g_amp_next = amp_next + (env_next.get(g_idx_next) * sample.get(off_next));

        // linear interpolation
        amp_out = ((1.0 - phase) * g_amp) + (phase * g_amp_next); 
      } else {
        amp_out = ((1.0 - phase) * amp) + (phase * amp_next); 
      }

      // advance the grain envelope indices
      g_idx = fmod(g_idx + (speed/2), 1.f);
      g_idx_next = fmod(g_idx_next + (speed/2), 1.f);

      // advance sample indices
      // TODO
      //  -> could maybe just bundle with the envelope indices ??
      //  -> MAKE CONTROLLABLE 
      off = fmod(off + (g_rate * 1e-1 * (1.f / 48000.f)), 1.f);
      off_next = fmod(off_next + (g_rate * 1e-4 * (1.f / 48000.f)), 1.f);

      //printf("new off: %f\n", off);
      
      phase += speed;
    }

    float wrap(float in, float lb, float ub) {
      float out = in;
      if (in > ub) out = ub;
      else if (in < lb) out = lb;
      return out;
    }
    
    float out() {
      return amp_out;
    }
  };

}

#endif
