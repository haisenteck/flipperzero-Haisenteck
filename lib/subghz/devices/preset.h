#pragma once

/** Radio Presets */
typedef enum {
    FuriHalSubGhzPresetIDLE, /**< default configuration */
    FuriHalSubGhzPresetOok270Async, /**<AM270 OOK, bandwidth 270kHz, asynchronous */
    FuriHalSubGhzPresetOok650Async, /**<AM650 OOK, bandwidth 650kHz, asynchronous */
    FuriHalSubGhzPreset2FSKDev238Async, /**<FM238 FM, deviation 2.380371 kHz, asynchronous */
    FuriHalSubGhzPreset2FSKDev476Async, /**<FM476 FM, deviation 47.60742 kHz, asynchronous */
    FuriHalSubGhzPresetMSK99_97KbAsync, /**<PAGERS MSK, deviation 47.60742 kHz, 99.97Kb/s, asynchronous */
    FuriHalSubGhzPresetGFSK9_99KbAsync, /**<HND_1 GFSK, deviation 19.042969 kHz, 9.996Kb/s, asynchronous */
	FuriHalSubGhzPresetTPMS, /** TPMS*/
	FuriHalSubGhzPresetHONDA1, /** HONDA1*/
	FuriHalSubGhzPresetHONDA2, /** HONDA2*/
    FuriHalSubGhzPresetCustom, /**Custom Preset*/
} FuriHalSubGhzPreset;
