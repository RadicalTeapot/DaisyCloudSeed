# DaisyCloudSeed
Cloud Seed is an open source algorithmic reverb plugin under the MIT license, which can be found at [ValdemarOrn/CloudSeed](https://github.com/ValdemarOrn/CloudSeed).
DaisyCloudSeed is a port to the Daisy seed platform with the goal to be used as a module for [AE modular systems](https://www.tangiblewaves.com/).

This repo also includes a modified version of CloudyReverb. It is a lighter reverb than CloudSeed (in terms of memory/processing requirements)

## Getting started
Build the libraries, source code and upload to the Daisy seed (after pressing on the boot and reset buttons):
```
./rebuild_all.sh && ./upload.sh
```
Or run the (default) task in VSCode `Build and program DFU`

## Notes
The number of delay lines is higher in Mono mode than in Stereo mode (5 in Mono vs 2 per channel in Stereo) to keep required processing within the CPU limits. 

# Control

| Control | Description | Comment |
| --- | --- | --- |
| Ctrl 1 | Dry Level | Adjusts the Dry level out |
| Ctrl 2 | Early Reverberation Level | Adjusts the Early Reverb stage output.  |
| Ctrl 3 | Late Reverberation Level | Adjusts the Late Reverb stage output |
| Ctrl 4 | Late Reverberation Feedback | Adjusts amount of signal fed back through the delay line. |
| Ctrl 5 | Early Reverberation Dampening | Controls amount of dampening for the early reverb stage. Actual parameter name is "TapDecay" |
| Ctrl 6 | Late Reverberation Decay | Adjust the decay time of the late reverberation stage. |
| Button 1 | Bypass / Active | Bypass / effect engaged |
| Button 2 | Cycle Preset | Loads the next available Preset, starts at beginning after the last in the list. |
| Switch 1 | Mono / Stereo mode | Output same channel to both Audio Out 1 and 2 in Mono mode, or the left / right channels in Stereo mode |
| Audio In 1 | Audio input | Mono only for now |
| Audio Out 1 | Mix Out | Mono or Left channel |
| Audio Out 2 | Mix Out | Mono or Right channel |
