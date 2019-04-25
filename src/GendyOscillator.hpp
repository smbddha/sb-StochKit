/*
 * GendyOscillator.hpp
 * Samuel Laing - 2019
 *
 * Implementation of a singular generator using granular stochastic
 * dynamic synthesis
 */

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
    bool is_fm_on = true; 

    int num_bpts = 12;
    int min_freq = 30; 
    int max_freq = 1000;

    float amps[MAX_BPTS] = {0.f};
    float durs[MAX_BPTS] = {1.f};
    float offs[MAX_BPTS] = {0.f};

    int index = 0;
    float amp = 0.0; 
    float amp_next = amps[0];
    
    float max_amp_step = 0.05;
    float max_dur_step = 0.05;
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

    Wavetable sample = Wavetable(SIN);
    Wavetable env = Wavetable(TRI); 

    float amp_out = 0.f;

    // for fm synthesis in grain
    float f_mod = 400.f;
    float f_car = 800.f;
    
    // need these to keep track of modulated carrier frequency for either
    // grain in the synthesis
    float f_car1 = f_car;
    float f_car2 = f_car;

    // fm modulation index
    float i_mod = 100.f;

    float phase_mod1 = 0.f;
    float phase_mod2 = 0.f;
    
    float phase_car1 = 0.f;
    float phase_car2 = 0.f;

    // only true when just reached last break point
    bool last_flag = false;

    int count = 0;

    void process(float deltaTime) {
      last_flag = false;
      if (phase >= 1.0) {
        
        //debug("-- PHASE: %f ; G_IDX: %f ; G_IDX_NEXT: %f", phase, g_idx, g_idx_next);
        phase -= 1.0;

        amp = amp_next;
        index = (index + 1) % num_bpts;
       
        last_flag = index == num_bpts - 1;

        /* adjust vals */
        amps[index] = wrap(amps[index] + (max_amp_step * randomNormal()), -1.0f, 1.0f); 
        durs[index] = wrap(durs[index] + (max_dur_step * randomNormal()), 0.5f, 1.5f);
     
        amp_next = amps[index];
        rate = durs[index];

        /* step/adjust grain sample offsets */
        off = off_next;
        off_next = offs[index];
    
        g_idx = g_idx_next;
        g_idx_next = 0.0;

        speed = ((max_freq - min_freq) * rate + min_freq) * deltaTime * num_bpts; 
        speed *= freq_mul;
      }
     
      if (!is_fm_on) {
       
        // perform addition of grain to the generated amplitudes
        // TODO
        // maybe envs need a corresponding amplitude as well 
        // could be controllable for some more audible effect
        
        g_amp = amp + (env.get(g_idx) * sample.get(off));
        //g_amp = amp + (env.get(g_idx) * sample.get(phase_car1));
        g_amp_next = amp_next + (env.get(g_idx_next) * sample.get(off_next));
        //g_amp_next = amp_next + (env.get(g_idx_next) * sample.get(phase_car2));

        // linear interpolation
        amp_out = ((1.0 - phase) * g_amp) + (phase * g_amp_next); 
      } else {
        //amp_out = ((1.0 - phase) * amp) + (phase * amp_next); 
        g_amp = amp + (env.get(g_idx) * sinf(phase_car1));
        g_amp_next = amp_next + (env.get(g_idx_next) * sinf(phase_car2));
        amp_out = ((1.0 - phase) * g_amp) + (phase * g_amp_next); 
      }

      // advance the grain envelope indices
      g_idx = fmod(g_idx + (speed/2), 1.f);
      g_idx_next = fmod(g_idx_next + (speed/2), 1.f);

      // advance sample indices
      // TODO
      //  -> could maybe just bundle with the envelope indices ??
      //  -> MAKE CONTROLLABLE 
      off = fmod(off + (g_rate * 1e-1 * deltaTime), 1.f);
      off_next = fmod(off_next + (g_rate * 1e-1 * deltaTime), 1.f);
      
      phase += speed;

      // step phases and frequencies for fm in grans
      phase_car1 += deltaTime * f_car1;
      phase_car2 += deltaTime * f_car2;

      phase_car1 = fmod(phase_car1, 1.f);
      phase_car2 = fmod(phase_car2, 1.f);

      phase_mod1 += deltaTime * f_mod;
      phase_mod2 += deltaTime * f_mod;

      phase_mod1 = fmod(phase_mod1, 1.f);
      phase_mod2 = fmod(phase_mod2, 1.f);

      f_car1 = fmod(f_car + (i_mod * sample.get(phase_mod1)), 22050.f);
      f_car2 = fmod(f_car + (i_mod * sample.get(phase_mod2)), 22050.f);
    
      if (count >30) {
        //debug("f car: %f; f mod: %f, phase_car: %f, phase_mod: %f\n", f_car, f_mod, phase_car1, phase_mod1);
        count = 30;
      }
      count++;
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
