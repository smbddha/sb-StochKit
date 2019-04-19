#include "util/common.hpp"
#include "dsp/digital.hpp"

#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"

#include "Gendy.hpp"
#include "wavetable.hpp"

#define MAX_BPTS 4096 
#define MAX_SAMPLE_SIZE 1 << 21

struct GenEcho : Module {
	enum ParamIds {
    BSPC_PARAM,
		TRIG_PARAM,
    STEP_PARAM,
    NUM_PARAMS
	};
	enum InputIds {
		WAV0_INPUT,
		GATE_INPUT,
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

  float *sample = NULL;
  float *_sample = NULL;

  unsigned int channels;
  unsigned int sampleRate;
  drwav_uint64 totalPCMFrameCount;
 
  unsigned int sample_size = 0;

  State state = LOADING;
  unsigned int idx = 0;

  // spacing between breakpoints... in samples rn
  unsigned int bpt_spc = 1500;
  unsigned int env_dur = bpt_spc / 2;

  // number of breakpoints - to be calculated according to size of
  // the sample
  unsigned int num_bpts = 0;

  float mAmps[MAX_BPTS] = {0.f};
  float mDurs[MAX_BPTS] = {0.f};
  float mOffs[MAX_BPTS] = {0.f};

  Wavetable env = Wavetable(TRI); 

  unsigned int index = 0;
  
  float max_amp_step = 0.05f;
  float amp = 0.f; 
  float amp_next = 0.f;
  float g_idx = 0.f; 
  float g_idx_next = 0.5f;
 
  // when true read in from wav0_input and store in the sample buffer
  bool sampling = false;
  unsigned int s_i = 0;

  // For more advanced Module features, read Rack's engine.hpp header file
	// - toJson, fromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - onReset, onRandomize, onCreate, onDelete: implements special behavior when user clicks these from the context menu

  GenEcho() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {
  }
	
  void step() override;
  float wrap(float,float,float);
};

void GenEcho::step() {
	// Implement a simple sine oscillator
  //float deltaTime = engineGetSampleTime();
  float amp_out = 0.0;

  max_amp_step = params[STEP_PARAM].value;

  bpt_spc = (unsigned int) params[BSPC_PARAM].value;
  num_bpts = totalPCMFrameCount / bpt_spc;
  env_dur = bpt_spc / 2;

  // handle sample reset
  if (smpTrigger.process(params[TRIG_PARAM].value)) {
    for (unsigned int i=0; i<totalPCMFrameCount; i++) sample[i] = _sample[i];
    for (unsigned int i=0; i<MAX_BPTS; i++) mAmps[i] = 0.f;
  }

  // handle sample trigger through gate 
	if (gTrigger.process(inputs[GATE_INPUT].value)) {
    debug("TRIGGERED");
    sample_size = MAX_SAMPLE_SIZE; 
    sampling = true;
    index = 0;
    s_i = 0;
  }

  if (state == LOADING) {
    // reads in the sample file and stores in the sample float arry
    // TODO
    // implement simple file browser
    sample = drwav_open_file_and_read_pcm_frames_f32("/Users/bdds/projects/vcv/Rack/plugins/SGDSS/src/sfwrite.wav", &channels, &sampleRate, &totalPCMFrameCount);
    _sample = drwav_open_file_and_read_pcm_frames_f32("/Users/bdds/projects/vcv/Rack/plugins/SGDSS/src/sfwrite.wav", &channels, &sampleRate, &totalPCMFrameCount);

    if (sample == NULL) {
      debug("ERROR OPENING FILE\n");
    }

    debug("totalPCMFrameCount: %d ; MAX_BPTS: %d ; RATIO for 150: %d", totalPCMFrameCount, MAX_BPTS, totalPCMFrameCount / bpt_spc);
    sample_size = (unsigned int) totalPCMFrameCount;
    num_bpts = totalPCMFrameCount / bpt_spc;
    state = GENERATING;
  } else if (state==GENERATING) {
    //debug("-- PHASE: %f ; G_IDX: %f ; G_IDX_NEXT: %f", phase, g_idx, g_idx_next);
    
    // can be sampling but still output, just at a 1 sample delay
    // or will there even be a delay ??
    if (sampling) {
      if (s_i >= sample_size) {
        sampling = false;
      } else {
        sample[s_i] = inputs[WAV0_INPUT].value; 
        _sample[s_i] = sample[s_i];
      } 
    }
    
    if (phase >= 1.0) {
      phase -= 1.0;

      amp = amp_next;
      index = (index + 1) % num_bpts;

      /* adjust vals */
      mAmps[index] = wrap(mAmps[index] + (max_amp_step * randomNormal()), -1.0f, 1.0f); 
      amp_next = mAmps[index];

      /* step/adjust grain sample offsets */
      g_idx = g_idx_next;
      g_idx_next = 0.0;
    }

    // change amp in sample buffer
    sample[idx] = wrap(sample[idx] + (amp * env.get(g_idx)), -1.f, 1.f);
    amp_out = sample[idx];
  
    idx = (idx + 1) % sample_size;
    g_idx = fmod(g_idx + (1.f / (4.f * env_dur)), 1.f);
    g_idx_next = fmod(g_idx_next + (1.f / (4.f * env_dur)), 1.f);
    
    phase += 1.f / (float) bpt_spc;
  }

  outputs[SINE_OUTPUT].value = 5.0f * amp_out;
}

float GenEcho::wrap(float in, float lb, float ub) {
  float out = in;
  if (in > ub) out = ub;
  else if (in < lb) out = lb;
  return out;
}

struct GenEchoWidget : ModuleWidget {
	GenEchoWidget(GenEcho *module) : ModuleWidget(module) {
		setPanel(SVG::load(assetPlugin(plugin, "res/MyModule3.svg")));
    
    addParam(ParamWidget::create<CKD6>(Vec(40, 70), module, GenEcho::TRIG_PARAM, 0.0f, 1.0f, 0.0f));
    addParam(ParamWidget::create<Davies1900hBlackKnob>(Vec(40, 140), module, GenEcho::BSPC_PARAM, 800, 3000, 0.0));
    addParam(ParamWidget::create<Davies1900hBlackKnob>(Vec(110, 140), module, GenEcho::STEP_PARAM, 0.0, 0.6, 0.9));
   
    addInput(Port::create<PJ301MPort>(Vec(33, 245), Port::INPUT, module, GenEcho::WAV0_INPUT));

    addOutput(Port::create<PJ301MPort>(Vec(33, 275), Port::OUTPUT, module, GenEcho::SINE_OUTPUT));
	}
};


// Specify the Module and ModuleWidget subclass, human-readable
// author name for categorization per plugin, module slug (should never
// change), human-readable module name, and any number of tags
// (found in `include/tags.hpp`) separated by commas.
Model *modelGenEcho = Model::create<GenEcho, GenEchoWidget>("Gendy", "gene c h o", "g e n e cho", OSCILLATOR_TAG);
