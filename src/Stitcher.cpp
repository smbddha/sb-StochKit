/*
 * Stitcher.cpp
 * Samuel Laing - 2019
 *
 * VCV Rack Stitcher module. An extened version of granular stochastic 
 * synthesis that connects waves produced by up to four seperate
 * GRANDY oscillators
 */

#include "util/common.hpp"
#include "dsp/digital.hpp"

#include "Gendy.hpp"
#include "wavetable.hpp"
#include "GendyOscillator.hpp"

#define NUM_OSCS 4

struct Stitcher : Module {
	enum ParamIds {
		G_FREQ_PARAM,
    G_ASTP_PARAM,
    G_DSTP_PARAM,
    G_BPTS_PARAM,
    G_GRAT_PARAM,
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
    ENUMS(ONOFF_LIGHT, NUM_OSCS),
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

  // vars for global parameter controls
  float g_freq_mul = 1.0;
  float g_max_amp_add = 0.f;
  float g_max_dur_add = 0.f;

  Stitcher() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {
  }
	
  void step() override;
  float wrap(float,float,float);
};

void Stitcher::step() {
  float deltaTime = engineGetSampleTime();

  
  //if (phase >= 1.0) debug("PITCH PARAM: %f\n", (float) params[PITCH_PARAM].value);

  // read in global controls
  g_freq_mul = params[G_FREQ_PARAM].value;
  g_max_amp_add = params[G_ASTP_PARAM].value;
  g_max_dur_add = params[G_DSTP_PARAM].value;

  // read in all the parameters for each oscillator
  for (int i=0; i<NUM_OSCS; i++) {
    stutters[i] = (int) params[ST_PARAM + i].value;
    
    gos[i].max_amp_step = rescale(params[S_PARAM + i].value, 0.0, 1.0, 0.05, 0.3) ;// + g_max_amp_add;
    gos[i].max_dur_step = g_max_dur_add;

    gos[i].freq_mul = rescale(params[F_PARAM + i].value, -1.0, 1.0, 0.5, 4.0) ;//* g_freq_mul;
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
        
        //debug("-- new idx %d, bpts: %d, freq_mul: %d", osc_idx, gos[osc_idx].num_bpts, gos[osc_idx].freq_mul); 

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

    /*
    // no screws ... at least for now
		addChild(Widget::create<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(Widget::create<ScrewSilver>(Vec(box.size.x - 6 * RACK_GRID_WIDTH, 0)));
		addChild(Widget::create<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(Widget::create<ScrewSilver>(Vec(box.size.x - 6 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
    */

		//addParam(ParamWidget::create<CKD6>(Vec(40, 70), module, Stitcher::TRIG_PARAM, 0.0f, 1.0f, 0.0f));

    int index = 0;
    for (int i = 0; i < NUM_OSCS; i++) {
      addParam(ParamWidget::create<RoundSmallBlackKnob>(Vec(9.140, 13.81+(i*95)), module, Stitcher::F_PARAM + index, -1.0, 1.0, 0.0));
      addParam(ParamWidget::create<RoundSmallBlackKnob>(Vec(9.140, 59.82+(i*95)), module, Stitcher::S_PARAM + index, 0.0, 1.0, 0.9));
      
      // stutter param
      addParam(ParamWidget::create<RoundBlackSnapKnob>(Vec(94.489, 24+(i*95)), module, Stitcher::ST_PARAM + index, 1.f, 5.f, 5.f));

      addParam(ParamWidget::create<RoundSmallBlackKnob>(Vec(55.140, 13.81+(i*95)), module, Stitcher::G_PARAM + index, 0.7, 1.3, 0.0));
      addParam(ParamWidget::create<RoundSmallBlackKnob>(Vec(55.140, 59.82+(i*95)), module, Stitcher::B_PARAM + index, 3, MAX_BPTS, 0));
    
      // light to signal if oscillator on / off 
		  addChild(ModuleLightWidget::create<MediumLight<RedLight>>(Vec(139.185, 80+(i*95)), module, Stitcher::ONOFF_LIGHT + index));
    }

    // global controls (on the right of the panel)
    addParam(ParamWidget::create<RoundHugeBlackKnob>(Vec(172.951, 56.23), module, Stitcher::G_FREQ_PARAM, 0.5f, 1.5f, 0.f));

    //addInput(Port::create<PJ301MPort>(Vec(33, 245), Port::INPUT, module, Stitcher::WAV0_INPUT));

		//addChild(ModuleLightWidget::create<MediumLight<GreenLight>>(Vec(41, 59), module, Stitcher::BLINK_LIGHT));
	
    addParam(ParamWidget::create<RoundLargeBlackKnob>(Vec(166.489, 152.32), module, Stitcher::G_ASTP_PARAM, 0.5f, 1.5f, 0.f));
    addParam(ParamWidget::create<RoundLargeBlackKnob>(Vec(218.489, 152.32), module, Stitcher::G_DSTP_PARAM, 0.5f, 1.5f, 0.f));
    addParam(ParamWidget::create<RoundLargeBlackKnob>(Vec(166.489, 225.86), module, Stitcher::G_BPTS_PARAM, 0.5f, 1.5f, 0.f));
    addParam(ParamWidget::create<RoundLargeBlackKnob>(Vec(218.489, 225.86), module, Stitcher::G_GRAT_PARAM, 0.5f, 1.5f, 0.f));

		addOutput(Port::create<PJ301MPort>(Vec(227, 339), Port::OUTPUT, module, Stitcher::SINE_OUTPUT));
  }
};


// Specify the Module and ModuleWidget subclass, human-readable
// author name for categorization per plugin, module slug (should never
// change), human-readable module name, and any number of tags
// (found in `include/tags.hpp`) separated by commas.
Model *modelStitcher = Model::create<Stitcher, StitcherWidget>("Gendy", "Stitcher", "Stitcher Module", OSCILLATOR_TAG);
