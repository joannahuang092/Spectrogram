#!/bin/bash

# Build and generate all 0.1 sec wave files
gcc -O3 -o sinegen  sinegen.c -lm
if [ $? -ne 0 ]; then
    echo "Error: sinegen.c compilation failed!"
    exit 1
fi

m=16
c=1
T=0.1
amplitude=(100.0 2000.0 1000.0 500.0 250.0 100.0 2000.0 1000.0 500.0 250.0)
frequency=(0.0 31.25 500.0 2000.0 4000.0 44.0 220.0 440.0 1760.0 3960.0)
wavetype=("sine" "sawtooth" "square" "triangle")

for fs in 8000 16000; do 
	w_count=0
    LIST_FILE="${fs}_list.txt"
    > "$LIST_FILE"  # 清空清單檔案
    
	for j in {0..3}; do
		for i in {0..9}; do 

			A=${amplitude[$i]}
			f=${frequency[$i]}
			w_type=${wavetype[$j]}
			w_count=$((w_count+1))

			CASC_OUTPUT_FILE="${fs}_${w_count}.wav"

			./sinegen $fs $m $c $w_type $f $A $T >"$CASC_OUTPUT_FILE" 2>log.txt

			# 每個生成的 0.1 秒音檔都加入 cascade 清單
			if [ $? -eq 0 ]; then
            	echo "$CASC_OUTPUT_FILE" >> "$LIST_FILE"
        	else
         	   echo "'$CASC_OUTPUT_FILE' generation failed."
        	fi
		done
	done
    echo "Generated all ${fs}Hz 0.1s WAV files and updated $LIST_FILE."
done
echo "-"

# Build and Cascade all 8k waves and all 16k waves
gcc -O3 -o cascade  cascade.c -lm
if [ $? -ne 0 ]; then
    echo "Error: cascade.c compilation failed!"
    exit 1
fi

./cascade 8000_list.txt s-8k.wav
if [ $? -eq 0 ]; then
    echo "Cascade to s-8k.wav successfully."
else
    echo "Cascade to s-8k.wav failed."
fi
./cascade 16000_list.txt s-16k.wav
if [ $? -eq 0 ]; then
    echo "Cascade to s-16k.wav successfully."
else
    echo "Cascade to s-16k.wav failed."
fi
echo "-"

# Build and generate 4 spectrogram for each WAV file, total:16
gcc -O3 -o spectrogram  spectrogram.c -lm
if [ $? -ne 0 ]; then
    echo "Error: spectrogram.c compilation failed!"
    exit 1
fi

window_size=(32 32 30 30)
window_type=(rectangular hamming rectangular hamming)
dft_size=32
f_itv=10

for wav_in in s-8k s-16k aeueo-8kHz aeueo-16kHz; do
    WAVEIN_FILE="${wav_in}.wav"
    for i in {0..3};do
        j=$((i+1))
        OUT_SPEC="${wav_in}.SET${j}.txt"
        w_size=${window_size[$i]}
        w_type=${window_type[$i]}
        
        ./spectrogram $w_size $w_type $dft_size $f_itv $WAVEIN_FILE $OUT_SPEC
        if [ $? -ne 0 ]; then
            echo "'$OUT_SPEC' saved failed"
        fi
    done
    echo "${wav_in}.{SET1 ~ SET4}.txt saved successfully."
done
echo "-"

# Save spectrogram according to each spec file, total:16
for in_wav in s-8k s-16k aeueo-8kHz aeueo-16kHz; do
    INWAV_FILE="${in_wav}.wav"
    for i in {0..3};do
        j=$((i+1))
        in_txt="${in_wav}.SET${j}.txt"
        out_pdf="${in_wav}.SET${j}.pdf"
        
        python3 spectshow.py $INWAV_FILE $in_txt $out_pdf
        if [ $? -ne 0 ]; then
            echo "'$out_pdf' saved failed"
        fi
    done
    echo "${in_wav}.{SET1 ~ SET4}.pdf saved successfully."
done

# All tasks completed
echo "----------------------------------------"
echo "All tasks completed successfully!"
