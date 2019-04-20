#include "Gendy.hpp"

#include "GendyOscillator.hpp"

struct MyModule : Module {
	enum ParamIds {
		FREQ_PARAM,
    STEP_PARAM,
    DSTP_PARAM,
    BPTS_PARAM,
    GRAT_PARAM,
    TRIG_PARAM,
		ENVS_PARAM,
    NUM_PARAMS
	};
	enum InputIds {
		FREQ_INPUT,
    STEP_INPUT,
    DSTP_INPUT,
    BPTS_INPUT,
    GRAT_INPUT,
    ENVS_INPUT,
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

  EnvType env = (EnvType) 1;

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

  //int env_next = (int) params[ENVS_PARAM].value + 1;
  
  // snap knob for selecting envelope for the grain
  int env_num = (int) clamp(roundf(params[ENVS_PARAM].value), 1.0f, 8.0f);

  if (env != (EnvType) env_num) {
    debug("Switching to env type: %d", env_num);
    env = (EnvType) env_num;
    go.env.switchEnvType(env);
  }

  //if (phase >= 1.0) debug("PITCH PARAM: %f\n", (float) params[PITCH_PARAM].value);
  //TODO
  // add mirroring function to complement the wrap
  // and then add switch to change between the two
 
  // TODO
  // accept modulation of signal inputs for each parameter
  go.max_amp_step = rescale(params[STEP_PARAM].value, 0.0, 1.0, 0.05, 0.3);
  go.max_dur_step = rescale(params[DSTP_PARAM].value, 0.0, 1.0, 0.01, 0.3);
  go.freq_mul = rescale(params[FREQ_PARAM].value, -1.0, 1.0, 0.05, 4.0);
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

    //addParam(ParamWidget::create<CKSSThree>(Vec(110, 240), module, MyModule::ENVS_PARAM, 0.0f, 2.0f, 0.0f));
		
    addParam(ParamWidget::create<RoundHugeBlackKnob>(Vec(15, 68), module, MyModule::FREQ_PARAM, -1.0, 1.0, 0.0));
    addParam(ParamWidget::create<RoundLargeBlackKnob>(Vec(113.328, 62.95), module, MyModule::BPTS_PARAM, 3, MAX_BPTS, 0));
    
    addParam(ParamWidget::create<RoundLargeBlackKnob>(Vec(14, 177.16), module, MyModule::DSTP_PARAM, 0.0, 1.0, 0.9));
    addParam(ParamWidget::create<RoundLargeBlackKnob>(Vec(92, 177.16), module, MyModule::STEP_PARAM, 0.0, 1.0, 0.9));
    
    addParam(ParamWidget::create<RoundLargeBlackKnob>(Vec(124.781, 275.91), module, MyModule::GRAT_PARAM, 0.7, 1.3, 0.0));
    
    addParam(ParamWidget::create<RoundBlackSnapKnob>(Vec(12, 275.83), module, MyModule::ENVS_PARAM, 1.0f, 4.0f, 4.0f));
	
    // signal inputs
    addInput(Port::create<PJ301MPort>(Vec(79.022, 133.72), Port::INPUT, module, MyModule::FREQ_INPUT));
    addInput(Port::create<PJ301MPort>(Vec(121.022, 133.72), Port::INPUT, module, MyModule::BPTS_INPUT));
    
    addInput(Port::create<PJ301MPort>(Vec(59.187, 224.25), Port::INPUT, module, MyModule::STEP_INPUT));
		addInput(Port::create<PJ301MPort>(Vec(137.187, 224.25), Port::INPUT, module, MyModule::DSTP_INPUT));
    
    addInput(Port::create<PJ301MPort>(Vec(19.875, 340.42), Port::INPUT, module, MyModule::ENVS_INPUT));
		addInput(Port::create<PJ301MPort>(Vec(76.210, 285.33), Port::INPUT, module, MyModule::GRAT_INPUT));

    addOutput(Port::create<PJ301MPort>(Vec(134.003, 334.86), Port::OUTPUT, module, MyModule::SINE_OUTPUT));

		//addChild(ModuleLightWidget::create<MediumLight<RedLight>>(Vec(41, 59), module, MyModule::BLINK_LIGHT));
	}
};


// Specify the Module and ModuleWidget subclass, human-readable
// author name for categorization per plugin, module slug (should never
// change), human-readable module name, and any number of tags
// (found in `include/tags.hpp`) separated by commas.
Model *modelMyModule = Model::create<MyModule, MyModuleWidget>("Gendy", "MyModule", "My Module", OSCILLATOR_TAG);
