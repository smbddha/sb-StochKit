#ifndef __WAVETABLE_HPP__
#define __WAVETABLE_HPP__

#include "util/common.hpp"

#define TABLE_SIZE 2048 

struct Wavetable {

  float table[TABLE_SIZE];

  Wavetable() {
    initWav(); 
  }

  Wavetable(int n) {
    switch (n) {
      case 0:
        initEnv();
      default:
        initWav();
    }
  }

  void initWav() {
    // Fill the wavetable
    float phase = 0.f;
    for (int i=0; i<TABLE_SIZE; i++) {
      table[i] = sinf(2.f*M_PI * phase); 
      phase += (float) i  / (2.f*M_PI);
    }
  }

  void initEnv() {
    float phase = 0.f;
    for (int i=0; i<TABLE_SIZE; i++) {
      // TODO
      // make env table
      if (phase < 0.5f) table[i] = (float) i / TABLE_SIZE;
      else table[i] = ((-1.f * i) / TABLE_SIZE) + 2.f;
      phase += 1.f / TABLE_SIZE;
    }
  }

  float operator[](int x) {
    return table[x];
  }

  float operator[](float x) {
    return index(x); 
  }

  float index(float x) {
    float fl = floorf(x);
    float ph = x - fl;
    float lb = table[(int) fl];
    float ub = table[(int) ceilf(x)];

    return ((1.0 - ph) * lb) + (ph * ub);
  }

  /*
   * Expects val 0.0 <= x < 1.0
   */
  float get(float x) {
    return index(x * (float) TABLE_SIZE); 
  }
};

#endif
