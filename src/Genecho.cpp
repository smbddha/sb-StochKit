/*
 * GenEcho (Gendy / Grandy Echo module)
 * Samuel Laing - 2019
 *
 * VCV Rack module that uses granular stochastic methods to alter a sample
 * sample can be loaded (wav files only) or be  piped in from another 
 * module
 */

#include "Gendy.hpp"

#include "util/common.hpp"
#include "dsp/digital.hpp"

#include "wavetable.hpp"

#define MAX_BPTS 4096 
#define MAX_SAMPLE_SIZE 44100 

struct GenEcho : Module {
	enum ParamIds {
    BPTS_PARAM,
		TRIG_PARAM,
    GATE_PARAM, 
    ASTP_PARAM,
    DSTP_PARAM,
    ENVS_PARAM,
    SLEN_PARAM,
    BPTSCV_PARAM,
    ASTPCV_PARAM,
    DSTPCV_PARAM,
    NUM_PARAMS
	};
	enum InputIds {
		WAV0_INPUT,
		GATE_INPUT,
    RSET_INPUT,
    BPTS_INPUT,
    ASTP_INPUT,
    DSTP_INPUT,
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
    LOADING,
    GENERATING,
    NUM_STATES
  };

  enum InterpolationTypes {
    LINEAR,
    COSINE,
    GRANULAR
  };

  SchmittTrigger smpTrigger;
  SchmittTrigger gTrigger;
  SchmittTrigger g2Trigger;
  SchmittTrigger resetTrigger;

  float sample[MAX_SAMPLE_SIZE] = {0.f};
  float _sample[MAX_SAMPLE_SIZE] = {0.f};

  unsigned int channels;
  unsigned int sampleRate;
 
  unsigned int sample_length = MAX_SAMPLE_SIZE;

  State state = LOADING;
  unsigned int idx = 0;

  // spacing between breakpoints... in samples rn
  unsigned int bpt_spc = 1500;
  unsigned int env_dur = bpt_spc / 2;

  // number of breakpoints - to be calculated according to size of
  // the sample
  unsigned int num_bpts = MAX_SAMPLE_SIZE / bpt_spc;

  float mAmps[MAX_BPTS] = {0.f};
  float mDurs[MAX_BPTS] = {1.f};
  //float mOffs[MAX_BPTS] = {0.f};

  Wavetable env = Wavetable(TRI); 

  unsigned int index = 0;
  
  float max_amp_step = 0.05f;
  float max_dur_step = 0.05f;

  float amp = 0.f; 
  float amp_next = 0.f;
  float g_idx = 0.f; 
  float g_idx_next = 0.5f;
 
  // when true read in from wav0_input and store in the sample buffer
  bool sampling = false;
  unsigned int s_i = 0;

  float num_bpts_cv = 1.f;
  float amp_step_cv = 1.f;
  float dur_step_cv = 1.f;

  GenEcho() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {}
	
  void step() override;
  float wrap(float,float,float);
};

void GenEcho::step() {
	// Implement a simple sine oscillator
  //float deltaTime = engineGetSampleTime();
  float amp_out = 0.0;

  // read in cv vals for astp, dstp and bpts
  // TODO
  // tweak the influence of the vals
  num_bpts_cv = inputs[BPTS_INPUT].value * params[BPTSCV_PARAM].value;
  amp_step_cv = inputs[ASTP_INPUT].value * params[ASTPCV_PARAM].value;
  dur_step_cv = inputs[DSTP_INPUT].value * params[DSTPCV_PARAM].value;
  
  amp_step_cv = rescale(amp_step_cv, -5.f, 5.f, -0.05, 0.05);

  sample_length = (int) (clamp(params[SLEN_PARAM].value, 0.1, 1.f) * MAX_SAMPLE_SIZE);

  max_amp_step = params[ASTP_PARAM].value + amp_step_cv;
  max_dur_step = params[DSTP_PARAM].value;

  bpt_spc = (unsigned int) params[BPTS_PARAM].value + 800;
  num_bpts = sample_length / bpt_spc + 1;
 
  env_dur = bpt_spc / 2;

  // snap knob for selecting envelope for the grain
  int env_num = (int) clamp(roundf(params[ENVS_PARAM].value), 1.0f, 4.0f);

  if (env.et != (EnvType) env_num) {
    env.switchEnvType((EnvType) env_num);
  }

  // handle sample reset
  if (smpTrigger.process(params[TRIG_PARAM].value)
      || resetTrigger.process(inputs[RSET_INPUT].value / 2.f)) {
    for (unsigned int i=0; i<MAX_SAMPLE_SIZE; i++) sample[i] = _sample[i];
    for (unsigned int i=0; i<MAX_BPTS; i++) {
      mAmps[i] = 0.f;
      mDurs[i] = 1.f; 
    }
  }

  // handle sample trigger through gate 
  if (g2Trigger.process(inputs[GATE_INPUT].value / 2.f)) {

    // reset accumulated breakpoint vals
    for (unsigned int i=0; i<MAX_BPTS; i++) {
      mAmps[i] = 0.f;
      mDurs[i] = 1.f;
    }

    num_bpts = sample_length / bpt_spc;
    sampling = true;
    idx = 0;
    s_i = 0;
  }

  // can be sampling but still output, just at a 1 sample delay
  // or will there even be a delay ??
  // -> actually no delay noice
  if (sampling) {
    if (s_i >= MAX_SAMPLE_SIZE - 50) {
      float x,y,p;
      x = sample[s_i-1];
      y = sample[0];
      p = 0.f;
      while (s_i < MAX_SAMPLE_SIZE) {
        sample[s_i] = (x * (1-p)) + (y * p);
        p += 1.f / 50.f;
        s_i++;
      }
      debug("Finished sampling");
      sampling = false;
    } else {
      sample[s_i] = inputs[WAV0_INPUT].value; 
      _sample[s_i] = sample[s_i];
      s_i++;
    } 
  }

  if (phase >= 1.0) {
    phase -= 1.0;

    amp = amp_next;
    index = (index + 1) % num_bpts;
    
    // adjust vals 
    mAmps[index] = wrap(mAmps[index] + (max_amp_step * randomNormal()), -1.0f, 1.0f); 
    amp_next = mAmps[index];

    mDurs[index] = wrap(mDurs[index] + (max_dur_step * randomNormal()), 0.8, 1.2);

    // step/adjust grain sample offsets 
    g_idx = g_idx_next;
    g_idx_next = 0.0;
  }

  // change amp in sample buffer
  sample[idx] = wrap(sample[idx] + (amp * env.get(g_idx)), -5.f, 5.f);
  amp_out = sample[idx];

  idx = (idx + 1) % sample_length;
  g_idx = fmod(g_idx + (1.f / (4.f * env_dur)), 1.f);
  g_idx_next = fmod(g_idx_next + (1.f / (4.f * env_dur)), 1.f);
  
  phase += 1.f / (mDurs[index] * bpt_spc);

  // get that amp OUT
  outputs[SINE_OUTPUT].value = amp_out;
}

float GenEcho::wrap(float in, float lb, float ub) {
  float out = in;
  if (in > ub) out = ub;
  else if (in < lb) out = lb;
  return out;
}

struct GenEchoWidget : ModuleWidget {
	GenEchoWidget(GenEcho *module) : ModuleWidget(module) {
		setPanel(SVG::load(assetPlugin(plugin, "res/GenEcho.svg")));
   
    //addParam(ParamWidget::create<CKD6>(Vec(51.210, 80.46), module, GenEcho::TRIG_PARAM, 0.0f, 1.0f, 0.0f));
   
    addParam(ParamWidget::create<RoundSmallBlackKnob>(Vec(9.883, 40.49), module, GenEcho::SLEN_PARAM, 0.01f, 1.f, 0.f));

    addParam(ParamWidget::create<RoundSmallBlackKnob>(Vec(9.883, 139.97), module, GenEcho::BPTS_PARAM, 0, 2200, 0.0));
    addParam(ParamWidget::create<RoundSmallBlackKnob>(Vec(55.883, 139.97), module, GenEcho::BPTSCV_PARAM, 0, 2200, 0.0));
    
    addParam(ParamWidget::create<RoundSmallBlackKnob>(Vec(9.883, 208.54), module, GenEcho::ASTP_PARAM, 0.0, 0.6, 0.9));
    addParam(ParamWidget::create<RoundSmallBlackKnob>(Vec(55.883, 208.54), module, GenEcho::ASTPCV_PARAM, 0.f, 1.f, 0.f));
    
    addParam(ParamWidget::create<RoundSmallBlackKnob>(Vec(9.883, 277.11), module, GenEcho::DSTP_PARAM, 0.0, 0.2, 0.9));
    addParam(ParamWidget::create<RoundSmallBlackKnob>(Vec(55.883, 277.11), module, GenEcho::DSTPCV_PARAM, 0.f, 1.f, 0.f));
    
    addParam(ParamWidget::create<RoundBlackSnapKnob>(Vec(7.883, 344.25), module, GenEcho::ENVS_PARAM, 1.f, 4.f, 4.f));

    //addParam(ParamWidget::create<CKD6>(Vec(110, 70), module, GenEcho::GATE_PARAM, 0.0f, 1.0f, 0.0f));

    addInput(Port::create<PJ301MPort>(Vec(10.281, 69.79), Port::INPUT, module, GenEcho::WAV0_INPUT));
    addInput(Port::create<PJ301MPort>(Vec(10.281, 95.54), Port::INPUT, module, GenEcho::GATE_INPUT));
    
    addInput(Port::create<PJ301MPort>(Vec(56.281, 95.54), Port::INPUT, module, GenEcho::RSET_INPUT));
    
    addInput(Port::create<PJ301MPort>(Vec(10.281, 169.01), Port::INPUT, module, GenEcho::BPTS_INPUT));
    addInput(Port::create<PJ301MPort>(Vec(10.281, 236.72), Port::INPUT, module, GenEcho::ASTP_INPUT));
    addInput(Port::create<PJ301MPort>(Vec(10.281, 306.00), Port::INPUT, module, GenEcho::DSTP_INPUT));

    addOutput(Port::create<PJ301MPort>(Vec(55.54, 345.97), Port::OUTPUT, module, GenEcho::SINE_OUTPUT));
  }
};


// Specify the Module and ModuleWidget subclass, human-readable
// author name for categorization per plugin, module slug (should never
// change), human-readable module name, and any number of tags
// (found in `include/tags.hpp`) separated by commas.
Model *modelGenEcho = Model::create<GenEcho, GenEchoWidget>("Gendy", "gene c h o", "g e n e cho", OSCILLATOR_TAG);
