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

  float *sample = NULL;
  unsigned int channels;
  unsigned int sampleRate;
  drwav_uint64 totalPCMFrameCount;
  
  State state = LOADING;
  unsigned int idx = 0;

  // spacing between breakpoints... in samples rn
  unsigned int bpt_spc = 150;
  unsigned int env_dur = bpt_spc / 2;

  // number of breakpoints - to be calculated according to size of
  // the sample
  unsigned int num_bpts = 0;

  float mAmps[MAX_BPTS] = {0.f};
  float mDurs[MAX_BPTS] = {0.f};
  float mOffs[MAX_BPTS] = {0.f};

  Wavetable env = Wavetable(1); 

  unsigned int index = 0;
  
  float max_amp_step = 0.05f;
  float amp = 0.f; 
  float amp_next = 0.f;
  float g_idx = 0.f; 
  float g_idx_next = 0.f;
  
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
  float deltaTime = engineGetSampleTime();
  float amp_out = 0.0;

  if (state == LOADING) {
    // reads in the sample file and stores in the sample float arry
    // TODO
    // implement simple file browser
    sample = drwav_open_file_and_read_pcm_frames_f32("/Users/bdds/projects/vcv/Rack/plugins/SGDSS/src/sfwrite.wav", &channels, &sampleRate, &totalPCMFrameCount);
    if (sample == NULL) {
      debug("ERROR OPENING FILE\n");
    }

    debug("totalPCMFrameCount: %d ; MAX_BPTS: %d ; RATIO for 150: %d", totalPCMFrameCount, MAX_BPTS, totalPCMFrameCount / bpt_spc);
    num_bpts = totalPCMFrameCount / bpt_spc;
    state = GENERATING;
  } else if (state==GENERATING) {
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
    sample[idx] += (amp * env.get(g_idx));
    amp_out = sample[idx];
    
    idx = (idx + 1) % (unsigned int) totalPCMFrameCount;
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
    
    addOutput(Port::create<PJ301MPort>(Vec(33, 275), Port::OUTPUT, module, GenEcho::SINE_OUTPUT));
	}
};


// Specify the Module and ModuleWidget subclass, human-readable
// author name for categorization per plugin, module slug (should never
// change), human-readable module name, and any number of tags
// (found in `include/tags.hpp`) separated by commas.
Model *modelGenEcho = Model::create<GenEcho, GenEchoWidget>("Gendy", "gene c h o", "g e n e cho", OSCILLATOR_TAG);
