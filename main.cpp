#include <OVR_Audio.h>
#include <stdio.h>
#include "AudioFile/AudioFile.h"
#include <vector>

ovrAudioContext context;
AudioFile<double> audioFile;

#define INPUT_BUFFER_SIZE 90000

using namespace std;


class Sound {
    public:

        float X, Y, Z;
        int RangeMin, RangeMax;
        float soundData[INPUT_BUFFER_SIZE];

         Sound(vector<vector<double>> sound){
            for(int i = 0; i < sound[0].size()
                ; i ++){
                soundData[i] = sound[0][i];
            } 

            X = 0;
            Y = 0;
            Z = 0;
            RangeMin = 1;
            RangeMax = 100;
        }

        int GetSoundData(float * inbuffer){
            for(int i = 0; i < INPUT_BUFFER_SIZE; i ++){
                inbuffer[i] = soundData[i];
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
    config.acc_BufferLength = 512;
    config.acc_MaxNumSources = 16;

    ovrAudioContext context1;

    if ( ovrAudio_CreateContext( &context1, &config ) != ovrSuccess )
    {
      printf( "WARNING: Could not create context!\n" );
      return;
    }

    context = context1;
}


void writeMixBuffer(float * buffer){

    vector<vector<double>> final;

    cout << "buffer addr: " << buffer << endl;

    final.resize(1);
    final[0].resize(INPUT_BUFFER_SIZE);

    for (int i = 0; i < INPUT_BUFFER_SIZE; i ++){
        final[0][i] = buffer[i];
    }

    audioFile.setAudioBuffer(final);
    audioFile.save("results/mixbuffer2.wav");
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

// Applying 3D spatialiazation consists of looping over all of your sounds, 
// copying their data into intermediate buffers, and passing them to the 
// positional audio engine. It will in turn process the sounds with the 
// appropriate HRTFs and effects and return a floating point stereo buffer:
// \code
void processSounds( Sound *sounds, int NumSounds, float *MixBuffer )
{
   // This assumes that all sounds want to be spatialized!
   // NOTE: In practice these should be 16-byte aligned, but for brevity
   // we're just showing them declared like this
   uint32_t Flags = 0, Status = 0;

   float outbuffer[ INPUT_BUFFER_SIZE * 2 ];
   float inbuffer[ INPUT_BUFFER_SIZE ];

    float * mixBuffer = MixBuffer;


   for ( int i = 0; i < NumSounds; i++ )
   {
      // Set the sound's position in space (using OVR coordinates)
      // NOTE: if a pose state has been specified by a previous call to
      // ovrAudio_ListenerPoseStatef then it will be transformed 
      // by that as well
      ovrAudio_SetAudioSourcePos( context, i, 
         sounds[ i ].X, sounds[ i ].Y, sounds[ i ].Z );

      // This sets the attenuation range from max volume to silent
      // NOTE: attenuation can be disabled or enabled
       ovrAudio_SetAudioSourceRange( context, i, 
         sounds[ i ].RangeMin, sounds[ i ].RangeMax );

      // Grabs the next chunk of data from the sound, looping etc. 
      // as necessary.  This is application specific code.
      sounds[ i ].GetSoundData( inbuffer );

        cout << "MixBuffer1 addr4: " << MixBuffer << endl;

      // Spatialize the sound into the output buffer.  Note that there
      // are two APIs, one for interleaved sample data and another for
      // separate left/right sample data
      ovrAudio_SpatializeMonoSourceInterleaved( context
        , 
          i, 
         Flags, &Status,
         outbuffer, inbuffer );
        cout << "MixBuffer addr5: " << MixBuffer << endl;
        cout << "mixBuffer  addr5: " << mixBuffer << endl;

       // compareInterleaved(outbuffer, inbuffer);

      // Do some mixing      
       for ( int j = 0; j < INPUT_BUFFER_SIZE; j++ ){
            // float out = 
            // mixBuffer[ j ] = outbuffer[ j ];
            // int x = 1;
         if ( i == 0 )
         {
            mixBuffer[ j ] = outbuffer[ j ];
         }
         else
         {
            mixBuffer[ j ] += outbuffer[ j ];
         }
       }  
   }

   // From here we'd send the MixBuffer on for more processing or
   // final device output

    writeMixBuffer(mixBuffer);    
}

int main(){
    


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

    setup();

    Sound * sound = new Sound(audioFile.samples);

    float * mixbuffer = new float[INPUT_BUFFER_SIZE];

    cout << "mixbuffer addr: " << mixbuffer << endl;

    processSounds(sound, INPUT_BUFFER_SIZE, mixbuffer);

    return 0;
}
// At that point we have spatialized sound mixed into our output buffer.
