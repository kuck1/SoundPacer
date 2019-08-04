#include <stdio.h>
#include "AudioFile/AudioFile.h"
#include <vector>
#import "resonance-audio/resonance_audio/graph/binaural_surround_renderer_impl.h"

AudioFile<double> audioFile;

#define INPUT_BUFFER_SIZE 1


using namespace std;
using namespace vraudio;

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

float * initSounds() {
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

    return 0;
}

int main(){

    // Sound * sounds = initSounds();


    BinauralSurroundRendererImpl * sound_renderer = new BinauralSurroundRendererImpl(1,1);

    // writeInBuffer(inBuffer);  
    // writeOutBuffer(outBuffer);


    return 0;
}
