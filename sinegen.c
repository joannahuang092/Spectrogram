//  mini_prj_3_411086009.c
//
//  Created by Joanna Huang on 2024/10/22.
//
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#define PI 3.14159265358979323846264338327950

struct wav_file{
    char chunkID[4];          // "RIFF"
    unsigned int chunkSize;   // 36 + subchunk2Size
    char format[4];           // "WAVE"
    char subchunk1ID[4];      // "fmt "
    unsigned int subchunk1Size; // 16 for PCM
    unsigned short audioFormat; // 1 for PCM
    unsigned short numChannels; //
    unsigned int sampleRate;    // fs 8000/16000/22050/44100
    unsigned int byteRate;      // sampleRate * numChannels * sample_size/8
    unsigned short blockAlign;  // numChannels * sample_size/8
    unsigned short bitsPerSample; // sample_size 8/16/32
    char subchunk2ID[4];      // "data"
    unsigned int subchunk2Size; // T * fs * numChannels *  sample_size/8
}wav_header;

void wav_header_fill_in_fn(struct wav_file *wav_header, unsigned int fs, unsigned int sample_size, unsigned int channel, double T) {
    memcpy(wav_header->chunkID, "RIFF", 4);
    wav_header->chunkSize = 36 + (unsigned int) (T * fs * channel * (sample_size / 8));
    memcpy(wav_header->format, "WAVE", 4);
    memcpy(wav_header->subchunk1ID, "fmt ", 4);
    wav_header->subchunk1Size = 16;
    wav_header->audioFormat = 1;
    wav_header->numChannels = channel;
    wav_header->sampleRate = fs;
    wav_header->byteRate = fs * channel * (sample_size / 8);
    wav_header->blockAlign = channel * (sample_size / 8);
    wav_header->bitsPerSample = sample_size;
    memcpy(wav_header->subchunk2ID, "data", 4);
    wav_header->subchunk2Size = (unsigned int) (T * fs * channel * (sample_size / 8));
}
void write_little_endian4(unsigned int data, FILE *file){
    fputc(data & 0xFF, file);
    fputc((data >> 8) & 0xFF, file);
    fputc((data >> 16) & 0xFF, file);
    fputc((data >> 24) & 0xFF, file);
}
void write_little_endian2(unsigned short data, FILE *file){
    fputc(data & 0xFF, file);
    fputc((data >> 8) & 0xFF, file);
}
double generate_sawtooth_wave(double f, double t){
    return f * t - floor(f * t);
}
double generate_square_wave(double f, double t){
    if (sin(2 * PI * f * t) >= 0){
        return  1;
    }else{
        return  -1;
    }
}
double generate_triangle_wave(double f, double t){
    return 2 * fabs(2 * (f * t - floor(0.5 + f * t))) - 1;
}

int main(int argc, char *argv[]){
    
    unsigned int fs;
    unsigned int sample_size;
    unsigned int channel;
    char *wavetype;
    double f;
    double A;
    double T;
    
    if (argc != 8){
        fprintf(stderr, "Invalid input parameters.\n");
    }
    
    //arguments
    fs = (unsigned int) atoi(argv[1]) ;
    sample_size = (unsigned int)atoi(argv[2]);
    channel = (unsigned int)atoi(argv[3]);
    wavetype = argv[4];
    f = atof(argv[5]);
    A = atof(argv[6]);
    T = atof(argv[7]);
    
    //wav file header
    wav_header_fill_in_fn(&wav_header, fs, sample_size, channel, T);
    FILE *wav_file = stdout;
    if (!wav_file) {
        fprintf(stderr, "Error opening WAV file\n");
    }
    fwrite(&wav_header.chunkID, sizeof(wav_header.chunkID), 1, wav_file);
    write_little_endian4(wav_header.chunkSize, wav_file);
    fwrite(&wav_header.format, sizeof(wav_header.format), 1, wav_file);
    fwrite(&wav_header.subchunk1ID, sizeof(wav_header.subchunk1ID), 1, wav_file);
    write_little_endian4(wav_header.subchunk1Size, wav_file);
    write_little_endian2(wav_header.audioFormat, wav_file);
    write_little_endian2(wav_header.numChannels, wav_file);
    write_little_endian4(wav_header.sampleRate, wav_file);
    write_little_endian4(wav_header.byteRate, wav_file);
    write_little_endian2(wav_header.blockAlign, wav_file);
    write_little_endian2(wav_header.bitsPerSample, wav_file);
    fwrite(&wav_header.subchunk2ID, sizeof(wav_header.subchunk2ID), 1, wav_file);
    write_little_endian4(wav_header.subchunk2Size, wav_file);

    
    //sampling [range:A ~ -A]
    double t;      //sampling time
    int num_sample = (int)fs * T * 1;
    double sample_ch1[num_sample];
    double sample_ch2[num_sample];
    double Ts = 1.0 / (double)fs;      //sample period
    double phase_shift = PI;
    
    for(int i = 0; i < num_sample; i++){
        t = (double)i * Ts;
        //handle different wave types
        if (strcmp(wavetype, "sine") == 0){
            sample_ch1[i] = A * sin(2 * PI * f * t);
            if(channel == 2){
                sample_ch2[i] = A * sin(2 * PI * f * t + phase_shift);
            }
        }
        else if(strcmp(wavetype, "sawtooth") == 0){
            sample_ch1[i] = A * generate_sawtooth_wave(f, t);
            if(channel == 2){
                double adjusted_time = t + (phase_shift / (2.0 * PI * f));
                sample_ch2[i] = A * generate_sawtooth_wave(f, adjusted_time);
            }
        }
        else if (strcmp(wavetype, "square") == 0){
            sample_ch1[i] = A * generate_square_wave(f, t);
            if(channel == 2){
                double adjusted_time = t + (phase_shift / (2.0 * PI * f));
                sample_ch2[i] = A * generate_square_wave(f, adjusted_time);
            }
        }
        else if (strcmp(wavetype, "triangle") == 0){
            sample_ch1[i] = A * generate_triangle_wave(f, t);
            if(channel == 2){
                double adjusted_time = t + (phase_shift / (2.0 * PI * f));
                sample_ch2[i] = A * generate_triangle_wave(f, adjusted_time);
            }
        }
    }
    
    // Normalize sample data and output
    if(sample_size == 8){
        unsigned char buffer_8bit;
        double max_amplitude = 127.0;
        for (int i = 0; i < num_sample; i++) {
            buffer_8bit = (unsigned char)(sample_ch1[i] * max_amplitude / 2000.0);
            fwrite(& buffer_8bit, sizeof(unsigned char), 1, wav_file);
            if(channel == 2){
                //
            }
        }
    }else if (sample_size == 16){
        short buffer_16bit;
        double max_amplitude = 32767.0;
        for (int i = 0; i < num_sample; i++) {
            buffer_16bit = (short)(sample_ch1[i] * max_amplitude / 2000.0);
            fwrite(&buffer_16bit, sizeof(short), 1, wav_file);
            if(channel == 2){
                //
            }
        }
    }else if(sample_size == 32){
        long buffer_32bit;
        double max_amplitude = 2147483647.0;
        for (int i = 0; i < num_sample; i++) {
            buffer_32bit = (long)(sample_ch1[i] * max_amplitude / 2000.0);
            fwrite(&buffer_32bit, sizeof(long), 1, wav_file);
            if(channel == 2){
                //
            }
        }
    }else{
        fprintf(stderr, "Error: Invalid sample size.\n");
    }

    return 0;
}

