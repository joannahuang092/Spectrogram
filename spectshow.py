
# Usage: specshow.py in_wav in_txt out_pdf

import sys
import numpy as np
import matplotlib.pyplot as plt
from matplotlib import gridspec
from scipy.io import wavfile

def load_spectrogram(file_path):
    with open(file_path, 'r') as f:
        lines = f.readlines()

    # 第一行：讀取 k 對應的實際頻率
    freq_line = lines[0].strip().split()[1:]  # 去掉標籤 "time_index"
    frequencies = np.array([float(freq) for freq in freq_line])  # fk

    # 後續：時間與頻譜值
    time_indices = []
    spectrogram = []

    for line in lines[1:]:
        parts = line.strip().split()
        time_indices.append(float(parts[0]))  # 時間索引
        magnitudes = np.array([float(mag) for mag in parts[1:]])  # 頻譜值
        #magnitudes[magnitudes < 0] = 0  # 小於零dB的設為零
        spectrogram.append(magnitudes)

    return np.array(time_indices), frequencies, np.array(spectrogram)

# 讀入參數
if len(sys.argv) != 4:
    print("Usage: specshow.py in_wav in_txt out_pdf")
    sys.exit(1)
wav_file = sys.argv[1]
in_txt = sys.argv[2]
out_pdf = sys.argv[3]


# 讀入頻譜圖資料
time_indices, frequencies, spectrogram = load_spectrogram(in_txt)


# 讀入 WAV 檔波形資料
wav_rate, wav_data = wavfile.read(wav_file)

# 時間軸，對應 WAV 音頻的時間
wav_time = np.arange(len(wav_data)) / wav_rate

# 時間軸對齊
time_min = min(wav_time[0], time_indices[0])
time_max = max(wav_time[-1], time_indices[-1])

plt.figure(figsize=(10, 8))
# 使用 gridspec 調整佈局
gs = gridspec.GridSpec(2, 2, height_ratios=[1, 2], width_ratios=[20, 1])  # 調整高度比
gs.update(hspace=0.4)  # 調整子圖之間的垂直間距

# 上半部：波形圖
ax_waveform = plt.subplot(gs[0, 0])
ax_waveform.plot(wav_time, wav_data, color='blue')
ax_waveform.set_xlabel("Time (s)")
ax_waveform.set_ylabel("Amplitude")
ax_waveform.set_title("Waveform")
ax_waveform.set_xlim(time_min, time_max)  # 設定時間軸範圍
# 下半部：頻譜圖（黑白）
ax_spectrogram = plt.subplot(gs[1, 0])
cbar_ax = plt.subplot(gs[1, 1])  # 用於顏色條
mesh = ax_spectrogram.pcolormesh(time_indices, frequencies, spectrogram.T, shading='auto', cmap='gray')
plt.colorbar(mesh, cax=cbar_ax, label="Magnitude (dB)")
ax_spectrogram.set_xlabel("Time (s)")
ax_spectrogram.set_ylabel("Frequency (Hz)")
ax_spectrogram.set_title("Spectrogram (Grayscale)")
ax_spectrogram.set_xlim(time_min, time_max)  # 設定時間軸範圍

# 儲存成 pdf
plt.savefig(out_pdf)
