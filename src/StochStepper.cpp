#include "plugin.hpp"
#include "dsp/digital.hpp"

struct StochStepper : Module {
	enum ParamIds {
    STEP_PARAM,
    NUM_PARAMS
	};
	enum InputIds {
    STEP_INPUT, 
    NUM_INPUTS
	};
	enum OutputIds {
		STOC_OUTPUT,
    NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};

	//float phase = 1.0;
	float blinkPhase = 0.0;

  enum StepTypes {
    STEP,
    RAMP,
    EXP 
  };

  dsp::SchmittTrigger stepTrigger;

  float amp = 0.f;
  float amp_next = 0.f;
  float amp_out = 0.f;

  StepTypes t_step = STEP;

  StochStepper() {
    config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
    configParam(STEP_PARAM, 0.0f, 2.0f, 0.0f);
    // configParam(FREQ_PARAM, -1.0, 1.0, 0.0);
  }

  void process(const ProcessArgs &args) override;
  float wrap(float,float,float);
};

void StochStepper::process(const ProcessArgs &args) {
  // Implement a simple sine oscillator
  //float deltaTime = args.sampleTime;
 
  t_step = (StepTypes) params[STEP_PARAM].getValue();

  if (stepTrigger.process(inputs[STEP_INPUT].getVoltage() / 2.f)) {
    // handle the next amplitude generation here using the 
    // different probability distributions
  }
 
  switch (t_step) {
    case STEP:
      break;
    case RAMP:
      break;
    case EXP:
      break;
    default:
      break;
  }
  // continue with interpolation if not doing ramp

  outputs[STOC_OUTPUT].setVoltage(5.0f);
}

struct StochStepperWidget : ModuleWidget {
	StochStepperWidget(StochStepper *module) {
    setModule(module);
    setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/StochStepper.svg")));

    addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 6 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 6 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

    addParam(createParam<CKSSThree>(Vec(110, 240), module, StochStepper::STEP_PARAM));
		
    //addParam(createParam<RoundHugeBlackKnob>(Vec(15, 68), module, StochStepper::FREQ_PARAM));
   	//addInput(createInput<PJ301MPort>(Vec(76.210, 285.33), module, StochStepper::GRAT_INPUT));

    addOutput(createOutput<PJ301MPort>(Vec(134.003, 334.86), module, StochStepper::STOC_OUTPUT));

		//addChild(createWidget<MediumLight<RedLight>>(Vec(41, 59), module, StochStepper::BLINK_LIGHT));
	}
};

Model *modelStochStepper = createModel<StochStepper, StochStepperWidget>("StochStepper");
