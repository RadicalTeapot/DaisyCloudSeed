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
enum Inputs {
    dryOut = 0,     // Dry / Wet
    earlyOut,       // Early reverb lvl
    mainOut,        // Late reverb lvl
    feedback,       // Late reverb feedback
    tapDecay,       // Early reverb dampening
    time,           // Late reverb decay
    INPUTS_COUNT,
};
float currentValues[INPUTS_COUNT],
      previousValues[INPUTS_COUNT];

struct GPIOPin {
    daisy::Pin pin;
    GPIO gpio;
    bool value;
    bool previous;
};
GPIOPin presetCycleButton;
GPIOPin bypassSwitch;
int currentPresetIndex;

const size_t blockSize = 48;
float ins[blockSize];
float outs[blockSize];

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

void readAdcValues()
{
    // TODO use a one pole filter to smooth those out (calling freq is sampleRate / blockSize)
    currentValues[dryOut] = hw.adc.GetFloat(dryOut);
    currentValues[earlyOut] = hw.adc.GetFloat(earlyOut);
    currentValues[mainOut] = hw.adc.GetFloat(mainOut);
    currentValues[feedback] = hw.adc.GetFloat(feedback);
    currentValues[tapDecay] = hw.adc.GetFloat(tapDecay);
    currentValues[time] = hw.adc.GetFloat(time);
}

void updateReverbParameters()
{
    readAdcValues();

    // TODO Code used to check if parameter changed from previous value before updating
    // reverb params, check if needed and if so use a threshold value to check if changed
    reverb->SetParameter(::Parameter::DryOut, currentValues[dryOut]);
    reverb->SetParameter(::Parameter::EarlyOut, currentValues[earlyOut]);
    reverb->SetParameter(::Parameter::MainOut, currentValues[mainOut]);
    reverb->SetParameter(::Parameter::LateDiffusionFeedback, currentValues[feedback]);
    reverb->SetParameter(::Parameter::TapDecay, currentValues[tapDecay]);
    reverb->SetParameter(::Parameter::LineDecay, currentValues[time]);
}

void updateSwitches()
{
    presetCycleButton.previous = presetCycleButton.value;
    presetCycleButton.value = presetCycleButton.gpio.Read();

    bypassSwitch.previous = bypassSwitch.value;
    bypassSwitch.value = bypassSwitch.gpio.Read();
}

// This runs at a fixed rate, to prepare audio samples
static void AudioCallback(AudioHandle::InputBuffer  in,
                          AudioHandle::OutputBuffer out,
                          size_t                    size)
{
    updateReverbParameters();
    updateSwitches();

    for (size_t i = 0; i < size; i++)
    {
        ins[i] = in[0][i];
    }

    // Cycle available models
    if(presetCycleButton.value && !presetCycleButton.previous) // Rising edge
    {
        cyclePreset();
    }

    if(!bypassSwitch.value) {
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

void AdcInit()
{
    AdcChannelConfig adcChannelConfig[INPUTS_COUNT];
    adcChannelConfig[dryOut].InitSingle(daisy::seed::A0);
    adcChannelConfig[earlyOut].InitSingle(daisy::seed::A1);
    adcChannelConfig[mainOut].InitSingle(daisy::seed::A2);
    adcChannelConfig[feedback].InitSingle(daisy::seed::A3);
    adcChannelConfig[tapDecay].InitSingle(daisy::seed::A4);
    adcChannelConfig[time].InitSingle(daisy::seed::A5);
    hw.adc.Init(adcChannelConfig, INPUTS_COUNT);
}

void GPIOInit()
{
    presetCycleButton.pin = daisy::seed::D29;
    presetCycleButton.gpio = GPIO();
    presetCycleButton.gpio.Init(presetCycleButton.pin, GPIO::Mode::INPUT, GPIO::Pull::PULLDOWN);

    bypassSwitch.pin = daisy::seed::D30;
    bypassSwitch.gpio = GPIO();
    bypassSwitch.gpio.Init(bypassSwitch.pin, GPIO::Mode::INPUT, GPIO::Pull::PULLDOWN);
}

int main(void)
{
    hw.Init();
    hw.SetAudioBlockSize(blockSize);
    float sampleRate = hw.AudioSampleRate();

    AudioLib::ValueTables::Init();
    CloudSeed::FastSin::Init();

    currentPresetIndex = 0;
    reverb = new CloudSeed::ReverbController(sampleRate);
    setPreset(currentPresetIndex);

    AdcInit();
    GPIOInit();

    hw.adc.Start();
    hw.StartAudio(AudioCallback);

    for (;;) {}
}
