// Modified version of DaisyCloudSeed by Mathias Capdet.

#include "daisysp.h"
#include "daisy_seed.h"

#include "../../CloudSeed/Default.h"
#include "../../CloudSeed/ReverbController.h"
#include "../../CloudSeed/FastSin.h"
#include "../../CloudSeed/AudioLib/ValueTables.h"
#include "../../CloudSeed/AudioLib/MathDefs.h"

using namespace daisy;
using namespace daisysp;

DaisySeed hw;
// ::daisy::Parameter dryOut, earlyOut, mainOut, time, diffusion, tapDecay;
bool bypass;
int currentPresetIndex;
// Led led1, led2;

// Initialize "previous" p values
// float pdryout_value, pearlyout_value, pmainout_value, ptime_value, pdiffusion_value, pnumDelayLines, ptap_decay_value;

CloudSeed::ReverbController *reverb = 0;

// This is used in the modified CloudSeed code for allocating
// delay line memory to SDRAM (64MB available on Daisy)
#define CUSTOM_POOL_SIZE (48 * 1024 * 1024)
DSY_SDRAM_BSS char custom_pool[CUSTOM_POOL_SIZE];
size_t pool_index = 0;
int allocation_count = 0;
void *custom_pool_allocate(size_t size)
{
    if (pool_index + size >= CUSTOM_POOL_SIZE)
    {
        return 0;
    }
    void *ptr = &custom_pool[pool_index];
    pool_index += size;
    return ptr;
}

void setPreset(int index)
{
    reverb->ClearBuffers();

    if (index == 0)
    {
        reverb->initFactoryChorus();
    }
    else if (index == 1)
    {
        reverb->initFactoryDullEchos();
    }
    else if (index == 2)
    {
        reverb->initFactoryHyperplane();
    }
    else if (index == 3)
    {
        reverb->initFactoryMediumSpace();
    }
    else if (index == 4)
    {
        reverb->initFactoryNoiseInTheHallway();
    }
    else if (index == 5)
    {
        reverb->initFactoryRubiKaFields();
    }
    else if (index == 6)
    {
        reverb->initFactorySmallRoom();
    }
    else if (index == 7)
    {
        reverb->initFactory90sAreBack();
    }
}

void cyclePreset()
{
    currentPresetIndex = (currentPresetIndex + 1) % 8;
    setPreset(currentPresetIndex);
}

// This runs at a fixed rate, to prepare audio samples
static void AudioCallback(AudioHandle::InputBuffer  in,
                          AudioHandle::OutputBuffer out,
                          size_t                    size)
{
    // hw.ProcessAllControls();
    // led1.Update();
    // led2.Update();

    // float dryout_value = dryOut.Process();
    // float earlyout_value = earlyOut.Process();
    // float mainout_value = mainOut.Process();
    // float time_value = time.Process();
    // float diffusion_value = diffusion.Process();
    // float tap_decay_value = tapDecay.Process();

    // if ((pdryout_value < dryout_value) || ( pdryout_value> dryout_value))
    // {
    //   reverb->SetParameter(::Parameter::DryOut, dryout_value);
    //   pdryout_value = dryout_value;
    // }

    // if ((pearlyout_value < earlyout_value) || ( pearlyout_value> earlyout_value))
    // {
    //   reverb->SetParameter(::Parameter::EarlyOut, earlyout_value);
    //   pearlyout_value = earlyout_value;
    // }

    // if ((pmainout_value < mainout_value) || ( pmainout_value > mainout_value))
    // {
    //   reverb->SetParameter(::Parameter::MainOut, mainout_value);
    //   pmainout_value = mainout_value;
    // }

    // if ((ptime_value < time_value) || ( ptime_value > time_value))
    // {
    //   reverb->SetParameter(::Parameter::LineDecay, time_value);
    //   ptime_value = time_value;
    // }
    // if ((pdiffusion_value < diffusion_value) || ( pdiffusion_value > diffusion_value))
    // {
    //   reverb->SetParameter(::Parameter::LateDiffusionFeedback, diffusion_value);
    //   pdiffusion_value = diffusion_value;
    // }

    // if ((ptap_decay_value < tap_decay_value) || ( ptap_decay_value > tap_decay_value))
    // {
    //   reverb->SetParameter(::Parameter::TapDecay, tap_decay_value);
    //   ptap_decay_value = tap_decay_value;
    // }

    float ins[48];
    float outs[48];
    for (size_t i = 0; i < size; i++)
    {
        ins[i] = in[0][i];
    }

    // // (De-)Activate bypass and toggle LED when left footswitch is pressed
    // if(hw.switches[Terrarium::FOOTSWITCH_1].RisingEdge())
    // {
    //     bypass = !bypass;
    //     led1.Set(bypass ? 0.0f : 1.0f);
    // }

    // // Cycle available models
    // if(hw.switches[Terrarium::FOOTSWITCH_2].RisingEdge())
    // {
    //     cyclePreset();
    // }

    if(!bypass) {
        reverb->Process(ins, outs, 48);
        for (size_t i = 0; i < size; i++)
        {
            out[0][i] = outs[i] * 1.2;  // Slight overall volume boost at 1.2
        }
    } else {
        for (size_t i = 0; i < size; i++)
        {
            out[0][i] = in[0][i];
        }
    }
}

int main(void)
{
    hw.Init();
    //hw.SetAudioBlockSize(4);
    float sampleRate = hw.AudioSampleRate();
    currentPresetIndex = 0;

    AudioLib::ValueTables::Init();
    CloudSeed::FastSin::Init();

    bypass = false;
    currentPresetIndex = 0;

    reverb = new CloudSeed::ReverbController(sampleRate);
    setPreset(currentPresetIndex);

    // dryOut.Init(hw.knob[Terrarium::KNOB_1], 0.0f, 1.0f, ::daisy::Parameter::LINEAR);
    // earlyOut.Init(hw.knob[Terrarium::KNOB_2], 0.0f, 1.0f, ::daisy::Parameter::LINEAR);
    // mainOut.Init(hw.knob[Terrarium::KNOB_3], 0.0f, 1.0f, ::daisy::Parameter::LINEAR);
    // diffusion.Init(hw.knob[Terrarium::KNOB_4], 0.0f, 1.0f, ::daisy::Parameter::LINEAR);
    // tapDecay.Init(hw.knob[Terrarium::KNOB_5], 0.0f, 1.0f, ::daisy::Parameter::LINEAR);
    // time.Init(hw.knob[Terrarium::KNOB_6], 0.0f, 1.0f, ::daisy::Parameter::LINEAR);

    // pdryout_value = 0.0;
    // pearlyout_value = 0.0;
    // pmainout_value = 0.0;
    // ptime_value = 0.0;
    // pdiffusion_value = 0.0;
    // ptap_decay_value = 0.0;

    // Init the LEDs and set activate bypass
    // led1.Init(hw.GetPin(Terrarium::LED_1),false);
    // led1.Update();

    // led2.Init(hw.GetPin(Terrarium::LED_2),false);
    // led2.Update();

    // hw.adc.Start();
    hw.StartAudio(AudioCallback);
    for (;;)
    {
        // Do Stuff Infinitely Here
    }
}
