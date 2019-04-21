#include "util/common.hpp"
#include "dsp/digital.hpp"

#include "Gendy.hpp"
#include "wavetable.hpp"
#include "GendyOscillator.hpp"

#define NUM_OSCS 4

struct Stitcher : Module {
	enum ParamIds {
		FREQ_PARAM,
    STEP_PARAM,
    BPTS_PARAM,
    GRAT_PARAM,
    TRIG_PARAM,
    ENUMS(F_PARAM, NUM_OSCS),
    ENUMS(B_PARAM, NUM_OSCS),
    ENUMS(S_PARAM, NUM_OSCS),
    ENUMS(G_PARAM, NUM_OSCS),
    ENUMS(ST_PARAM, NUM_OSCS),
    NUM_PARAMS
	};
	enum InputIds {
		WAV0_INPUT,
		ENUMS(F_INPUT, NUM_OSCS),
    ENUMS(B_INPUT, NUM_OSCS),
    ENUMS(S_INPUT, NUM_OSCS),
    ENUMS(G_INPUT, NUM_OSCS),
    ENUMS(ST_INPUT, NUM_OSCS),
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

  // allow an adjustable number of oscillators
  // to be used 2 -> 4
  int curr_num_oscs = NUM_OSCS;
  int stutters[NUM_OSCS] = {1};
  int current_stutter = 1;

  float phase = 0.f;
  float amp = 0.f;
  float amp_next = 0.f;
  float amp_out = 0.f;
  float speed = 0.f;

  bool is_swapping = false;
  int stutter = 1;

  Stitcher() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {
  }
	
  void step() override;
  float wrap(float,float,float);
};

void Stitcher::step() {
  float deltaTime = engineGetSampleTime();

  
  //if (phase >= 1.0) debug("PITCH PARAM: %f\n", (float) params[PITCH_PARAM].value);

  // TODO
  // add global controls

  // read in all the parameters for each oscillator
  for (int i=0; i<NUM_OSCS; i++) {
    stutters[i] = (int) params[ST_PARAM + i].value;
    
    gos[i].max_amp_step = rescale(params[S_PARAM + i].value, 0.0, 1.0, 0.05, 0.3);
    gos[i].freq_mul = rescale(params[F_PARAM + i].value, -1.0, 1.0, 0.5, 4.0);
    gos[i].g_rate = params[G_PARAM + i].value * 5.f;
  
    int new_nbpts = clamp((int) params[B_PARAM + i].value, 3, MAX_BPTS);
    if (new_nbpts != gos[i].num_bpts) gos[i].num_bpts = new_nbpts;
    //debug("I: %d", i);
  }

  if (smpTrigger.process(params[TRIG_PARAM].value)) {
  }
 
  if (is_swapping) {
    amp_out = ((1.0 - phase) * amp) + (phase * amp_next); 

    phase += speed;
    
    if (phase >= 1.0) is_swapping = false;
  } else {
    
    gos[osc_idx].process(deltaTime);
    amp_out = gos[osc_idx].out();
    
    if (gos[osc_idx].last_flag) {
      current_stutter--;
      if (current_stutter < 1) {
        amp = amp_out;
        speed = gos[osc_idx].speed;
        osc_idx = (osc_idx + 1) % NUM_OSCS;
        
        debug("-- new idx %d, bpts: %d, freq_mul: %d", osc_idx, gos[osc_idx].num_bpts, gos[osc_idx].freq_mul); 

        gos[osc_idx].process(deltaTime);
        amp_next = gos[osc_idx].out();  
       
        current_stutter = stutters[osc_idx];

        phase = 0.f;
        is_swapping = true;
      }
    }
  }
  
  outputs[SINE_OUTPUT].value = 5.0f * amp_out;
}

struct StitcherWidget : ModuleWidget {
	StitcherWidget(Stitcher *module) : ModuleWidget(module) {
		setPanel(SVG::load(assetPlugin(plugin, "res/Stitch.svg")));

		addChild(Widget::create<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(Widget::create<ScrewSilver>(Vec(box.size.x - 6 * RACK_GRID_WIDTH, 0)));
		addChild(Widget::create<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(Widget::create<ScrewSilver>(Vec(box.size.x - 6 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(ParamWidget::create<CKD6>(Vec(40, 70), module, Stitcher::TRIG_PARAM, 0.0f, 1.0f, 0.0f));

    int index = 0;
    for (int i = 0; i < NUM_OSCS/2; i++) {
			for (int j = 0; j < NUM_OSCS/2; j++) {
        addParam(ParamWidget::create<RoundSmallBlackKnob>(Vec(30+(i*115), 100+(j*115)), module, Stitcher::F_PARAM + index, -1.0, 1.0, 0.0));
        addParam(ParamWidget::create<RoundSmallBlackKnob>(Vec(30+(i*115), 140+(j*115)), module, Stitcher::S_PARAM + index, 0.0, 1.0, 0.9));
        
        // stutter param
        addParam(ParamWidget::create<RoundBlackSnapKnob>(Vec(60+(i*115), 120+(j*115)), module, Stitcher::ST_PARAM + index, 1.f, 5.f, 5.f));

        addParam(ParamWidget::create<RoundSmallBlackKnob>(Vec(90+(i*115), 100+(j*115)), module, Stitcher::G_PARAM + index, 0.7, 1.3, 0.0));
        addParam(ParamWidget::create<RoundSmallBlackKnob>(Vec(90+(i*115), 140+(j*115)), module, Stitcher::B_PARAM + index, 3, MAX_BPTS, 0));
        index++;
      }
    }

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
