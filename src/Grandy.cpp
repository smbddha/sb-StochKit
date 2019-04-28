/*
 * Grandy.cpp
 * Samuel Laing - 2019
 *
 * VCV Rack Module with a single GRANDY oscillator
 *
 */

#include "StochKit.hpp"
#include "dsp/resampler.hpp"

#include "GrandyOscillator.hpp"

struct Grandy : Module {
	enum ParamIds {
		FREQ_PARAM,
    ASTP_PARAM,
    DSTP_PARAM,
    BPTS_PARAM,
    GRAT_PARAM,
    GRATCV_PARAM,
    FREQCV_PARAM,
    ASTPCV_PARAM,
    DSTPCV_PARAM,
    BPTSCV_PARAM,
    TRIG_PARAM,
    FMTR_PARAM,
		ENVS_PARAM,
    FMOD_PARAM,
    FCAR_PARAM,
    IMOD_PARAM,
    FMODCV_PARAM,
    IMODCV_PARAM,
    PDST_PARAM,
    MIRR_PARAM,
    NUM_PARAMS
	};
	enum InputIds {
		FREQ_INPUT,
    ASTP_INPUT,
    DSTP_INPUT,
    BPTS_INPUT,
    ENVS_INPUT,
    FMOD_INPUT,
    IMOD_INPUT,
    GRAT_INPUT,
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
  
  GendyOscillator go;

  EnvType env = (EnvType) 1;

  float freq_sig = 0.f;
  float astp_sig = 0.f;
  float dstp_sig = 0.f;
  float grat_sig = 0.f;
  float envs_sig = 0.f;
  float bpts_sig = 0.f;
  float fmod_sig = 0.f;
  float imod_sig = 0.f;

  bool fm_is_on = false;

  // For more advanced Module features, read Rack's engine.hpp header file
	// - toJson, fromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - onReset, onRandomize, onCreate, onDelete: implements special behavior when user clicks these from the context menu

  Grandy() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {
  }
	
  void step() override;
  float wrap(float,float,float);
};

void Grandy::step() {
  float deltaTime = engineGetSampleTime();

  // snap knob for selecting envelope for the grain
  int env_num = (int) clamp(roundf(params[ENVS_PARAM].value), 1.0f, 4.0f);

  if (env != (EnvType) env_num) {
    debug("Switching to env type: %d", env_num);
    env = (EnvType) env_num;
    go.env.switchEnvType(env);
  }

  // handle mirror / fold switch
  go.is_mirroring = (int) params[MIRR_PARAM].value;
 
  // accept modulation of signal inputs for each parameter
  freq_sig = (inputs[FREQ_INPUT].value / 5.f) * params[FREQCV_PARAM].value;
  
  bpts_sig = 5.f * quadraticBipolar((inputs[BPTS_INPUT].value / 5.f) * params[BPTSCV_PARAM].value);
  astp_sig = quadraticBipolar((inputs[ASTP_INPUT].value / 5.f) * params[ASTPCV_PARAM].value);
  dstp_sig = quadraticBipolar((inputs[DSTP_INPUT].value / 5.f) * params[DSTPCV_PARAM].value);
  grat_sig = (inputs[GRAT_INPUT].value / 5.f) * params[GRATCV_PARAM].value;
 
  // fm control sigs
  fmod_sig = (inputs[FMOD_INPUT].value / 5.f) * params[FMODCV_PARAM].value;
  imod_sig = quadraticBipolar((inputs[IMOD_INPUT].value / 5.f) * params[IMODCV_PARAM].value);

  int new_nbpts = clamp((int) params[BPTS_PARAM].value + (int) bpts_sig, 2, MAX_BPTS);
  if (new_nbpts != go.num_bpts) go.num_bpts = new_nbpts;

  // better frequency control
  freq_sig += params[FREQ_PARAM].value;
  grat_sig += params[GRAT_PARAM].value;

  go.freq = clamp(261.626f * powf(2.0f, freq_sig), 1.f, 3000.f);

  go.max_amp_step = rescale(params[ASTP_PARAM].value + (astp_sig / 4.f), 0.0, 1.0, 0.05, 0.3);
  go.max_dur_step = rescale(params[DSTP_PARAM].value + (dstp_sig / 4.f), 0.0, 1.0, 0.01, 0.3);
  go.freq_mul = rescale(params[FREQ_PARAM].value, -1.0, 1.0, 0.05, 4.0);
  go.g_rate = clamp(261.626f * powf(2.0f, grat_sig), 1e-6, 3000.f);

  go.dt = (DistType) params[PDST_PARAM].value;

  // set fm params
  go.is_fm_on = !(params[FMTR_PARAM].value > 0.0f);
 
  fmod_sig += params[FMOD_PARAM].value;
  imod_sig += params[IMOD_PARAM].value;

  go.f_car = clamp(261.626f * powf(2.0f, params[FCAR_PARAM].value), 1.f, 5000.f);
  go.f_mod = clamp(261.626f * powf(2.0f, fmod_sig), 1.f, 5000.f);
  
  go.i_mod = rescale(params[IMOD_PARAM].value, 0.f, 1.f, 10.f, 3000.f);

  go.process(deltaTime);

  outputs[SINE_OUTPUT].value = 5.0f * go.out();
}


struct GrandyWidget : ModuleWidget {
	GrandyWidget(Grandy *module) : ModuleWidget(module) {
		setPanel(SVG::load(assetPlugin(plugin, "res/Grandy.svg")));

    /*
		addChild(Widget::create<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(Widget::create<ScrewSilver>(Vec(box.size.x - 1 * RACK_GRID_WIDTH, 0)));
		addChild(Widget::create<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(Widget::create<ScrewSilver>(Vec(box.size.x - 1 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
    */

    //addParam(ParamWidget::create<CKSSThree>(Vec(110, 240), module, Grandy::ENVS_PARAM, 0.0f, 2.0f, 0.0f));
	
    // knob params
    addParam(ParamWidget::create<RoundLargeBlackKnob>(Vec(36.307, 50.42), module, Grandy::FREQ_PARAM, -4.0, 3.0, 0.0));
    addParam(ParamWidget::create<RoundSmallBlackKnob>(Vec(61.360, 94.21), module, Grandy::FREQCV_PARAM, 0.f, 1.f, 0.f));
    
    addParam(ParamWidget::create<RoundLargeBlackKnob>(Vec(104.307, 50.42), module, Grandy::BPTS_PARAM, 3, MAX_BPTS, 0));
    addParam(ParamWidget::create<RoundSmallBlackKnob>(Vec(129.360, 94.21), module, Grandy::BPTSCV_PARAM, 0.f, 1.f, 0.f));
    
    addParam(ParamWidget::create<RoundLargeBlackKnob>(Vec(14.307, 145.54), module, Grandy::DSTP_PARAM, 0.f, 1.f, 0.f));
    addParam(ParamWidget::create<RoundSmallBlackKnob>(Vec(39.360, 191.10), module, Grandy::DSTPCV_PARAM, 0.f, 1.f, 0.f));
    
    addParam(ParamWidget::create<RoundLargeBlackKnob>(Vec(84.307, 145.54), module, Grandy::ASTP_PARAM, 0.f, 1.f, 0.f));
    addParam(ParamWidget::create<RoundSmallBlackKnob>(Vec(107.360, 191.10), module, Grandy::ASTPCV_PARAM, 0.f, 1.f, 0.f));
    
    addParam(ParamWidget::create<CKSSThree>(Vec(143.417, 147.64), module, Grandy::PDST_PARAM, 0.f, 2.f, 0.f)); 
    addParam(ParamWidget::create<CKSS>(Vec(143.379, 202.07), module, Grandy::MIRR_PARAM, 0.f, 1.f, 0.f)); 

    addParam(ParamWidget::create<RoundSmallBlackKnob>(Vec(35.360, 243.98), module, Grandy::GRAT_PARAM, -6.f, 3.f, 0.f));
    addParam(ParamWidget::create<RoundSmallBlackKnob>(Vec(69.360, 243.98), module, Grandy::GRATCV_PARAM, 0.f, 1.f, 0.f));

    addParam(ParamWidget::create<RoundBlackSnapKnob>(Vec(141.195, 240.69), module, Grandy::ENVS_PARAM, 1.0f, 4.0f, 4.0f));

    // for fm 
    addParam(ParamWidget::create<RoundSmallBlackKnob>(Vec(23.360, 302.25), module, Grandy::FCAR_PARAM, -4.f, 4.f, 0.f));
    
    addParam(ParamWidget::create<RoundSmallBlackKnob>(Vec(63.360, 302.25), module, Grandy::FMOD_PARAM, -4.f, 4.f, 0.f));
    addParam(ParamWidget::create<RoundSmallBlackKnob>(Vec(97.360, 302.25), module, Grandy::FMODCV_PARAM, 0.f, 1.f, 0.f));

    addParam(ParamWidget::create<RoundSmallBlackKnob>(Vec(13.360, 348.84), module, Grandy::IMOD_PARAM, -4.f, 4.f, 0.f));
    addParam(ParamWidget::create<RoundSmallBlackKnob>(Vec(47.360, 348.84), module, Grandy::IMODCV_PARAM, 0.f, 1.f, 0.f));

	  addParam(ParamWidget::create<CKSS>(Vec(11.360, 257.01), module, Grandy::FMTR_PARAM, 0.0f, 1.0f, 0.0f));
    
    // signal inputs
    addInput(Port::create<PJ301MPort>(Vec(24.967, 93.61), Port::INPUT, module, Grandy::FREQ_INPUT));
    addInput(Port::create<PJ301MPort>(Vec(92.967, 93.61), Port::INPUT, module, Grandy::BPTS_INPUT));
    
    addInput(Port::create<PJ301MPort>(Vec(2.976, 188.72), Port::INPUT, module, Grandy::ASTP_INPUT));
		addInput(Port::create<PJ301MPort>(Vec(70.966, 188.72), Port::INPUT, module, Grandy::DSTP_INPUT));
    
    //addInput(Port::create<PJ301MPort>(Vec(100, 340.42), Port::INPUT, module, Grandy::ENVS_INPUT));
		
    addInput(Port::create<PJ301MPort>(Vec(102.966, 243.50), Port::INPUT, module, Grandy::GRAT_INPUT));
   
    // for fm
		addInput(Port::create<PJ301MPort>(Vec(130.966, 300.72), Port::INPUT, module, Grandy::FMOD_INPUT));
		addInput(Port::create<PJ301MPort>(Vec(82.966, 348.50), Port::INPUT, module, Grandy::IMOD_INPUT));

    // output signal 
    addOutput(Port::create<PJ301MPort>(Vec(124.003, 348.50), Port::OUTPUT, module, Grandy::SINE_OUTPUT));
	}
};


// Specify the Module and ModuleWidget subclass, human-readable
// author name for categorization per plugin, module slug (should never
// change), human-readable module name, and any number of tags
// (found in `include/tags.hpp`) separated by commas.
Model *modelGrandy = Model::create<Grandy, GrandyWidget>("StochKit", "Grandy", "A granular dynamic stochastic synthesis generator", OSCILLATOR_TAG);
