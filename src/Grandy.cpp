/*
 * Grandy.cpp
 * Samuel Laing - 2019
 *
 * VCV Rack Module with a single GRANDY oscillator
 *
 */

#include "plugin.hpp"
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

	float blinkPhase = 0.0;

  dsp::SchmittTrigger smpTrigger;
  
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

  Grandy() {
    config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

    configParam(FREQ_PARAM, -4.0, 3.0, 0.0);
    configParam(FREQCV_PARAM, 0.f, 1.f, 0.f);
    configParam(BPTS_PARAM, 3, MAX_BPTS, 0);
    configParam(BPTSCV_PARAM, 0.f, 1.f, 0.f);
    configParam(DSTP_PARAM, 0.f, 1.f, 0.f);
    configParam(DSTPCV_PARAM, 0.f, 1.f, 0.f);
    configParam(ASTP_PARAM, 0.f, 1.f, 0.f);
    configParam(ASTPCV_PARAM, 0.f, 1.f, 0.f);
    configParam(PDST_PARAM, 0.f, 2.f, 0.f);
    configParam(MIRR_PARAM, 0.f, 1.f, 0.f);
    configParam(GRAT_PARAM, -6.f, 3.f, 0.f);
    configParam(GRATCV_PARAM, 0.f, 1.f, 0.f);
    configParam(ENVS_PARAM, 1.0f, 4.0f, 4.0f);
    configParam(FCAR_PARAM, -4.f, 4.f, 0.f);
    configParam(FMOD_PARAM, -4.f, 4.f, 0.f);
    configParam(FMODCV_PARAM, 0.f, 1.f, 0.f);
    configParam(IMOD_PARAM, -4.f, 4.f, 0.f);
    configParam(IMODCV_PARAM, 0.f, 1.f, 0.f);
    configParam(FMTR_PARAM, 0.0f, 1.0f, 0.0f);
  }

  void process(const ProcessArgs &args) override;
  float wrap(float,float,float);
};

void Grandy::process(const ProcessArgs &args) {
  float deltaTime = args.sampleTime;

  // snap knob for selecting envelope for the grain
  int env_num = (int) clamp(roundf(params[ENVS_PARAM].getValue()), 1.0f, 4.0f);

  if (env != (EnvType) env_num) {
    DEBUG("Switching to env type: %d", env_num);
    env = (EnvType) env_num;
    go.env.switchEnvType(env);
  }

  // handle mirror / fold switch
  go.is_mirroring = (int) params[MIRR_PARAM].getValue();
 
  // accept modulation of signal inputs for each parameter
  freq_sig = (inputs[FREQ_INPUT].getVoltage() / 5.f) * params[FREQCV_PARAM].getValue();
  
  bpts_sig = 5.f * dsp::quadraticBipolar((inputs[BPTS_INPUT].getVoltage() / 5.f) * params[BPTSCV_PARAM].getValue());
  astp_sig = dsp::quadraticBipolar((inputs[ASTP_INPUT].getVoltage() / 5.f) * params[ASTPCV_PARAM].getValue());
  dstp_sig = dsp::quadraticBipolar((inputs[DSTP_INPUT].getVoltage() / 5.f) * params[DSTPCV_PARAM].getValue());
  grat_sig = (inputs[GRAT_INPUT].getVoltage() / 5.f) * params[GRATCV_PARAM].getValue();
 
  // fm control sigs
  fmod_sig = (inputs[FMOD_INPUT].getVoltage() / 5.f) * params[FMODCV_PARAM].getValue();
  imod_sig = dsp::quadraticBipolar((inputs[IMOD_INPUT].getVoltage() / 5.f) * params[IMODCV_PARAM].getValue());

  int new_nbpts = clamp((int) params[BPTS_PARAM].getValue() + (int) bpts_sig, 2, MAX_BPTS);
  if (new_nbpts != go.num_bpts) go.num_bpts = new_nbpts;

  // better frequency control
  freq_sig += params[FREQ_PARAM].getValue();
  grat_sig += params[GRAT_PARAM].getValue();

  go.freq = clamp(261.626f * powf(2.0f, freq_sig), 1.f, 3000.f);

  go.max_amp_step = rescale(params[ASTP_PARAM].getValue() + (astp_sig / 4.f), 0.0, 1.0, 0.05, 0.3);
  go.max_dur_step = rescale(params[DSTP_PARAM].getValue() + (dstp_sig / 4.f), 0.0, 1.0, 0.01, 0.3);
  go.freq_mul = rescale(params[FREQ_PARAM].getValue(), -1.0, 1.0, 0.05, 4.0);
  go.g_rate = clamp(261.626f * powf(2.0f, grat_sig), 1e-6, 3000.f);

  go.dt = (DistType) params[PDST_PARAM].getValue();

  // set fm params
  go.is_fm_on = !(params[FMTR_PARAM].getValue() > 0.0f);
 
  fmod_sig += params[FMOD_PARAM].getValue();
  imod_sig += params[IMOD_PARAM].getValue();

  go.f_car = clamp(261.626f * powf(2.0f, params[FCAR_PARAM].getValue()), 1.f, 5000.f);
  go.f_mod = clamp(261.626f * powf(2.0f, fmod_sig), 1.f, 5000.f);
  
  go.i_mod = rescale(params[IMOD_PARAM].getValue(), 0.f, 1.f, 10.f, 3000.f);

  go.process(deltaTime);

  outputs[SINE_OUTPUT].setVoltage(5.0f * go.out());
}


struct GrandyWidget : ModuleWidget {
	GrandyWidget(Grandy *module) {
    setModule(module);
    setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Grandy.svg")));

    /*
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 1 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 1 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
    */

    // knob params
    addParam(createParam<RoundLargeBlackKnob>(Vec(36.307, 50.42), module, Grandy::FREQ_PARAM));
    addParam(createParam<RoundSmallBlackKnob>(Vec(61.360, 94.21), module, Grandy::FREQCV_PARAM));
    
    addParam(createParam<RoundLargeBlackKnob>(Vec(104.307, 50.42), module, Grandy::BPTS_PARAM));
    addParam(createParam<RoundSmallBlackKnob>(Vec(129.360, 94.21), module, Grandy::BPTSCV_PARAM));
    
    addParam(createParam<RoundLargeBlackKnob>(Vec(14.307, 145.54), module, Grandy::DSTP_PARAM));
    addParam(createParam<RoundSmallBlackKnob>(Vec(39.360, 191.10), module, Grandy::DSTPCV_PARAM));
    
    addParam(createParam<RoundLargeBlackKnob>(Vec(84.307, 145.54), module, Grandy::ASTP_PARAM));
    addParam(createParam<RoundSmallBlackKnob>(Vec(107.360, 191.10), module, Grandy::ASTPCV_PARAM));
    
    addParam(createParam<CKSSThree>(Vec(143.417, 147.64), module, Grandy::PDST_PARAM)); 
    addParam(createParam<CKSS>(Vec(143.379, 202.07), module, Grandy::MIRR_PARAM)); 

    addParam(createParam<RoundSmallBlackKnob>(Vec(35.360, 243.98), module, Grandy::GRAT_PARAM));
    addParam(createParam<RoundSmallBlackKnob>(Vec(69.360, 243.98), module, Grandy::GRATCV_PARAM));

    addParam(createParam<RoundBlackSnapKnob>(Vec(141.195, 240.69), module, Grandy::ENVS_PARAM));

    // for fm 
    addParam(createParam<RoundSmallBlackKnob>(Vec(23.360, 302.25), module, Grandy::FCAR_PARAM));
    
    addParam(createParam<RoundSmallBlackKnob>(Vec(63.360, 302.25), module, Grandy::FMOD_PARAM));
    addParam(createParam<RoundSmallBlackKnob>(Vec(97.360, 302.25), module, Grandy::FMODCV_PARAM));

    addParam(createParam<RoundSmallBlackKnob>(Vec(13.360, 348.84), module, Grandy::IMOD_PARAM));
    addParam(createParam<RoundSmallBlackKnob>(Vec(47.360, 348.84), module, Grandy::IMODCV_PARAM));

	  addParam(createParam<CKSS>(Vec(11.360, 257.01), module, Grandy::FMTR_PARAM));
    
    // signal inputs
    addInput(createInput<PJ301MPort>(Vec(24.967, 93.61), module, Grandy::FREQ_INPUT));
    addInput(createInput<PJ301MPort>(Vec(92.967, 93.61), module, Grandy::BPTS_INPUT));
    
    addInput(createInput<PJ301MPort>(Vec(2.976, 188.72), module, Grandy::ASTP_INPUT));
		addInput(createInput<PJ301MPort>(Vec(70.966, 188.72), module, Grandy::DSTP_INPUT));
    
    addInput(createInput<PJ301MPort>(Vec(102.966, 243.50), module, Grandy::GRAT_INPUT));
   
    // for fm
		addInput(createInput<PJ301MPort>(Vec(130.966, 300.72), module, Grandy::FMOD_INPUT));
		addInput(createInput<PJ301MPort>(Vec(82.966, 348.50), module, Grandy::IMOD_INPUT));

    // output signal 
    addOutput(createOutput<PJ301MPort>(Vec(124.003, 348.50), module, Grandy::SINE_OUTPUT));
	}
};

Model *modelGrandy = createModel<Grandy, GrandyWidget>("Grandy");
