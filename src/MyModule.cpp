#include "Gendy.hpp"

#include "GendyOscillator.hpp"

struct MyModule : Module {
	enum ParamIds {
		FREQ_PARAM,
    STEP_PARAM,
    BPTS_PARAM,
    GRAT_PARAM,
    TRIG_PARAM,
		ENVS_PARAM,
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

  int env_next = (int) params[ENVS_PARAM].value + 1;
  if (env != (EnvType) env_next) {
    debug("Switching to env type: %d", env_next);
    env = (EnvType) env_next;
    go.env.switchEnvType(env);
  }

  //if (phase >= 1.0) debug("PITCH PARAM: %f\n", (float) params[PITCH_PARAM].value);

  if (smpTrigger.process(params[TRIG_PARAM].value)) {
    
  }
  
  go.max_amp_step = rescale(params[STEP_PARAM].value, 0.0, 1.0, 0.05, 0.3);
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

    addParam(ParamWidget::create<CKSSThree>(Vec(110, 240), module, MyModule::ENVS_PARAM, 0.0f, 2.0f, 0.0f));
		
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
