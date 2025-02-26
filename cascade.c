//  cascade.c
//
//  Created by Joanna Huang on 2024/12/04.
//

#include<stdio.h>
#include<string.h>
#include<stdbool.h>
#include<stdint.h>

unsigned int little_endian_to_big(unsigned char *data) {
    return (data[3] << 24) | (data[2] << 16) | (data[1] << 8) | data[0] ;
}
void write_little_endian4(unsigned int data, FILE *file){
    fputc(data & 0xFF, file);
    fputc((data >> 8) & 0xFF, file);
    fputc((data >> 16) & 0xFF, file);
    fputc((data >> 24) & 0xFF, file);
}

int main(int argc, char *argv[]){
    //Usage: cascade scp output
    
    if (argc != 3){
        fprintf(stderr, "Invalid input parameters.\n");
    }
    char *list_file = argv[1];
    char *output_file = argv[2];

    FILE *fp_wavlist = fopen(list_file, "r");
    if (fp_wavlist == NULL) {
        fprintf(stderr, "Error opening %s\n", list_file);
        return 1;
    }
    
    char wavfile_name[50];
    bool first_wavfile = true;
    unsigned char wav_data_buffer[4];
    unsigned char wav_header_buffer[32];
    unsigned int subchunksize;
    unsigned int subchunk2size;
    size_t bytesRead;
    
    FILE *fp_cascaded = fopen(output_file, "wb");
    while ((fgets(wavfile_name, 50, fp_wavlist)) != NULL) {
        
        // 移除讀取的 wavfile_name 多餘換行符
        wavfile_name[strcspn(wavfile_name, "\n")] = '\0';
        FILE *fp_wavfile = fopen(wavfile_name, "rb");
        if (fp_wavfile == NULL){
            fprintf(stderr, "Error opening file %s\n", wavfile_name);
            return 1;
        }
        // write header of cascaded wav file
        if(first_wavfile){
            
            //RIFF
            fread(wav_data_buffer, 1, sizeof(wav_data_buffer), fp_wavfile);
            fwrite(wav_data_buffer, 1,  sizeof(wav_data_buffer), fp_cascaded);
            
            // 讀取 subchunksize, 解析, 更新 subchunksize
            fread(wav_data_buffer, 1,  sizeof(wav_data_buffer), fp_wavfile);
            subchunksize = little_endian_to_big(wav_data_buffer);
            subchunk2size = 40 * (subchunksize - 36);
            subchunksize = subchunk2size + 36;
            write_little_endian4(subchunksize, fp_cascaded);

            // Read and write RIFF-format ~ data-subchunk2size
            fread(wav_header_buffer, 1,  sizeof(wav_header_buffer), fp_wavfile);
            fwrite(wav_header_buffer, 1, sizeof(wav_header_buffer), fp_cascaded);
            write_little_endian4(subchunk2size, fp_cascaded);
            //skip original subcunk2size
            fseek(fp_wavfile, 4, SEEK_CUR);
            
            //handle audio data
            while((bytesRead = fread(wav_data_buffer, 1, sizeof(wav_data_buffer), fp_wavfile)) > 0 ){
                size_t bytesWritten = fwrite(wav_data_buffer, 1, bytesRead, fp_cascaded);
                if (bytesWritten != bytesRead){
                    fprintf(stderr, "Error writing %s audio to cascaded file.\n", wavfile_name);
                    return 1;
                }
            }
            first_wavfile = false;
        }
        else{
            // cascade audio data
            int seek_audio = fseek(fp_wavfile, 44, SEEK_SET);
            if(seek_audio != 0){
                fprintf(stderr, "Error finding audio data in %s\n", wavfile_name);
            }else{
                while((bytesRead = fread(wav_data_buffer, 1, sizeof(wav_data_buffer), fp_wavfile)) > 0 ){
                    size_t bytesWritten = fwrite(wav_data_buffer, 1, bytesRead, fp_cascaded);
                    if (bytesWritten != bytesRead){
                        fprintf(stderr, "Error writing %s audio to cascaded file.\n", wavfile_name);
                        return 1;
                    }
                }
            }
        }
        fclose(fp_wavfile);
    }
    fclose(fp_cascaded);
    fclose(fp_wavlist);
    return 0;
}


