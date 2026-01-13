
> **Universidade Federal da Bahia (UFBA)**
> Escola de Administração
> Programa de Pós-Graduação em Desenvolvimento e Gestão Social
>
> **Relatório Final — Laboratório de DSP**
> **Autores:** Ciro Andrade, Daniel Gomes, Gerson Daniel, José Santos

---

## 📖 Descrição do Projeto
Este projeto realiza a engenharia reversa e a reimplementação de efeitos de áudio da mesa digital comercial **Vedo/Teyun A8** no kit de desenvolvimento **TMS320C5502 eZdsp**. O objetivo foi reproduzir a assinatura sonora (Reverb, Pitch, Phaser, Wah) em um hardware com restrições de memória, utilizando **DMA** para processamento em tempo real e algoritmos otimizados em **Ponto Fixo**.

---

## 🎯 Objetivos

* Reimplementar efeitos de áudio
* Garantir processamento em tempo real
* Utilizar DMA p

---

## ⚙️ Arquitetura de Software

Os principais arquivos do projeto são:

* `dma.c`
* `oled.c`
* `*_params.h` — Parâmetros específicos de cada efeito

---

## 🔁 Gerenciamento de DMA (Ping-Pong Buffer)

Para evitar que a placa sofra com problemas de audio, foi implementado um esquema de **DMA em ping-pong**.

### Características

* Buffers globais: `g_rxBuffer`, `g_txBuffer`
* Tamanho total: **4096 amostras**
* Dois blocos de **2048 amostras**

### Funcionamento

* Interrupção de **meio do buffer** → processamento do bloco PING
* Interrupção de **fim do buffer** → processamento do bloco PONG

```c
interrupt void dmaRxIsr(void)
{
    if (dmaPingPongFlag == 0) {
        pRx = &g_rxBuffer[0];
        dmaPingPongFlag = 1;
    } else {
        pRx = &g_rxBuffer[AUDIO_BLOCK_SIZE];
        dmaPingPongFlag = 0;
    }
}
```

---

## 🔢 Aritmética de Ponto Fixo (Q15)

Todos os cálculos são realizados em **ponto fixo (Q15)**.

### Estratégia

* Uso de `Int16` e `Int32`
* Simulação de casas decimais com **bit shifting**
* Multiplicações normalizadas manualmente

```c
term1 = -((Int32)alpha_Q15 * (Int32)x_n) >> 15;
```


## 🎼 Pitch Shifter 

O efeito de Pitch Shift foi implementado por meio de um **acumulador inteiro de 32 bits**.

### Conceitos

* Overflow natural do inteiro gera um dente-de-serra
* Endereçamento circular do buffer de áudio
* Interpolação linear manual entre amostras

```c
idxA_0 = (int)(rdPtrA >> 16) & PITCH_MASK;
fracA  = (Uint16)(rdPtrA & 0xFFFF);

sA = vA0 + (Int16)(((Int32)(vA1 - vA0) * fracA) >> 16);
```

---

## 🖥️ Interface OLED


* Implementação de **rotinas de escrita invertida**
* Correção da ordem visual de caracteres longos

```c
void oled_updateEffectName(Uint8 state) {
    case 1: // REV HALL
        printLetter(...);
        printLetter(...);
        printLetter(...);
        printLetter(...);
        printWord_REV_Inverted();
        break;
}
```

---

## 🎚️ Efeitos Implementados

### 🌊 Reverb (Schroeder)

* 4 filtros Comb em paralelo
* 2 filtros All-Pass em série
* Uso de um único buffer global `g_reverbMemory`

### 🌪️ Phaser

* Filtros All-Pass modulados
* LFO baseado em tabela de seno (`g_lfoTable`)
* 256 posições pré-calculadas

### 🎸 Auto-Wah

* Detector de envelope (retificação + LPF)
* Filtro de Variável de Estado (SVF)
* Frequência de corte dinâmica baseada na amplitude do sinal

---


## 🤖 Determinação de Parâmetros com Python e IA Generativa

Para otimizar os **parâmetros dos efeitos** (ex.: ganhos dos filtros Comb e All-Pass), utilizamos **Python** com **algoritmos genéticos** e processamento de áudio.

### Abordagem

* Implementamos a **classe SchroederReverb** em Python simulando o efeito em tempo reduzido
* Pré-processamos os sinais de entrada e referência para alinhamento e envelope (via `hilbert`)
* Definimos uma **função de fitness** baseada no **erro quadrático médio do envelope em dB**
* Utilizamos a biblioteca **PyGAD** para evolução genética das soluções

### Contribuição de IA Generativa

* A IA gerativa, neste contexto, ajuda na **exploração do espaço de parâmetros** sem precisar testar manualmente cada combinação
* O GA atua como uma forma de **otimização inspirada em processos evolutivos**, simulando inteligência adaptativa
* Resultados: melhores configurações de **gains** para Reverb que são depois convertidos para Q15 no DSP

```python
# Exemplo de execução do GA
ga_instance = pygad.GA(
    num_generations=30,
    num_parents_mating=4,
    fitness_func=fitness_func,
    sol_per_pop=10,
    num_genes=5,
    gene_space=gene_space
)

ga_instance.run()
solution, solution_fitness, solution_idx = ga_instance.best_solution()
```

Essa etapa permitiu **automatizar a escolha de parâmetros complexos**, reduzindo o tempo de desenvolvimento e garantindo resultados consistentes.

---

## 🛠️ Compilação e Execução

### Requisitos

* **Hardware:** Spectrum Digital TMS320C5502 eZdsp
* **Software:** Code Composer Studio (CCS) v5

### Passos

1. Importar o projeto no CCS
2. Verificar o arquivo de linker (`.cmd`)
3. Mapear as seções `dmaMem` para a DARAM
4. Compilar o projeto (`Ctrl + B`)
5. Carregar o arquivo `.out` via JTAG
6. Conectar Line In e Headphone Out
7. Utilizar **SW1 / SW2** para alternar os efeitos

---

## 📝 Conclusão

Este trabalho demonstra que **DSPs legados** são plenamente capazes de executar **processamento de áudio avançado**, desde que combinados com:

* Uso eficiente de DMA
* Algoritmos em ponto fixo
* Organização cuidadosa de memória

O sistema final apresenta **robustez**, **baixa latência** e **execução contínua**, sem artefatos audíveis.

---

## 📚 Referências

* Texas Instruments — *TMS320C55x DSP Library Documentation*
* D. A. Christensen — *Interface to the OSD9616 OLED Display*
* Relatório Final da Disciplina (Seções 3, 4 e 5)

