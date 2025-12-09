import numpy as np
import scipy.io.wavfile as wav
import time

# ==============================================================================
# 🎛️ ÁREA DE PARÂMETROS (EDITE AQUI)
# ==============================================================================

# Arquivos
ARQUIVO_ENTRADA = "original.wav"
ARQUIVO_SAIDA = "reverb_com_filtro.wav"

# --- 1. CONFIGURAÇÃO DO REVERB (Schroeder) ---
C_LENS = [1687, 1601, 2053, 2251]  
AP_LENS = [225, 341]               

# Ganhos (0.0 a 1.0)
C_GAINS = [0.8403, 0.9419, 0.9438, 0.7149] # Ganhos individuais para cada Comb
AP_GAIN = 0.6                      
          

# --- 2. CONFIGURAÇÃO DO FILTRO PASSA-BAIXA (SOLUÇÃO 2) ---
ENABLE_PRE_LPF = True  # Mude para False para comparar sem o filtro

# Intensidade do Filtro (Alpha)
# 1.0 = Som Original (Sem filtro)
# 0.6 = Filtro Suave (Similar ao valor 20000 no DSP)
# 0.1 = Muito Abafado (Apenas sub-graves)
# FÓRMULA Q15 -> FLOAT: Valor_DSP / 32767
# Ex: Se você usar 14000 no DSP -> 14000 / 32767 = 0.42
LPF_ALPHA = 0.2

# ==============================================================================
# ⚙️ MOTOR DO REVERB + FILTRO
# ==============================================================================

class DSP_Simulator_With_Filter:
    def __init__(self, c_lens, c_gains, ap_lens, ap_gain, lpf_alpha, enable_lpf):
        self.c_lens = c_lens
        self.c_gains = c_gains
        self.ap_lens = ap_lens
        self.ap_gain = ap_gain
        self.lpf_alpha = lpf_alpha
        self.enable_lpf = enable_lpf
        
        # Buffers do Reverb
        self.combs = [np.zeros(l) for l in c_lens]
        self.aps = [np.zeros(l) for l in ap_lens]
        self.idx_c = [0] * 4
        self.idx_ap = [0] * 2
        
        # Estado do Filtro Low Pass (Memória do valor anterior)
        self.lpf_state = 0.0

    def process_file(self, input_audio):
        output_audio = np.zeros_like(input_audio)
        total_samples = len(input_audio)
        
        print(f"Processando com LPF={'LIGADO' if self.enable_lpf else 'DESLIGADO'} (Alpha={self.lpf_alpha})...")
        start_time = time.time()

        for i, sample in enumerate(input_audio):
            
            # --- ESTÁGIO 0: PRÉ-FILTRO (SOLUÇÃO 2) ---
            processed_input = sample
            
            if self.enable_lpf:
                # Implementação One-Pole Low Pass Filter
                # y[n] = y[n-1] + alpha * (x[n] - y[n-1])
                self.lpf_state += self.lpf_alpha * (sample - self.lpf_state)
                processed_input = self.lpf_state

            # Input Gain do Reverb (x >> 2)
            # Agora usamos o sinal já filtrado
            x_n = processed_input * 0.25
            
            # --- ESTÁGIO 1: 4 COMBS PARALELOS ---
            sum_combs = 0.0
            
            for k in range(4):
                buff_val = self.combs[k][self.idx_c[k]]
                
                # Feedback
                new_val = x_n + (buff_val * self.c_gains[k])
                
                self.combs[k][self.idx_c[k]] = new_val
                sum_combs += buff_val
                self.idx_c[k] = (self.idx_c[k] + 1) % self.c_lens[k]
            
            # --- ESTÁGIO 2: ALL-PASS 1 ---
            ap1_in = sum_combs
            ap1_buf = self.aps[0][self.idx_ap[0]]
            
            self.aps[0][self.idx_ap[0]] = ap1_in + (ap1_buf * self.ap_gain)
            ap1_out = ap1_buf - (ap1_in * self.ap_gain)
            self.idx_ap[0] = (self.idx_ap[0] + 1) % self.ap_lens[0]
            
            # --- ESTÁGIO 3: ALL-PASS 2 ---
            ap2_in = ap1_out
            ap2_buf = self.aps[1][self.idx_ap[1]]
            
            self.aps[1][self.idx_ap[1]] = ap2_in + (ap2_buf * self.ap_gain)
            ap2_out = ap2_buf - (ap2_in * self.ap_gain)
            self.idx_ap[1] = (self.idx_ap[1] + 1) % self.ap_lens[1]
            
            output_audio[i] = ap2_out

        print(f"Concluído em {time.time() - start_time:.2f} segundos.")
        return output_audio

# ==============================================================================
# 📂 MAIN
# ==============================================================================

def main():
    try:
        # Carregar
        fs, audio_data = wav.read(ARQUIVO_ENTRADA)
        
        # Converter Estéreo -> Mono
        if len(audio_data.shape) > 1:
            audio_data = audio_data.mean(axis=1)

        # Converter Int16 -> Float (-1.0 a 1.0)
        audio_float = audio_data.astype(np.float32)
        max_val = np.max(np.abs(audio_float))
        if max_val > 0: audio_float /= 32768.0 

        # Processar
        sim = DSP_Simulator_With_Filter(C_LENS, C_GAINS, AP_LENS, AP_GAIN, LPF_ALPHA, ENABLE_PRE_LPF)
        audio_processed = sim.process_file(audio_float)

        # Salvar (Normalizado)
        max_out = np.max(np.abs(audio_processed))
        if max_out > 0: audio_processed = audio_processed / max_out * 0.9
        
        wav.write(ARQUIVO_SAIDA, fs, (audio_processed * 32767).astype(np.int16))
        print(f"Arquivo salvo: {ARQUIVO_SAIDA}")

    except Exception as e:
        print(f"Erro: {e}")

if __name__ == "__main__":
    main()