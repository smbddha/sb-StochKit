#include "util/common.hpp"
#include "dsp/digital.hpp"

#include "Gendy.hpp"
#include "wavetable.hpp"
#include "GendyOscillator.hpp"

#define NUM_OSCS 3

struct Stitcher : Module {
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

  SchmittTrigger smpTrigger;
  
  GendyOscillator gos[NUM_OSCS];
  int osc_idx = 0;

  float phase = 0.f;
  float amp = 0.f;
  float amp_next = 0.f;
  float amp_out = 0.f;
  float speed = 0.f;

  bool is_swapping = false;
  int stutter = 1;
  int s_count = stutter;

  Stitcher() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {
  }
	
  void step() override;
  float wrap(float,float,float);
};

void Stitcher::step() {
  float deltaTime = engineGetSampleTime();

  int new_nbpts = clamp((int) params[BPTS_PARAM].value, 3, MAX_BPTS);
  if (new_nbpts != gos[0].num_bpts) gos[0].num_bpts = new_nbpts;

  //if (phase >= 1.0) debug("PITCH PARAM: %f\n", (float) params[PITCH_PARAM].value);

  if (smpTrigger.process(params[TRIG_PARAM].value)) {
  }
 
  if (is_swapping) {
    amp_out = ((1.0 - phase) * amp) + (phase * amp_next); 

    phase += speed;
    
    if (phase >= 1.0) is_swapping = false;
  } else {
    gos[osc_idx].max_amp_step = rescale(params[STEP_PARAM].value, 0.0, 1.0, 0.05, 0.3);
    gos[osc_idx].freq_mul = rescale(params[FREQ_PARAM].value, -1.0, 1.0, 0.5, 4.0);
    gos[osc_idx].g_rate = params[GRAT_PARAM].value * 5.f;

    gos[osc_idx].process(deltaTime);
    amp_out = gos[osc_idx].out();
    
    if (gos[osc_idx].last_flag) {
      s_count--;
      if (s_count < 0) {
        debug("SWAPPING FROM %d", osc_idx);
        
        amp = amp_out;
        speed = gos[osc_idx].speed;
        osc_idx = (osc_idx + 1) % NUM_OSCS;
        
        gos[osc_idx].process(deltaTime);
        amp_next = gos[osc_idx].out();  
        
        s_count = stutter;
        phase = 0.f;
        is_swapping = true;
      }
    }
  }
  
  outputs[SINE_OUTPUT].value = 5.0f * amp_out;
}

struct StitcherWidget : ModuleWidget {
	StitcherWidget(Stitcher *module) : ModuleWidget(module) {
		setPanel(SVG::load(assetPlugin(plugin, "res/MyModule3.svg")));

		addChild(Widget::create<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(Widget::create<ScrewSilver>(Vec(box.size.x - 6 * RACK_GRID_WIDTH, 0)));
		addChild(Widget::create<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(Widget::create<ScrewSilver>(Vec(box.size.x - 6 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(ParamWidget::create<Davies1900hBlackKnob>(Vec(40, 140), module, Stitcher::FREQ_PARAM, -1.0, 1.0, 0.0));
    addParam(ParamWidget::create<Davies1900hBlackKnob>(Vec(110, 140), module, Stitcher::STEP_PARAM, 0.0, 1.0, 0.9));
    addParam(ParamWidget::create<Davies1900hBlackKnob>(Vec(40, 200), module, Stitcher::GRAT_PARAM, 0.7, 1.3, 0.0));
    addParam(ParamWidget::create<Davies1900hBlackKnob>(Vec(110, 200), module, Stitcher::BPTS_PARAM, 3, MAX_BPTS, 0));
    addParam(ParamWidget::create<CKD6>(Vec(40, 70), module, Stitcher::TRIG_PARAM, 0.0f, 1.0f, 0.0f));
		
    addInput(Port::create<PJ301MPort>(Vec(33, 245), Port::INPUT, module, Stitcher::WAV0_INPUT));
		addOutput(Port::create<PJ301MPort>(Vec(33, 275), Port::OUTPUT, module, Stitcher::SINE_OUTPUT));

		addChild(ModuleLightWidget::create<MediumLight<RedLight>>(Vec(41, 59), module, Stitcher::BLINK_LIGHT));
	}
};


// Specify the Module and ModuleWidget subclass, human-readable
// author name for categorization per plugin, module slug (should never
// change), human-readable module name, and any number of tags
// (found in `include/tags.hpp`) separated by commas.
Model *modelStitcher = Model::create<Stitcher, StitcherWidget>("Gendy", "Stitcher", "Stitcher Module", OSCILLATOR_TAG);
