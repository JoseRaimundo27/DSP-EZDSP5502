import numpy as np
import scipy.io.wavfile as wav
import math
import os

# ==============================================================================
# 🎛️ CONFIGURAÇÃO
# ==============================================================================

ARQUIVO_ENTRADA = "original.wav"
NOTA_ORIGINAL = "D4" 

# Lista de notas que você quer gerar
# (Nome do Arquivo, Nota Alvo, Oitava Alvo)
# Obs: Fb (Fá Bemol) é enarmonicamente igual a E (Mi)
ALVOS = [
    {"nome": "A",  "nota": "A",  "oitava": 3}, # Lá mais grave (-5 semitons)
    {"nome": "Bb", "nota": "Bb", "oitava": 3}, # Si Bemol grave (-4 semitons)
    {"nome": "Db", "nota": "Db", "oitava": 4}, # Ré Bemol (vizinho, -1 semitom)
    {"nome": "Fb", "nota": "E",  "oitava": 4}, # Fb é Mi (+2 semitons)
]

# Parâmetros do Reverb (Cauda longa que você ajustou)
C_LENS = [1687, 1601, 2053, 2251]
#C_GAINS = [0.8340, 0.9283, 0.9433, 0.7350] 
C_GAINS = [0.6340, 0.7283, 0.7433, 0.7350] 
AP_GAIN = 0.7
ENABLE_LPF = True # Filtro para amaciar a voz

# ==============================================================================
# 🧠 LÓGICA MUSICAL
# ==============================================================================

def nota_para_midi(nota, oitava):
    notas = {
        "C": 0, "Do": 0,
        "C#": 1, "Db": 1,
        "D": 2, "Re": 2,
        "D#": 3, "Eb": 3,
        "E": 4, "Mi": 4, "Fb": 4,
        "F": 5, "Fa": 5, "E#": 5,
        "F#": 6, "Gb": 6,
        "G": 7, "Sol": 7,
        "G#": 8, "Ab": 8,
        "A": 9, "La": 9,
        "A#": 10, "Bb": 10,
        "B": 11, "Si": 11, "Cb": 11
    }
    return notas[nota] + ((oitava + 1) * 12)

def calcular_ratio(origem_str, destino_nota, destino_oit):
    # Parse da origem (Ex: "D4")
    nota_orig = origem_str[:-1]
    oit_orig = int(origem_str[-1])
    
    midi_origem = nota_para_midi(nota_orig, oit_orig)
    midi_destino = nota_para_midi(destino_nota, destino_oit)
    
    semitons = midi_destino - midi_origem
    
    # Ratio de velocidade de reprodução
    # Se subir tom (+), toca mais rápido (ratio > 1)
    # Se descer tom (-), toca mais devagar (ratio < 1)
    # MAS... no algoritmo de delay:
    # Ratio < 1.0 (Delay encurta) = Pitch Sobe
    # Ratio > 1.0 (Delay estica) = Pitch Desce
    
    ratio_freq = math.pow(2, semitons / 12.0)
    ratio_delay = 1.0 / ratio_freq
    
    return ratio_delay, semitons

# ==============================================================================
# ⚙️ PROCESSAMENTO DSP
# ==============================================================================

class ProcessadorAudio:
    def __init__(self, fs):
        self.fs = fs

    def pitch_shift(self, audio, ratio, window_size=2048):
        # Proteção anti-aliasing se for Pitch UP (Ratio Delay < 1.0)
        audio_in = audio.copy()
        if ratio < 0.95: 
             # Filtro simples (média móvel) para cortar agudos antes de subir
             audio_in[1:] = (audio_in[1:] + audio_in[:-1]) * 0.5

        output = np.zeros_like(audio_in)
        buffer_len = window_size * 2
        buffer = np.zeros(buffer_len)
        wr_idx = 0
        phasor = 0.0
        phasor_step = (1.0 - ratio) / window_size

        for i, x in enumerate(audio_in):
            buffer[wr_idx] = x
            
            # Delay Variável
            delay1 = phasor * window_size
            rd1 = (wr_idx - delay1) % buffer_len
            
            phasor2 = phasor + 0.5
            if phasor2 >= 1.0: phasor2 -= 1.0
            delay2 = phasor2 * window_size
            rd2 = (wr_idx - delay2) % buffer_len
            
            # Interpolação
            s1 = buffer[int(rd1)]
            s2 = buffer[int(rd2)]
            
            # Janela
            gain1 = 2.0 * phasor if phasor < 0.5 else 2.0 * (1.0 - phasor)
            gain2 = 2.0 * phasor2 if phasor2 < 0.5 else 2.0 * (1.0 - phasor2)
            
            output[i] = (s1 * gain1) + (s2 * gain2)
            
            wr_idx = (wr_idx + 1) % buffer_len
            phasor += phasor_step
            if phasor >= 1.0: phasor -= 1.0
            if phasor < 0.0: phasor += 1.0 # Correção para Pitch Up
            
        return output

    def reverb(self, audio, c_lens, c_gains, ap_gain, mix = 0.2):
        output = np.zeros_like(audio)
        combs = [np.zeros(l) for l in c_lens]
        aps = [np.zeros(l) for l in [225, 341]]
        idx_c = [0]*4
        idx_ap = [0]*2
        
        lpf_state = 0.0
        lpf_alpha = 0.45 if ENABLE_LPF else 1.0

        for i, x in enumerate(audio):
            # Pré-filtro (Amaciar voz)
            lpf_state += lpf_alpha * (x - lpf_state)
            input_val = lpf_state * 0.25
          
            # Combs
            sum_combs = 0.0
            for k in range(4):
                buf_val = combs[k][idx_c[k]]
                new_val = input_val + (buf_val * c_gains[k])
                combs[k][idx_c[k]] = new_val
                sum_combs += buf_val
                idx_c[k] = (idx_c[k] + 1) % c_lens[k]
        
            # All Pass
            ap1_in = sum_combs
            ap1_buf = aps[0][idx_ap[0]]
            aps[0][idx_ap[0]] = ap1_in + (ap1_buf * ap_gain)
            ap1_out = ap1_buf - (ap1_in * ap_gain)
            idx_ap[0] = (idx_ap[0] + 1) % 225
            
            ap2_in = ap1_out
            ap2_buf = aps[1][idx_ap[1]]
            aps[1][idx_ap[1]] = ap2_in + (ap2_buf * ap_gain)
            ap2_out = ap2_buf - (ap2_in * ap_gain)
            idx_ap[1] = (idx_ap[1] + 1) % 341
            
            
            sinal_dry = x          # O som original (já com pitch shift)
            sinal_wet = ap2_out    # O som do reverb
            
            # Mistura proporcional
            output[i] = (sinal_dry * (1.0 - mix)) + (sinal_wet * mix)
            
        return output

# ==============================================================================
# 🚀 MAIN
# ==============================================================================

def main():
    # 1. Carregar
    try:
        fs, audio = wav.read(ARQUIVO_ENTRADA)
    except:
        print(f"Erro: Não achei '{ARQUIVO_ENTRADA}'")
        return

    if len(audio.shape) > 1: audio = audio[:,0]
    audio = audio.astype(np.float32) / 32768.0
    
    proc = ProcessadorAudio(fs)

    # 2. Loop pelos Alvos
    for alvo in ALVOS:
        nome_arquivo = f"resultado_{alvo['nome']}.wav"
        
        # Calcular Ratio
        ratio, semitons = calcular_ratio(NOTA_ORIGINAL, alvo['nota'], alvo['oitava'])
        
        print(f"Gerando '{alvo['nome']}'...")
        print(f"   -> Transição: {NOTA_ORIGINAL} para {alvo['nota']}{alvo['oitava']}")
        print(f"   -> Semitons: {semitons}")
        print(f"   -> Ratio Delay: {ratio:.4f}")
        
        # Passo A: Pitch Shift
        pitched = proc.pitch_shift(audio, ratio)
        
        # Passo B: Reverb
        final = proc.reverb(pitched, C_LENS, C_GAINS, AP_GAIN, mix = 0.4)
        
        # Salvar
        max_val = np.max(np.abs(final))
        if max_val > 0: final = final / max_val * 0.95
        wav.write(nome_arquivo, fs, (final * 32767).astype(np.int16))
        print(f"   -> Salvo: {nome_arquivo}\n")

if __name__ == "__main__":
    main()