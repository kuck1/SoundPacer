#include <stdio.h>
#include "AudioFile/AudioFile.h"
#include <vector>
#import "resonance-audio/resonance_audio/graph/binaural_surround_renderer_impl.h"
#import "resonance-audio/resonance_audio/graph/resonance_audio_api_impl.h"
#import "resonance-audio/platforms/common/room_properties.h"

AudioFile<double> audioFile;

#define SOUND_SIZE 50000
#define INPUT_BUFFER_SIZE 5000


using namespace std;
using namespace vraudio;

// Stores the necessary components for the ResonanceAudio system.
struct ResonanceAudioSystem {
  ResonanceAudioSystem(int sample_rate, size_t num_channels,
                       size_t frames_per_buffer)
      : api(CreateResonanceAudioApi(num_channels, frames_per_buffer,
                                    sample_rate)) {}

  // VrAudio API instance to communicate with the internal system.
  std::unique_ptr<ResonanceAudioApi> api;

  // Current room properties of the environment.
  RoomProperties room_properties;
};

void writeOutBuffer(float * buffer){

    vector<vector<double>> final;

    cout << "buffer addr: " << buffer << endl;

    final.resize(2);
    final[0].resize(SOUND_SIZE);
    final[1].resize(SOUND_SIZE);


    for (int i = 0; i < SOUND_SIZE; i ++){
        int left = 2 * i;
        int right = left + 1;

        final[0][i] = buffer[left];
        final[1][i] = buffer[right];
    }

    audioFile.setAudioBuffer(final);
    audioFile.printSummary();

    audioFile.save("results/outbuffer_new_rendered111--z.wav");
}

void writeInBuffer(float * buffer){

    vector<vector<double>> final;

    cout << "buffer addr: " << buffer << endl;

    final.resize(1);
    final[0].resize(SOUND_SIZE);

    for (int i = 0; i < SOUND_SIZE; i ++){
        final[0][i] = buffer[i];
    }

    audioFile.setAudioBuffer(final);
    audioFile.printSummary();

    audioFile.save("results/inbuffer_rendered111.wav");
}


// void compareInterleaved(float * outbuffer, float * inbuffer){
//     for (int i = 0; i < INPUT_BUFFER_SIZE; i++){
//         int left = i * 2;
//         int right = left + 1;
        
//         if (outbuffer[left] > .1){
//             cout << i << endl;
//             cout << "left: " << outbuffer[left] << endl;
//             cout << "left - right: " << outbuffer[left] - outbuffer[right] << endl;
//             cout << "left - in: " << outbuffer[left] - inbuffer[i] << endl;
//         }
//     }
// }

void initSounds(float * input_float, int offset) {
    audioFile.load("./samples/Chirping-Birds.wav");

    int sampleRate = audioFile.getSampleRate();
    int bitDepth = audioFile.getBitDepth();

    int numSamples = audioFile.getNumSamplesPerChannel();
    double lengthInSeconds = audioFile.getLengthInSeconds();

    int numChannels = audioFile.getNumChannels();
    bool isMono = audioFile.isMono();
    bool isStereo = audioFile.isStereo();

    // or, just use this quick shortcut to print a summary to the console
    audioFile.printSummary();

    vector<vector<double>> input = audioFile.samples;

    
    for (int in_idx = 0; in_idx < INPUT_BUFFER_SIZE; in_idx++){
        // int left = in_idx * 2;
        // int right = left + 1;
        // input_float[left] = (float) input[0][in_idx];
        // input_float[right] = (float) input[1][in_idx];
        input_float[in_idx] = (float) input[0][in_idx + offset];
    }
}

// Represents the current state of a sound object source.
struct SourceState {
  // Id number for a sound object source.
  int source_id;

  // Source position.
  WorldPosition position;

  // Gain applied to a source (stored here in dB).
  float gain;

  // Spread of a source in degrees.
  float spread;

  // Intensity of the occlusion effect.
  float occlusion;

  // Roll off model.
  // FMOD_DSP_PAN_3D_ROLLOFF_TYPE model;

  // Current distance between source and listener.
  float distance;

  // Minimum distance for the distance rolloff effects.
  float min_distance;

  // Maximum distance for the distance rolloff effects.
  float max_distance;

  // Order of the source directivity pattern, conrols sharpness.
  float directivity_order;

  // Alpha value for the directivity pattern equation.
  float directivity_alpha;

  // Toggles room effects for the source.
  bool bypass_room;

  // Enables near field effects at distances less than 1m for the source.
  bool enable_near_field;

  // Near field effects gain.
  float near_field_gain;
};

void copyOutputToFull(float * output, float * full_output, int offset){
     int out_offset = 2 * offset;
     for (int out_idx = 0; out_idx < INPUT_BUFFER_SIZE * 2; out_idx++){
        full_output[out_idx + out_offset] = (float) output[out_idx];
    }
}

void copyInputToFull(float * input, float * full_input, int offset){
     for (int in_idx = 0; in_idx < INPUT_BUFFER_SIZE; in_idx++){
        full_input[in_idx + offset] = (float) input[in_idx];
    }
}

int main(){
    // float * input = new float[2*INPUT_BUFFER_SIZE];
    float * input = new float[INPUT_BUFFER_SIZE];
    float * output = new float[2* INPUT_BUFFER_SIZE];

    // BinauralSurroundRendererImpl * sound_renderer = new BinauralSurroundRendererImpl(INPUT_BUFFER_SIZE,44100);
    // sound_renderer->AddInterleavedInput(input, 1, INPUT_BUFFER_SIZE);
    // sound_renderer->GetInterleavedStereoOutput(&output, INPUT_BUFFER_SIZE);
    // sound_renderer->TriggerProcessing();


    // ResonanceAudioApiImpl * resonance_audio = new ResonanceAudioApiImpl(1, INPUT_BUFFER_SIZE, 44100);
    // resonance_audio->SetInterleavedBuffer(1, input, 1, INPUT_BUFFER_SIZE);

    ResonanceAudioSystem * resonance_audio = new ResonanceAudioSystem(44100, kNumStereoChannels, INPUT_BUFFER_SIZE);

    SourceState * state = new SourceState();
    
    WorldPosition kSourcePosition(0.0f, 0.0f, 0.0f);
    state->position = kSourcePosition;
    state->source_id = resonance_audio->api->CreateSoundObjectSource(RenderingMode::kBinauralHighQuality);

    float * full_output = new float[SOUND_SIZE * 2];
    float * full_input = new float[SOUND_SIZE];

    for (int main_idx = 0; main_idx < SOUND_SIZE; main_idx += INPUT_BUFFER_SIZE){
        
        cout << "main_idx -*- offset: " << main_idx << endl;

        initSounds(input, main_idx + 5000);

        resonance_audio->api->SetInterleavedBuffer(state->source_id, input, 1, INPUT_BUFFER_SIZE);
        // resonance_audio->api->SetInterleavedBuffer(state->source_id, input, kNumStereoChannels, INPUT_BUFFER_SIZE);

        // Updates distance model to ensure near field effects are only applied when
        // the minimum distance is below 1m. The +1.0f here ensures that max distance
        // is greater than min distance.
        resonance_audio->api->SetSourceDistanceModel(state->source_id, DistanceRolloffModel::kLinear, kNearFieldThreshold,
        kNearFieldThreshold + 1.0f);
        
        resonance_audio->api->SetSourcePosition( state->source_id, state->position.x(), state->position.y(), state->position.z());
        
        resonance_audio->api->FillInterleavedOutputBuffer(2, INPUT_BUFFER_SIZE, output);
        // writeOutBuffer(output);
        copyOutputToFull(output, full_output, main_idx);    
        copyInputToFull(input, full_input, main_idx);    
    }

    writeInBuffer(full_input);  
    writeOutBuffer(full_output);


    return 0;
}
