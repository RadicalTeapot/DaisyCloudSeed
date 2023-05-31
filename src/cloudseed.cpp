// Modified version of DaisyCloudSeed by Mathias Capdet.

#include "daisysp.h"
#include "daisy_seed.h"

#include "../../CloudSeed/Default.h"
#include "../../CloudSeed/ReverbController.h"
#include "../../CloudSeed/FastSin.h"
#include "../../CloudSeed/AudioLib/ValueTables.h"
#include "../../CloudSeed/AudioLib/MathDefs.h"

// #define PERFORMANCE_MONITOR 1

using namespace daisy;
using namespace daisysp;

DaisySeed hw;
enum class Inputs: int {
    dryOut = 0,     // Dry / Wet
    earlyOut,       // Early reverb lvl
    mainOut,        // Late reverb lvl
    feedback,       // Late reverb feedback
    tapDecay,       // Early reverb dampening
    time,           // Late reverb decay
    Count,
};
float currentValues[(int)Inputs::Count],
      previousValues[(int)Inputs::Count];

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
float ins[blockSize*2];
float outs[blockSize*2];

#ifdef PERFORMANCE_MONITOR
CpuLoadMeter loadMeter;
#endif

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
    currentValues[(int)Inputs::dryOut] = hw.adc.GetFloat((int)Inputs::dryOut);
    currentValues[(int)Inputs::earlyOut] = hw.adc.GetFloat((int)Inputs::earlyOut);
    currentValues[(int)Inputs::mainOut] = hw.adc.GetFloat((int)Inputs::mainOut);
    currentValues[(int)Inputs::feedback] = hw.adc.GetFloat((int)Inputs::feedback);
    currentValues[(int)Inputs::tapDecay] = hw.adc.GetFloat((int)Inputs::tapDecay);
    currentValues[(int)Inputs::time] = hw.adc.GetFloat((int)Inputs::time);
}

void updateReverbParameters()
{
    readAdcValues();

    // TODO Code used to check if parameter changed from previous value before updating
    // reverb params, check if needed and if so use a threshold value to check if changed
    reverb->SetParameter(::Parameter::DryOut, currentValues[(int)Inputs::dryOut]);
    reverb->SetParameter(::Parameter::EarlyOut, currentValues[(int)Inputs::earlyOut]);
    reverb->SetParameter(::Parameter::MainOut, currentValues[(int)Inputs::mainOut]);
    reverb->SetParameter(::Parameter::LateDiffusionFeedback, currentValues[(int)Inputs::feedback]);
    reverb->SetParameter(::Parameter::TapDecay, currentValues[(int)Inputs::tapDecay]);
    reverb->SetParameter(::Parameter::LineDecay, currentValues[(int)Inputs::time]);
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
    #ifdef PERFORMANCE_MONITOR
    loadMeter.OnBlockStart();
    #endif

    updateReverbParameters();
    updateSwitches();

    for (size_t i = 0; i < size; i++)
    {
        ins[i*2] = in[0][i];
        ins[i*2+1] = in[0][i];
    }

    // Cycle available models
    if(presetCycleButton.value && !presetCycleButton.previous) // Rising edge
    {
        cyclePreset();
    }

    if(!bypassSwitch.value) {
        reverb->Process(ins, outs);
        for (size_t i = 0; i < size; i++)
        {
            out[0][i] = outs[i*2] * 1.2;  // Slight overall volume boost at 1.2
            out[1][i] = outs[i*2+1] * 1.2;  // Slight overall volume boost at 1.2
        }
    } else {
        for (size_t i = 0; i < size; i++)
        {
            out[0][i] = in[0][i];
            out[1][i] = in[0][i];
        }
    }

#ifdef PERFORMANCE_MONITOR
    loadMeter.OnBlockEnd();
#endif
}

void AdcInit()
{
    AdcChannelConfig adcChannelConfig[(int)Inputs::Count];
    adcChannelConfig[(int)Inputs::dryOut].InitSingle(daisy::seed::A0);
    adcChannelConfig[(int)Inputs::earlyOut].InitSingle(daisy::seed::A1);
    adcChannelConfig[(int)Inputs::mainOut].InitSingle(daisy::seed::A2);
    adcChannelConfig[(int)Inputs::feedback].InitSingle(daisy::seed::A3);
    adcChannelConfig[(int)Inputs::tapDecay].InitSingle(daisy::seed::A4);
    adcChannelConfig[(int)Inputs::time].InitSingle(daisy::seed::A5);
    hw.adc.Init(adcChannelConfig, (int)Inputs::Count);
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

    #ifdef PERFORMANCE_MONITOR
    hw.StartLog();
    loadMeter.Init(sampleRate, blockSize);
    #endif

    AudioLib::ValueTables::Init();
    CloudSeed::FastSin::Init();

    currentPresetIndex = 0;
    reverb = new CloudSeed::ReverbController(sampleRate, CloudSeed::StereoMode::Stereo);
    setPreset(currentPresetIndex);

    AdcInit();
    GPIOInit();

    hw.adc.Start();
    hw.StartAudio(AudioCallback);

    for (;;) {
        #ifdef PERFORMANCE_MONITOR
        // get the current load (smoothed value and peak values)
        const float avgLoad = loadMeter.GetAvgCpuLoad();
        const float maxLoad = loadMeter.GetMaxCpuLoad();
        const float minLoad = loadMeter.GetMinCpuLoad();
        // print it to the serial connection (as percentages)
        hw.PrintLine("Processing Load %:");
        hw.PrintLine("Max: " FLT_FMT3, FLT_VAR3(maxLoad * 100.0f));
        hw.PrintLine("Avg: " FLT_FMT3, FLT_VAR3(avgLoad * 100.0f));
        hw.PrintLine("Min: " FLT_FMT3, FLT_VAR3(minLoad * 100.0f));
        // don't spam the serial connection too much
        System::Delay(500);
        #endif
    }
}
