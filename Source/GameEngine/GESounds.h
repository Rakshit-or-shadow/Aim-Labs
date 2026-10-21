// GESounds.h
// Makes simple "beep" sound effects in code, so the aim trainer needs no sound files.
// Each sound is a sine wave whose pitch slides from StartFreq to EndFreq and fades out.
// (The weapon component and training manager have their own private tone code with
// different shapes - noise mix / decaying ping - so this helper is separate on purpose.)
#pragma once

#include "CoreMinimal.h"
#include "Sound/SoundWaveProcedural.h"
#include "Kismet/GameplayStatics.h"

namespace GESounds
{
	/** Plays one synthesized tone as a 2D sound. Does nothing when Volume <= 0. */
	inline void PlayTone(UObject* WorldContext, float StartFreq, float EndFreq, float Duration, float Volume)
	{
		if (WorldContext == nullptr || Volume <= 0.0f)
		{
			return; // muted
		}

		const int32 SampleRate = 44100; // samples per second
		const int32 NumSamples = FMath::Max(1, (int32)(Duration * SampleRate));

		// fill an array with 16-bit audio samples
		TArray<int16> Samples;
		Samples.SetNumUninitialized(NumSamples);

		float Phase = 0.0f;
		for (int32 i = 0; i < NumSamples; i++)
		{
			const float Progress = (float)i / (float)NumSamples;          // 0 -> 1
			const float Freq = FMath::Lerp(StartFreq, EndFreq, Progress); // slide the pitch
			Phase += 2.0f * PI * Freq / SampleRate;

			// envelope: quick fade-in (no click), then fade-out
			const float FadeIn = FMath::Min(1.0f, i / (0.004f * SampleRate));
			const float FadeOut = (1.0f - Progress) * (1.0f - Progress);
			const float Value = FMath::Sin(Phase) * FadeIn * FadeOut * 0.5f;

			Samples[i] = (int16)(Value * 32767.0f);
		}

		// put the samples into a sound wave Unreal can play
		USoundWaveProcedural* Wave = NewObject<USoundWaveProcedural>();
		Wave->SetSampleRate((uint32)SampleRate);
		Wave->NumChannels = 1;
		Wave->Duration = Duration;
		Wave->SoundGroup = SOUNDGROUP_Default;
		Wave->bLooping = false;
		Wave->QueueAudio((const uint8*)Samples.GetData(), Samples.Num() * sizeof(int16));

		UGameplayStatics::PlaySound2D(WorldContext, Wave, Volume);
	}

	// ready-made sounds used by the aim trainer (Volume = the saved Volume setting)
	inline void PlayHit(UObject* Ctx, float Volume)        { PlayTone(Ctx, 1500.0f, 900.0f, 0.07f, Volume); }
	inline void PlayMiss(UObject* Ctx, float Volume)       { PlayTone(Ctx, 260.0f, 180.0f, 0.08f, Volume * 0.6f); }
	inline void PlayClick(UObject* Ctx, float Volume)      { PlayTone(Ctx, 900.0f, 900.0f, 0.03f, Volume * 0.4f); }
	inline void PlayCountdown(UObject* Ctx, float Volume)  { PlayTone(Ctx, 700.0f, 700.0f, 0.09f, Volume * 0.7f); }
	inline void PlayRoundStart(UObject* Ctx, float Volume) { PlayTone(Ctx, 1000.0f, 1600.0f, 0.18f, Volume); }
	inline void PlayRoundEnd(UObject* Ctx, float Volume)   { PlayTone(Ctx, 900.0f, 400.0f, 0.30f, Volume); }
	inline void PlayNewBest(UObject* Ctx, float Volume)    { PlayTone(Ctx, 1000.0f, 2000.0f, 0.40f, Volume); }
}
