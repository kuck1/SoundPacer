#include <stdio.h>
#include "AudioFile/AudioFile.h"
#include <vector>
#include <cstdlib>
#import "resonance-audio/resonance_audio/graph/binaural_surround_renderer_impl.h"
#import "resonance-audio/resonance_audio/graph/resonance_audio_api_impl.h"
#import "resonance-audio/platforms/common/room_properties.h"

AudioFile<double> audioFile;

#define SOUND_SIZE 200000
#define INPUT_BUFFER_SIZE 5000


using namespace std;
using namespace vraudio;

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

ResonanceAudioSystem * resonance_audio;
SourceState * state;
float * input;
float * output;
float * full_input;
float * full_output;



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

    audioFile.save("results/outbuffer_*.wav");
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

    audioFile.save("results/inbuffer_*.wav");
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
    audioFile.load("./samples/ballad_piano.wav");

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

void spatialize(int main_idx){
  
    cout << "main_idx -***- offset: " << main_idx << endl;

    initSounds(input, main_idx);

    resonance_audio->api->SetInterleavedBuffer(state->source_id, input, 1, INPUT_BUFFER_SIZE);
    // resonance_audio->api->SetInterleavedBuffer(state->source_id, input, kNumStereoChannels, INPUT_BUFFER_SIZE);

    // Updates distance model to ensure near field effects are only applied when
    // the minimum distance is below 1m. The +1.0f here ensures that max distance
    // is greater than min distance.
    resonance_audio->api->SetSourceDistanceModel(state->source_id, DistanceRolloffModel::kLinear, 10, 100);
    
    resonance_audio->api->SetSourcePosition( state->source_id, state->position.x(), state->position.y(), state->position.z());
    
    resonance_audio->api->FillInterleavedOutputBuffer(2, INPUT_BUFFER_SIZE, output);
    // writeOutBuffer(output);
    copyOutputToFull(output, full_output, main_idx);    
    copyInputToFull(input, full_input, main_idx);    

}


void spatialize_setup(){
    input = new float[INPUT_BUFFER_SIZE];
    output = new float[2* INPUT_BUFFER_SIZE];
    resonance_audio = new ResonanceAudioSystem(44100, kNumStereoChannels, INPUT_BUFFER_SIZE);

    state = new SourceState();
    
    WorldPosition kSourcePosition(0.0f, 0.0f, 0.0f);
    state->position = kSourcePosition;
    state->source_id = resonance_audio->api->CreateSoundObjectSource(RenderingMode::kBinauralHighQuality);

    full_output = new float[SOUND_SIZE * 2];
    full_input = new float[SOUND_SIZE];
}

void linearTest(WorldPosition start, WorldPosition end){
    spatialize_setup();

    WorldPosition curr;

    float x_inc = (end.x() - start.x()) / (SOUND_SIZE / INPUT_BUFFER_SIZE);
    float y_inc = (end.y() - start.y()) / (SOUND_SIZE / INPUT_BUFFER_SIZE);
    float z_inc = (end.z() - start.z()) / (SOUND_SIZE / INPUT_BUFFER_SIZE);

    for (int main_idx = 0; main_idx < SOUND_SIZE; main_idx += INPUT_BUFFER_SIZE){

        float curr_x = start.x() + x_inc * (main_idx / INPUT_BUFFER_SIZE);
        float curr_y = start.y() + y_inc * (main_idx / INPUT_BUFFER_SIZE);
        float curr_z = start.z() + z_inc * (main_idx / INPUT_BUFFER_SIZE);

        curr = WorldPosition(curr_x, curr_y, curr_z);
        state->position = curr;
        spatialize(main_idx);
    }

    writeInBuffer(full_input);  
    writeOutBuffer(full_output);
}

bool pastNext(WorldPosition curr, WorldPosition start, WorldPosition end){
    if (abs(end.x() - start.x()) <= abs(curr.x() - start.x()) &&
        abs(end.y() - start.y()) <= abs(curr.y() - start.y()) &&
        abs(end.z() - start.z()) <= abs(curr.z() - start.z())){
        return true;
    }
    return false;
}

void multiPointTest(WorldPosition * position, int num_intervals){
    spatialize_setup();

    WorldPosition start, end, curr;

    start = position[0];
    end = position[1];

    float x_inc = ((end.x() - start.x()) / (SOUND_SIZE / INPUT_BUFFER_SIZE)) * num_intervals;
    float y_inc = ((end.y() - start.y()) / (SOUND_SIZE / INPUT_BUFFER_SIZE)) * num_intervals;
    float z_inc = ((end.z() - start.z()) / (SOUND_SIZE / INPUT_BUFFER_SIZE)) * num_intervals;

    int next_position_idx = 2;
    int sound_position_idx = 0;
    int last_updated = 0;

    for (int main_idx = 0; main_idx < SOUND_SIZE; main_idx += INPUT_BUFFER_SIZE){
        if(pastNext(curr,start,end) && next_position_idx != num_intervals + 1){
            cout << "**** updating start and end" << endl;
            start = end;
            end = position[next_position_idx];
            next_position_idx ++;

            cout << "****end_x: " << end.x() << endl;
            cout << "****end_y: " << end.y() << endl;
            cout << "****end_z: " << end.z() << endl;
            x_inc = ((end.x() - start.x()) / (SOUND_SIZE / INPUT_BUFFER_SIZE)) * num_intervals;
            y_inc = ((end.y() - start.y()) / (SOUND_SIZE / INPUT_BUFFER_SIZE)) * num_intervals;
            z_inc = ((end.z() - start.z()) / (SOUND_SIZE / INPUT_BUFFER_SIZE)) * num_intervals;
            sound_position_idx = 0;
            last_updated = 0;
        }

        float curr_x = start.x() + x_inc * (sound_position_idx / INPUT_BUFFER_SIZE);
        float curr_y = start.y() + y_inc * (sound_position_idx / INPUT_BUFFER_SIZE);
        float curr_z = start.z() + z_inc * (sound_position_idx / INPUT_BUFFER_SIZE);

        cout << "****curr_x: " << curr_x << endl;
        cout << "****curr_y: " << curr_y << endl;
        cout << "****curr_z: " << curr_z << endl;

        curr = WorldPosition(curr_x, curr_y, curr_z);
        state->position = curr;
        spatialize(main_idx);
        last_updated ++;
        sound_position_idx += INPUT_BUFFER_SIZE;
    }

    writeInBuffer(full_input);  
    writeOutBuffer(full_output);
}

int main(){
    WorldPosition sourceStart(5.0f,-1.0f, 60.0f);
    WorldPosition sourceMid1(5.0f, -1.0f, -1.0f);
    WorldPosition sourceMid2(1.5f, -1.0f, -1.0f);
    WorldPosition sourceMid3(0.0f, -1.0f, -1.0f);
    WorldPosition sourceMid4(-1.5f, -1.0f, -1.0f);
    WorldPosition sourceMid5(-5.0f, -1.0f, -1.0f);
    WorldPosition sourceEnd(-5.0f, -1.0f, -60.0f);

    WorldPosition positions[] = {sourceStart,sourceMid1,sourceMid2,sourceMid3,sourceMid4,sourceMid5,sourceEnd};
    int num_intervals = 6;
    
    WorldPosition linearStart(1.5f, -1.0f, -1.0f);
    WorldPosition linearEnd(-1.5f, -1.0f, -1.0f);

    // linearTest(linearStart, linearEnd);
    multiPointTest(positions, num_intervals);

    return 0;
}


