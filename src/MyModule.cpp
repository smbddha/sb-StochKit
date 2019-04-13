#include "util/common.hpp"
#include "dsp/digital.hpp"

#include "Gendy.hpp"

#define MAX_BPTS 30

struct MyModule : Module {
	enum ParamIds {
		FREQ_PARAM,
    STEP_PARAM,
    BPTS_PARAM,
    TRIG_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		WAV0_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		SINE_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		BLINK_LIGHT,
		NUM_LIGHTS
	};

	float phase = 1.0;
	float blinkPhase = 0.0;


  enum State {
    SAMPLING,
    GENERATING,
    NUM_STATES
  };

  SchmittTrigger smpTrigger;

  /*
   * GENDY VARS
   */

  int num_bpts = 12;
  State state = SAMPLING;

  int min_freq = 30; 
  int max_freq = 1000;


  float mAmps[MAX_BPTS] = {0.};
  float mDurs[MAX_BPTS] = {0.};

  int index = 0;
  float amp = 0.0; 

  float max_amp_step = 0.05;
  float max_dur_step = 0.0;
  float speed = 0.0;
  float rate = 0.0;

  float freq_mul = 1.0;

  /*
   * TODO
   * wavetable integration ?
   */

  /*
   * Do random initialization stuff
   */

  /*
  for (int i=0; i<12; i++) {
    mAmps[i] = rack::randomNormal(); mDurs[i] = 0.6;
  } 
  */

  float sample[48000];
  float amp_next = mAmps[0];

	// For more advanced Module features, read Rack's engine.hpp header file
	// - toJson, fromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - onReset, onRandomize, onCreate, onDelete: implements special behavior when user clicks these from the context menu

  MyModule() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {
  
    //randomInit();
  }
	void step() override;
  float wrap(float,float,float);
};

void MyModule::step() {
	// Implement a simple sine oscillator
  float deltaTime = engineGetSampleTime();
  float amp_out = 0.0;

  int new_nbpts = clamp((int) params[BPTS_PARAM].value, 3, MAX_BPTS);
  if (new_nbpts != num_bpts) num_bpts = new_nbpts;

  //if (phase >= 1.0) debug("PITCH PARAM: %f\n", (float) params[PITCH_PARAM].value);

  if (state==SAMPLING) {
    speed = ((max_freq - min_freq) * 0.5 + min_freq) * deltaTime * num_bpts; 

    if (phase >= 1.0) {
      mAmps[index] = inputs[WAV0_INPUT].value;

      index++;
      if (index >= num_bpts) {
        state = GENERATING;
        index = 0;
      }
    } 
  } else if (state==GENERATING) {
    if (smpTrigger.process(params[TRIG_PARAM].value)) {
	    state = SAMPLING; index = 0;
    }
    
    max_amp_step = rescale(params[STEP_PARAM].value, 0.0, 1.0, 0.05, 0.2);
    freq_mul = rescale(params[FREQ_PARAM].value, -1.0, 1.0, 0.5, 2.0);

    if (phase >= 1.0) {
      phase -= 1.0;

      amp = amp_next;
      index = (index + 1) % num_bpts;

      /* adjust vals */
      mAmps[index] = wrap(mAmps[index] + (max_amp_step * randomNormal()), -1.0f, 1.0f); 
      mDurs[index] = mDurs[index] + (max_dur_step * randomNormal());
   
      amp_next = mAmps[index];
      rate = mDurs[index];

      speed = ((max_freq - min_freq) * rate + min_freq) * deltaTime * num_bpts; 
      speed *= freq_mul;
    }
   
    amp_out = ((1.0 - phase) * amp) + (phase * amp_next);

  }

  phase += speed;

  //debug("Phase %f\n", phase);
  outputs[SINE_OUTPUT].value = 5.0f * amp_out;
}

float MyModule::wrap(float in, float lb, float ub) {
  float out = in;
  if (in > ub) out = ub;
  else if (in < lb) out = lb;
  return out;
}

struct MyModuleWidget : ModuleWidget {
	MyModuleWidget(MyModule *module) : ModuleWidget(module) {
		setPanel(SVG::load(assetPlugin(plugin, "res/MyModule3.svg")));

		addChild(Widget::create<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(Widget::create<ScrewSilver>(Vec(box.size.x - 6 * RACK_GRID_WIDTH, 0)));
		addChild(Widget::create<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(Widget::create<ScrewSilver>(Vec(box.size.x - 6 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(ParamWidget::create<Davies1900hBlackKnob>(Vec(28, 87), module, MyModule::FREQ_PARAM, -1.0, 1.0, 0.0));
    addParam(ParamWidget::create<Davies1900hBlackKnob>(Vec(68, 87), module, MyModule::STEP_PARAM, 0.0, 1.0, 0.9));
    addParam(ParamWidget::create<CKD6>(Vec(33, 185), module, MyModule::TRIG_PARAM, 0.0f, 1.0f, 0.0f));
    addParam(ParamWidget::create<Davies1900hBlackKnob>(Vec(33, 210), module, MyModule::BPTS_PARAM, 3, MAX_BPTS, 0));

		addInput(Port::create<PJ301MPort>(Vec(33, 245), Port::INPUT, module, MyModule::WAV0_INPUT));

		addOutput(Port::create<PJ301MPort>(Vec(33, 275), Port::OUTPUT, module, MyModule::SINE_OUTPUT));

		addChild(ModuleLightWidget::create<MediumLight<RedLight>>(Vec(41, 59), module, MyModule::BLINK_LIGHT));
	}
};


// Specify the Module and ModuleWidget subclass, human-readable
// author name for categorization per plugin, module slug (should never
// change), human-readable module name, and any number of tags
// (found in `include/tags.hpp`) separated by commas.
Model *modelMyModule = Model::create<MyModule, MyModuleWidget>("Gendy", "MyModule", "My Module", OSCILLATOR_TAG);
