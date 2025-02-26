//
//  spectrogram.c
//  
//
//  Created by Joanna Huang on 2024/12/6.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#define PI 3.14159

unsigned int little_endian_to_big(unsigned char *data) {
    return (data[3] << 24) | (data[2] << 16) | (data[1] << 8) | data[0] ;
}

int extract_fs_from_filename(const char *filename){
    // 尋找 "-" 之後的字元
    for(size_t index = 0; index < strlen(filename); index++){
        // 確認 fs 數值
        if (filename[index] == '-'){
            if(filename[index + 1] == '8'){
                return 8;
            }else if ((filename[index + 1] == '1') && (filename[index + 2] == '6')){
                return 16;
            }else{
                fprintf(stderr, "Invalid sample rate in file name %s.\n", filename);
                return -1;
            }
        }
    }
    // 在整個 filename 中都沒找到 “-“
    fprintf(stderr, "Can't find sample rate from %s.\n", filename);
    return -1;
    
}

double handle_hamming_w(short sample_data, int n , int P){
    double w = 0.54 - 0.46 * cos(2 * PI * n / (P - 1));
    return sample_data * w;
}

void spec_fqcy_mapping(double f, FILE *output_file, int N){
    fprintf(output_file, "time_index ");
    for(int k = 0; k < (N / 2); k++){
        double fk = k * f;
        fprintf(output_file, "%.2f  ", fk);
    }
    fprintf(output_file, "\n");
}

void compute_dft(const double *input, double *real, double *imag, double *magnitude, int N) {
    double epsilon = 1e-10;
    for (int k = 0; k < N; k++) {
        real[k] = 0.0;
        imag[k] = 0.0;
        // sigma
        for (int n = 0; n < N; n++) {
            real[k] += input[n] * cos(2.0 * PI * k * n / N);
            imag[k] -= input[n] * sin(2.0 * PI * k * n / N);
        }
        if (sqrt(real[k] * real[k] + imag[k] * imag[k]) == 0){
            magnitude[k] = 20 * log10(epsilon);     //避免log(0)
        }else
            magnitude[k] = 20 * log10(sqrt(real[k] * real[k] + imag[k] * imag[k]));
    }
}

void print_spec_2_file(FILE *output_file, double time_index, double *magnitude, int N){
    
    fprintf(output_file, "%.5f   ", time_index);
    for(int k = 0; k < (N / 2); k++){
        fprintf(output_file, "%.5f   ", magnitude[k]);
    }
    fprintf(output_file, "\n");
}

int main(int argc, char *argv[]){
    //Usage: spectrogram w_size w_type dft_size f_itv wav_in spec_out
    if (argc != 7){
        fprintf(stderr,"Invalid input parameters.\n");
        return 1;
    }
    
    unsigned int w_size = atoi(argv[1]);        // 32 30 (ms)
    char *w_type = argv[2];     // rectangular hamming
    unsigned int dft_size = atoi(argv[3]);      // 32 (ms)
    unsigned int f_itv = atoi(argv[4]);     // 10 (ms)
    char *wav_in = argv[5];
    char *spec_out = argv[6];
    
    //FILE *fp_log = fopen("log.txt", "w");
    FILE *fp_in = fopen(wav_in, "rb");
    if (fp_in == NULL){
        fprintf(stderr, "Error opening input file.\n");
        return 1;
    }
    
    
    // save wav file audio data
    unsigned int subchunk2size;
    unsigned char wav_data_buffer[4];
    size_t bytesread;
    int seek_inf = fseek(fp_in, 40, SEEK_SET);
    fread(wav_data_buffer, 1, 4, fp_in);
    subchunk2size = little_endian_to_big(wav_data_buffer);
    int num_samples = subchunk2size / 2;        // bit_depth = 16
    short *audio_buffer = malloc(num_samples * sizeof(short));
    if (audio_buffer == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        return 1;
    }
    
    for(int i = 0; i < num_samples; i++){
        bytesread = fread(&audio_buffer[i], sizeof(short), 1, fp_in);
        if (bytesread != 1){
            fprintf(stderr, "Error reading audio data from %s\n", wav_in);
            free(audio_buffer); // 釋放記憶體
            fclose(fp_in);
            return 1;
        }
    }
    
    // Framming and taking window (Zero padding if needed)
    int num_frames;         // 切割出的 analysis window 總數
    int samples_per_frame;      // analysis window 內樣本點數
    int fs;
    int cur_n;      // 每個 frame 樣本點起始位置
    fs = extract_fs_from_filename(wav_in);      // 8, 16
    int N = dft_size * fs;      // dft_window 內樣本點數
    int frame_shift =  f_itv * fs;     // 每個 analysis window 間的（樣本數）偏移量
    
    //double **x = NULL;
    if (fs == -1){
        return 1;
    }
    
    num_frames = num_samples / (fs * f_itv);     // window 數量 ＝ T / frame_interval
    samples_per_frame =  fs * w_size;        // 每個 window 有 fs*1000*w_size*0.001 個樣本點
    double **x = malloc(num_frames * sizeof(double *));
    for (int i = 0; i < num_frames; i++) {
        x[i] = malloc(N * sizeof(double));
    }
        
    // 遍歷每個 frames
    for(int w_index = 0; w_index < num_frames; w_index++){
        // 處理 window 內樣本資料, 若 w_size 小於 dft_size 作 zero paddding
        for(int s_index = 0; s_index < N; s_index++){
            cur_n = (w_index * frame_shift) + s_index;
            
            if ((s_index < samples_per_frame) && (cur_n < num_samples)){      // 最後一個 frame 樣本點可能不足
                // taking window (rectangular or hamming
                if (strcmp(w_type, "rectangular") == 0) {
                    x[w_index][s_index] = (double)audio_buffer[cur_n] * 1.0;
                } else if (strcmp(w_type, "hamming") == 0) {
                    x[w_index][s_index] = handle_hamming_w(audio_buffer[cur_n], s_index, samples_per_frame);
                } else {
                    return 1;
                }
            }
            else if ((s_index > samples_per_frame) || (cur_n > num_samples)){
                //zero padding
                x[w_index][s_index] = 0.0;
            }
            
        }
    }
    
    // DFT and Output spectrogram
    FILE *fp_out = fopen(spec_out, "w");
    if (fp_out == NULL){
        fprintf(stderr, "Error opening output file.\n");
        return 1;
    }
    double **X = malloc(num_frames * sizeof(double *));
    for (int i = 0; i < num_frames; i++) {
        X[i] = malloc(N * sizeof(double));
    }

    double real[N];
    double img[N];
    double time_index = 0.0;
    double frequency = 1000.0 / dft_size ;        // frequency = fs / N, fk = k * fs / N
    
    // 輸出 k 對應頻率至 spectrogram file      [正負頻率對稱：印出至 Nyquist frequency（sample_rate / 2）]
    spec_fqcy_mapping(frequency, fp_out, N);
    // 計算每個 dft_window 內 dft : 投影至 k domain 頻譜大小 ( X[m,k] )
    for(int m = 0; m < num_frames; m++){
        compute_dft(x[m], real, img, X[m], N);
        time_index = m * f_itv * 0.001;
        print_spec_2_file(fp_out, time_index, X[m], N);
    }
    
    // 釋放記憶體
    for (int i = 0; i < num_frames; i++) {
        free(x[i]);
        free(X[i]);
    }
    free(x);
    free(X);
    free(audio_buffer);
    
    //fclose(fp_log);
    fclose(fp_in);
    fclose(fp_out);
    return 0;
}
