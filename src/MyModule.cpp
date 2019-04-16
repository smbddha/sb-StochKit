#include "util/common.hpp"
#include "dsp/digital.hpp"

#include "Gendy.hpp"
#include "wavetable.hpp"

#define MAX_BPTS 30

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

  void process(float deltaTime) {
    if (phase >= 1.0) {
      
      //debug("-- PHASE: %f ; G_IDX: %f ; G_IDX_NEXT: %f", phase, g_idx, g_idx_next);
      phase -= 1.0;

      amp = amp_next;
      index = (index + 1) % num_bpts;

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

struct MyModule : Module {
	enum ParamIds {
		FREQ_PARAM,
    STEP_PARAM,
    BPTS_PARAM,
    GRAT_PARAM,
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

	//float phase = 1.0;
	float blinkPhase = 0.0;

  enum InterpolationTypes {
    LINEAR,
    COSINE,
    GRANULAR
  };

  SchmittTrigger smpTrigger;
  GendyOscillator go;

  /*
   * GENDY VARS
   */

  // For more advanced Module features, read Rack's engine.hpp header file
	// - toJson, fromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - onReset, onRandomize, onCreate, onDelete: implements special behavior when user clicks these from the context menu

  MyModule() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {
  }
	
  void step() override;
  float wrap(float,float,float);
};

void MyModule::step() {
	// Implement a simple sine oscillator
  float deltaTime = engineGetSampleTime();

  int new_nbpts = clamp((int) params[BPTS_PARAM].value, 3, MAX_BPTS);
  if (new_nbpts != go.num_bpts) go.num_bpts = new_nbpts;

  //if (phase >= 1.0) debug("PITCH PARAM: %f\n", (float) params[PITCH_PARAM].value);

  if (smpTrigger.process(params[TRIG_PARAM].value)) {
    
  }
  
  go.max_amp_step = rescale(params[STEP_PARAM].value, 0.0, 1.0, 0.05, 0.2);
  go.freq_mul = rescale(params[FREQ_PARAM].value, -1.0, 1.0, 0.5, 2.0);
  go.g_rate = params[GRAT_PARAM].value * 5.f;

  go.process(deltaTime);

  outputs[SINE_OUTPUT].value = 5.0f * go.out();
}



struct MyModuleWidget : ModuleWidget {
	MyModuleWidget(MyModule *module) : ModuleWidget(module) {
		setPanel(SVG::load(assetPlugin(plugin, "res/MyModule3.svg")));

		addChild(Widget::create<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(Widget::create<ScrewSilver>(Vec(box.size.x - 6 * RACK_GRID_WIDTH, 0)));
		addChild(Widget::create<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(Widget::create<ScrewSilver>(Vec(box.size.x - 6 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(ParamWidget::create<Davies1900hBlackKnob>(Vec(40, 140), module, MyModule::FREQ_PARAM, -1.0, 1.0, 0.0));
    addParam(ParamWidget::create<Davies1900hBlackKnob>(Vec(110, 140), module, MyModule::STEP_PARAM, 0.0, 1.0, 0.9));
    addParam(ParamWidget::create<Davies1900hBlackKnob>(Vec(40, 200), module, MyModule::GRAT_PARAM, 0.7, 1.3, 0.0));
    addParam(ParamWidget::create<Davies1900hBlackKnob>(Vec(110, 200), module, MyModule::BPTS_PARAM, 3, MAX_BPTS, 0));
    addParam(ParamWidget::create<CKD6>(Vec(40, 70), module, MyModule::TRIG_PARAM, 0.0f, 1.0f, 0.0f));
		
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
