#include <stdio.h>
#include "AudioFile/AudioFile.h"
#include <vector>
#import "resonance-audio/resonance_audio/graph/binaural_surround_renderer_impl.h"
#import "resonance-audio/resonance_audio/graph/resonance_audio_api_impl.h"
#import "resonance-audio/platforms/common/room_properties.h"

AudioFile<double> audioFile;

#define INPUT_BUFFER_SIZE 50000


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
    final[0].resize(INPUT_BUFFER_SIZE);
    final[1].resize(INPUT_BUFFER_SIZE);


    for (int i = 0; i < INPUT_BUFFER_SIZE; i ++){
        int left = 2 * i;
        int right = left + 1;

        final[0][i] = buffer[left];
        final[1][i] = buffer[right];
    }

    audioFile.setAudioBuffer(final);
    audioFile.printSummary();

    audioFile.save("results/outbuffer_new_rendered.wav");
}

void writeInBuffer(float * buffer){

    vector<vector<double>> final;

    cout << "buffer addr: " << buffer << endl;

    final.resize(1);
    final[0].resize(INPUT_BUFFER_SIZE);

    for (int i = 0; i < INPUT_BUFFER_SIZE; i ++){
        final[0][i] = buffer[i];
    }

    audioFile.setAudioBuffer(final);
    audioFile.printSummary();

    audioFile.save("results/inbuffer_rendered.wav");
}


void compareInterleaved(float * outbuffer, float * inbuffer){
    for (int i = 0; i < INPUT_BUFFER_SIZE; i++){
        int left = i * 2;
        int right = left + 1;
        
        if (outbuffer[left] > .1){
            cout << i << endl;
            cout << "left: " << outbuffer[left] << endl;
            cout << "left - right: " << outbuffer[left] - outbuffer[right] << endl;
            cout << "left - in: " << outbuffer[left] - inbuffer[i] << endl;
        }
    }
}

void initSounds(float * input_float) {
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
        input_float[in_idx] = (float) input[0][in_idx];
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

int main(){
    float * input = new float[INPUT_BUFFER_SIZE];

    initSounds(input);

    // BinauralSurroundRendererImpl * sound_renderer = new BinauralSurroundRendererImpl(INPUT_BUFFER_SIZE,44100);
    // sound_renderer->AddInterleavedInput(input, 1, INPUT_BUFFER_SIZE);
    // sound_renderer->GetPlanarStereoOutput(&output, INPUT_BUFFER_SIZE);
    // sound_renderer->TriggerProcessing();


    // ResonanceAudioApiImpl * resonance_audio = new ResonanceAudioApiImpl(1, INPUT_BUFFER_SIZE, 44100);
    // resonance_audio->SetInterleavedBuffer(1, input, 1, INPUT_BUFFER_SIZE);

    ResonanceAudioSystem * resonance_audio = new ResonanceAudioSystem(44100, 1, INPUT_BUFFER_SIZE);

    SourceState* state;
    state->source_id = resonance_audio->api->CreateSoundObjectSource(RenderingMode::kBinauralHighQuality);

    // Updates distance model to ensure near field effects are only applied when
    // the minimum distance is below 1m. The +1.0f here ensures that max distance
    // is greater than min distance.
    // resonance_audio->api->SetSourceDistanceModel(state->source_id, DistanceRolloffModel::kNone, kNearFieldThreshold,
    // kNearFieldThreshold + 1.0f);

    // resonance_audio->api->SetSourcePosition( state->source_id, state->position.x(), state->position.y(), state->position.z());
    
    // resonance_audio->api->SetInterleavedBuffer(state->source_id, input, 1, INPUT_BUFFER_SIZE);

    // float * output = new float[INPUT_BUFFER_SIZE*2];

    // resonance_audio->api->FillInterleavedOutputBuffer(2, INPUT_BUFFER_SIZE*2, output);
    

    // writeInBuffer(input);  
    // writeOutBuffer(output);


    return 0;
}
