/*

#include "dsp/digital.hpp"

#include "Gendy.hpp"

struct StochasticGenerator : Module {
  enum ParamIds {
    SSWI_PARAM, 
    SPR0_PARAM,
    SPR1_PARAM,
    NUM_PARAMS 
  };
  enum InputIds {
    SPR0_INPUT,
    SPR1_INPUT,
    GATE_INPUT,
    NUM_INPUTS 
  };
  enum OutputIds {
    NUM_OUTPUTS
  };
  enum LightIds {
    NUM_LIGHTS
  };

  SchmittTrigger trigger;

  MyModule() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {};

  void step() override;

};

void StochasticGenerator::step() {

  // Gate and trigger
	bool gated = inputs[GATE_INPUT].value >= 1.0f;
	if (trigger.process(inputs[TRIG_INPUT].value))
		;

};
*/
