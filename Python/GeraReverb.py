import numpy as np
import scipy.io.wavfile as wav
import time

# ==============================================================================
# 🎛️ ÁREA DE PARÂMETROS (EDITE AQUI)
# ==============================================================================

# Nome do arquivo de entrada (deve estar na mesma pasta)
ARQUIVO_ENTRADA = "original.wav"

# Nome do arquivo de saída
ARQUIVO_SAIDA = "reverb_teste.wav"

# --- 1. TAMANHOS DOS BUFFERS (Delay Lengths) ---
# Copie os valores que você quer testar (números primos recomendados)
C_LENS = [1687, 1601, 2053, 2251]  # Combs (Sala)
AP_LENS = [225, 341]               # All-Pass (Difusão)

# --- 2. GANHOS (0.0 a 1.0) ---
# Aqui você define o tempo de decaimento.
# OBS: Se no C você usa Q15 (ex: 27524), divida por 32767 aqui.
# Ex: 27524 / 32767 = 0.84

C_GAINS = [0.9482, 0.9329, 0.9423, 0.9] # Ganhos individuais para cada Comb
AP_GAIN = 0.6119                      # Ganho do All-Pass (fixo para ambos)

# ==============================================================================
# ⚙️ MOTOR DO REVERB (NÃO PRECISA MEXER)
# ==============================================================================

class DSP_Reverb_Simulator:
    def __init__(self, c_lens, c_gains, ap_lens, ap_gain):
        self.c_lens = c_lens
        self.c_gains = c_gains
        self.ap_lens = ap_lens
        self.ap_gain = ap_gain
        
        # Inicializa buffers com zeros
        self.combs = [np.zeros(l) for l in c_lens]
        self.aps = [np.zeros(l) for l in ap_lens]
        
        # Índices de leitura/escrita (ponteiros circulares)
        self.idx_c = [0] * 4
        self.idx_ap = [0] * 2

    def process_file(self, input_audio):
        """
        Processa o áudio amostra por amostra, igual ao DSP C55x.
        """
        # Preparar saída
        output_audio = np.zeros_like(input_audio)
        total_samples = len(input_audio)
        
        print(f"Processando {total_samples} amostras...")
        start_time = time.time()

        # Loop principal (Simula o 'for' do dma.c)
        for i, sample in enumerate(input_audio):
            
            # Input Gain (x >> 2 no C equivale a x * 0.25)
            x_n = sample * 0.25
            
            # --- ESTÁGIO 1: 4 COMBS PARALELOS ---
            sum_combs = 0.0
            
            for k in range(4):
                # Ler do buffer (pC[idx])
                buff_val = self.combs[k][self.idx_c[k]]
                
                # Feedback: buffer = input + (buff_val * gain)
                # No C: tempCalc = (x >> 2) + ((buff * GAIN) >> 15)
                new_val = x_n + (buff_val * self.c_gains[k])
                
                # Atualizar buffer
                self.combs[k][self.idx_c[k]] = new_val
                
                # Somar para a saída
                sum_combs += buff_val
                
                # Avançar índice circular
                self.idx_c[k] = (self.idx_c[k] + 1) % self.c_lens[k]
            
            # --- ESTÁGIO 2: ALL-PASS 1 ---
            ap1_in = sum_combs
            ap1_buf = self.aps[0][self.idx_ap[0]]
            
            # Schroeder All-Pass:
            # Buffer_New = Input + (Buffer_Old * Gain)
            # Output     = Buffer_Old - (Input * Gain)
            
            self.aps[0][self.idx_ap[0]] = ap1_in + (ap1_buf * self.ap_gain)
            ap1_out = ap1_buf - (ap1_in * self.ap_gain)
            
            self.idx_ap[0] = (self.idx_ap[0] + 1) % self.ap_lens[0]
            
            # --- ESTÁGIO 3: ALL-PASS 2 ---
            ap2_in = ap1_out
            ap2_buf = self.aps[1][self.idx_ap[1]]
            
            self.aps[1][self.idx_ap[1]] = ap2_in + (ap2_buf * self.ap_gain)
            ap2_out = ap2_buf - (ap2_in * self.ap_gain)
            
            self.idx_ap[1] = (self.idx_ap[1] + 1) % self.ap_lens[1]
            
            # Saída final
            output_audio[i] = ap2_out

            # Barra de progresso simples
            if i % 10000 == 0:
                prog = (i / total_samples) * 100
                print(f"\rProgresso: {prog:.1f}%", end="")

        print(f"\nConcluído em {time.time() - start_time:.2f} segundos.")
        return output_audio

# ==============================================================================
# 📂 LEITURA E ESCRITA DE ARQUIVOS
# ==============================================================================

def main():
    try:
        # 1. Ler Arquivo
        fs, audio_data = wav.read(ARQUIVO_ENTRADA)
        print(f"Arquivo '{ARQUIVO_ENTRADA}' carregado. Taxa: {fs}Hz")

        # Se for estéreo, converte para mono
        if len(audio_data.shape) > 1:
            print("Convertendo estéreo para mono...")
            audio_data = audio_data.mean(axis=1)

        # Converter para float (-1.0 a 1.0) para processamento
        # Assumindo áudio 16-bit PCM
        audio_float = audio_data.astype(np.float32)
        max_val = np.max(np.abs(audio_float))
        if max_val > 0:
            audio_float /= 32768.0 

        # 2. Processar
        simulator = DSP_Reverb_Simulator(C_LENS, C_GAINS, AP_LENS, AP_GAIN)
        audio_processed = simulator.process_file(audio_float)

        # 3. Normalizar e Salvar
        # Normaliza o pico para evitar clipping no arquivo final
        max_out = np.max(np.abs(audio_processed))
        if max_out > 0:
            audio_processed = audio_processed / max_out * 0.9 # 90% do volume max
        
        # Converter de volta para Int16
        audio_int16 = (audio_processed * 32767).astype(np.int16)
        
        wav.write(ARQUIVO_SAIDA, fs, audio_int16)
        print(f"Sucesso! Arquivo salvo como: '{ARQUIVO_SAIDA}'")
        print("Agora você pode ouvir este arquivo e comparar com o da mesa.")

    except FileNotFoundError:
        print(f"ERRO: Não encontrei o arquivo '{ARQUIVO_ENTRADA}'.")
        print("Certifique-se de colocar um arquivo .wav na mesma pasta deste script.")
    except Exception as e:
        print(f"Ocorreu um erro: {e}")

if __name__ == "__main__":
    main()