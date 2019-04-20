#include "Gendy.hpp"

#include "util/common.hpp"
#include "dsp/digital.hpp"

struct StochStepper : Module {
	enum ParamIds {
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
    LINEAR,
    EXP,
    CUBIC
  };

  SchmittTrigger stepTrigger;

  float amp = 0.f;
  float amp_next = 0.f;
  float amp_out = 0.f;

  StepTypes t_step = STEP;

  StochStepper() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {
  }
	
  void step() override;
  float wrap(float,float,float);
};

void StochStepper::step() {
	// Implement a simple sine oscillator
  //float deltaTime = engineGetSampleTime();
  
  if (stepTrigger.process(inputs[STEP_INPUT].value / 2.f)) {
    switch (t_step) {
      case STEP:
        break;
      case RAMP:
        break;
      case LINEAR:
        break;
      case EXP:
        break;
      case CUBIC:
        break;
      default:
        break;
    } 
  }
 
  // continue with interpolation if not doing ramp

  outputs[STOC_OUTPUT].value = 5.0f;
}

struct StochStepperWidget : ModuleWidget {
	StochStepperWidget(StochStepper *module) : ModuleWidget(module) {
		setPanel(SVG::load(assetPlugin(plugin, "res/StochStepper.svg")));

		addChild(Widget::create<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(Widget::create<ScrewSilver>(Vec(box.size.x - 6 * RACK_GRID_WIDTH, 0)));
		addChild(Widget::create<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(Widget::create<ScrewSilver>(Vec(box.size.x - 6 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

    //addParam(ParamWidget::create<CKSSThree>(Vec(110, 240), module, StochStepper::ENVS_PARAM, 0.0f, 2.0f, 0.0f));
		
    //addParam(ParamWidget::create<RoundHugeBlackKnob>(Vec(15, 68), module, StochStepper::FREQ_PARAM, -1.0, 1.0, 0.0));
   	//addInput(Port::create<PJ301MPort>(Vec(76.210, 285.33), Port::INPUT, module, StochStepper::GRAT_INPUT));

    addOutput(Port::create<PJ301MPort>(Vec(134.003, 334.86), Port::OUTPUT, module, StochStepper::STOC_OUTPUT));

		//addChild(ModuleLightWidget::create<MediumLight<RedLight>>(Vec(41, 59), module, StochStepper::BLINK_LIGHT));
	}
};


// Specify the Module and ModuleWidget subclass, human-readable
// author name for categorization per plugin, module slug (should never
// change), human-readable module name, and any number of tags
// (found in `include/tags.hpp`) separated by commas.
Model *modelStochStepper = Model::create<StochStepper, StochStepperWidget>("Gendy", "StochStepper", "Stochastic Stepper", OSCILLATOR_TAG);
