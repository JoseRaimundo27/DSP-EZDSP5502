import numpy as np
import scipy.io.wavfile as wav

# --- PARÂMETROS ---
ARQUIVO_ENTRADA = "original.wav"
ARQUIVO_SAIDA = "teste_pitch_shift.wav"

PITCH_RATIO = 0.6  # 0.8 = Voz Grossa, 0.5 = Oitava Abaixo
WINDOW_SIZE = 2000      # Tamanho do grão (em amostras)

def process_pitch_shift(input_file, output_file, ratio, win_size):
    fs, audio = wav.read(input_file)
    
    # Mono e Float
    if len(audio.shape) > 1: audio = audio[:,0]
    audio = audio.astype(np.float32) / 32768.0
    
    output = np.zeros_like(audio)
    buffer_len = win_size * 2 # Buffer de segurança
    buffer = np.zeros(buffer_len)
    wr_idx = 0
    
    phasor = 0.0
    # A velocidade que o ponteiro de leitura se afasta do de escrita
    phasor_step = (1.0 - ratio) / win_size
    
    print("Processando Pitch Shift...")
    
    for i, x in enumerate(audio):
        # Gravar no buffer circular
        buffer[wr_idx] = x
        
        # Calcular posições de leitura (Relativas ao wr_idx)
        # Ponteiro 1
        delay1 = phasor * win_size
        rd_idx1 = wr_idx - delay1
        
        # Ponteiro 2 (defasado 180 graus / 0.5)
        phasor2 = phasor + 0.5
        if phasor2 >= 1.0: phasor2 -= 1.0
        delay2 = phasor2 * win_size
        rd_idx2 = wr_idx - delay2
        
        # Wrap around indices
        rd_idx1 = rd_idx1 % buffer_len
        rd_idx2 = rd_idx2 % buffer_len
        
        # Ler amostras (Interpolacao simples - vizinho mais próximo para teste rápido)
        # Para qualidade igual ao C (Linear), seria mais codigo, mas vizinho serve para teste
        s1 = buffer[int(rd_idx1)]
        s2 = buffer[int(rd_idx2)]
        
        # Janela Triangular (Gain)
        if phasor < 0.5: gain1 = 2.0 * phasor
        else:            gain1 = 2.0 * (1.0 - phasor)
            
        if phasor2 < 0.5: gain2 = 2.0 * phasor2
        else:             gain2 = 2.0 * (1.0 - phasor2)
            
        # Mix
        output[i] = (s1 * gain1) + (s2 * gain2)
        
        # Atualizar ponteiros
        wr_idx = (wr_idx + 1) % buffer_len
        phasor += phasor_step
        if phasor >= 1.0: phasor -= 1.0
            
    # Salvar
    wav.write(output_file, fs, (output * 32767).astype(np.int16))
    print(f"Salvo: {output_file}")

# Executar
process_pitch_shift(ARQUIVO_ENTRADA, ARQUIVO_SAIDA, PITCH_RATIO, WINDOW_SIZE)