#include <OVR_Audio.h>
#include <stdio.h>
#include "AudioFile/AudioFile.h"
#include <vector>

ovrAudioContext context;
AudioFile<double> audioFile;

#define INPUT_BUFFER_SIZE 50000
#define BUFFER_SIZE 100

using namespace std;


class Sound {
    public:

        float X, Y, Z;
        int RangeMin, RangeMax;
        float soundData[INPUT_BUFFER_SIZE];

         Sound(vector<vector<double>> sound){
            for(int i = 0; i < sound[0].size(); i ++){
                soundData[i] = sound[0][i];
            } 

            X = 0;
            Y = 0;
            Z = 0;
            RangeMin = 1;
            RangeMax = 100;
        }

        int GetSoundData(float * inbuffer, int sound_idx){
            for(int i = 0; i < BUFFER_SIZE; i ++){
                inbuffer[i] = soundData[(i + sound_idx)];
            }

            return 0;
        }

};

void setup()
{
    // Version checking is not strictly necessary but it's a good idea!
    int major, minor, patch;
    const char *VERSION_STRING;
     
    VERSION_STRING = ovrAudio_GetVersion( &major, &minor, &patch );
    printf( "Using OVRAudio: %s\n", VERSION_STRING );

    if ( major != OVR_AUDIO_MAJOR_VERSION || 
         minor != OVR_AUDIO_MINOR_VERSION )
    {
      printf( "Mismatched Audio SDK version!\n" );
    }

    ovrAudioContextConfiguration config = {};

    config.acc_Size = sizeof( config );
    // config.acc_Provider = ovrAudioSpatializationProvider_OVR_OculusHQ;
    config.acc_SampleRate = 44100;
    config.acc_BufferLength = 100; //INPUT_BUFFER_SIZE;
    config.acc_MaxNumSources = 16;

    ovrAudioContext context1;

    if ( ovrAudio_CreateContext( &context1, &config ) != ovrSuccess )
    {
      printf( "WARNING: Could not create context!\n" );
      return;
    }

    context = context1;
}


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

    audioFile.save("results/outbuffer.wav");
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

    audioFile.save("results/_inbuffer.wav");
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

void saveOutBuffer(float * temp, float * final, int global_idx){
    for (int save_idx = 0; save_idx < BUFFER_SIZE * 2; save_idx++){
        int final_idx = global_idx * 2 + save_idx;
        final[final_idx] = temp[save_idx];
    }
}

void saveInBuffer(float * temp, float * final, int global_idx){
    for (int save_idx = 0; save_idx < BUFFER_SIZE; save_idx++){
        int final_idx = global_idx + save_idx;
        final[final_idx] = temp[save_idx];
    }
}

// Applying 3D spatialiazation consists of looping over all of your sounds, 
// copying their data into intermediate buffers, and passing them to the 
// positional audio engine. It will in turn process the sounds with the 
// appropriate HRTFs and effects and return a floating point stereo buffer:
// \code
void processSounds( Sound *sounds, int NumSounds, float *SaveOutBuffer, float *SaveInBuffer, int process_idx)
{
    cout << "process_idx: " << process_idx << endl;

   // This assumes that all sounds want to be spatialized!
   // NOTE: In practice these should be 16-byte aligned, but for brevity
   // we're just showing them declared like this
   uint32_t Flags = 0, Status = 0;

   float outbuffer[ BUFFER_SIZE * 2 ];
   float inbuffer[ BUFFER_SIZE ];
   int p_idx = process_idx;

    float * mixBuffer1 = SaveInBuffer;
    float * mixBuffer2 = SaveOutBuffer;    

    float * inbuffer1 = inbuffer;
    float * outbuffer1 = outbuffer;

    int i = 0;
    // for ( int i = 0; i < NumSounds; i++ )
    // {
    // Set the sound's position in space (using OVR coordinates)
    // NOTE: if a pose state has been specified by a previous call to
    // ovrAudio_ListenerPoseStatef then it will be transformed 
    // by that as well

    ovrResult success1 = ovrAudio_SetAudioSourcePos( context, i, 
       sounds[ i ].X, sounds[ i ].Y, sounds[ i ].Z );

    // This sets the attenuation range from max volume to silent
    // NOTE: attenuation can be disabled or enabled
     ovrResult success2 = ovrAudio_SetAudioSourceRange( context, i, 
       sounds[ i ].RangeMin, sounds[ i ].RangeMax );

    // Grabs the next chunk of data from the sound, looping etc. 
    // as necessary.  This is application specific code.
    sounds[ i ].GetSoundData( inbuffer, process_idx );

    // Spatialize the sound into the output buffer.  Note that there
    // are two APIs, one for interleaved sample data and another for
    // separate left/right sample data
    ovrResult success3 = ovrAudio_SpatializeMonoSourceInterleaved( context, i, 
    Flags, &Status,
    outbuffer, inbuffer );

    SaveInBuffer = mixBuffer1;
    SaveOutBuffer = mixBuffer2;

    // saveBuffer(inbuffer, SaveInBuffer, idx);
    cout << "process_idx: " << process_idx << endl;
    cout << "p_idx: " << p_idx << endl;
    saveOutBuffer(outbuffer1, SaveOutBuffer, p_idx);
    saveInBuffer(inbuffer1, SaveInBuffer, p_idx);

}

Sound * initSounds() {
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

    Sound * sounds = new Sound(audioFile.samples);

    return sounds;

}

int main(){

    Sound * sounds = initSounds();

    setup();

    float * outBuffer = new float[INPUT_BUFFER_SIZE*2];
    float * inBuffer = new float[INPUT_BUFFER_SIZE];

    for (int main_idx = 0; main_idx < (INPUT_BUFFER_SIZE / BUFFER_SIZE); main_idx++){
        processSounds(sounds, 1, outBuffer, inBuffer, main_idx*BUFFER_SIZE);
    }

    writeInBuffer(inBuffer);  
    writeOutBuffer(outBuffer);


    return 0;
}
// At that point we have spatialized sound mixed into our output buffer.
