#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Estrutura que representa uma página na tabela de páginas de um processo
typedef struct {
    int frame;          // Número do frame físico onde a página está carregada
    int presente;       // Flag indicando se a página está na memória física (1) ou não (0)
    int tempo_carga;    // Timestamp de quando a página foi carregada (para algoritmo FIFO)
    int ultimo_acesso;  // Timestamp do último acesso à página (para algoritmo LRU)
} Pagina;

// Estrutura que representa um processo no sistema
typedef struct {
    int pid;                    // Identificador único do processo
    int tamanho;               // Tamanho total do processo em bytes
    int num_paginas;           // Número total de páginas que o processo ocupa
    Pagina *tabela_paginas;    // Ponteiro para a tabela de páginas do processo
} Processo;

// Estrutura que representa uma entrada na memória física
typedef struct {
    int pid;     // PID do processo que possui a página neste frame
    int pagina;  // Número da página que está armazenada neste frame
} EntradaMemoria;

// Estrutura que representa a memória física do sistema
typedef struct {
    EntradaMemoria *frames;  // Array de frames da memória física
    int *tempo_carga;       // Array com o tempo de carregamento de cada frame
    int num_frames;         // Número total de frames disponíveis na memória
} MemoriaFisica;

// Estrutura principal do simulador que contém todas as informações do sistema
typedef struct {
    Processo *processos;      // Array de processos no sistema
    int num_processos;       // Número atual de processos
    MemoriaFisica memoria;   // Estrutura da memória física
    int tamanho_pagina;      // Tamanho de cada página em bytes
    int algoritmo;           // Algoritmo de substituição (0=FIFO, 1=LRU)
    int tempo_atual;         // Contador de tempo para a simulação
    int page_faults;         // Contador de page faults ocorridos
    int total_acessos;       // Contador total de acessos à memória
} Simulador;

// Função para inicializar o simulador com os parâmetros especificados
Simulador *inicializar_simulador(int tamanho_pagina, int tamanho_memoria, int algoritmo) {
    // Aloca memória para a estrutura principal do simulador
    Simulador *sim = malloc(sizeof(Simulador));
    sim->tamanho_pagina = tamanho_pagina;
    
    // Calcula o número de frames baseado no tamanho da memória e tamanho da página
    sim->memoria.num_frames = tamanho_memoria / tamanho_pagina;
    
    // Aloca memória para os arrays de frames e tempo de carregamento
    sim->memoria.frames = malloc(sim->memoria.num_frames * sizeof(EntradaMemoria));
    sim->memoria.tempo_carga = malloc(sim->memoria.num_frames * sizeof(int));
    
    // Inicializa todos os frames como vazios (-1 indica frame vazio)
    for (int i = 0; i < sim->memoria.num_frames; i++) {
        sim->memoria.frames[i].pid = -1;
        sim->memoria.frames[i].pagina = -1;
        sim->memoria.tempo_carga[i] = -1;
    }
    
    // Aloca espaço para até 10 processos e inicializa contadores
    sim->processos = malloc(10 * sizeof(Processo));
    sim->num_processos = 0;
    sim->algoritmo = algoritmo;
    sim->tempo_atual = 0;
    sim->page_faults = 0;
    sim->total_acessos = 0;

    // Imprime informações iniciais da simulação
    printf("===== SIMULADOR DE PAGINAÇÃO =====\n");
    printf("Tamanho da página: %d bytes (%.0f KB)\n", tamanho_pagina, tamanho_pagina / 1024.0);
    printf("Tamanho da memória física: %d bytes (%.0f KB)\n", tamanho_memoria, tamanho_memoria / 1024.0);
    printf("Número de frames: %d\n", sim->memoria.num_frames);
    printf("Algoritmo de substituição: %s\n", algoritmo == 0 ? "FIFO" : "LRU");
    printf("======== INÍCIO DA SIMULAÇÃO ========\n");
    return sim;
}

// Função para criar um novo processo no simulador
Processo *criar_processo(Simulador *sim, int tamanho) {
    // Obtém referência para o próximo slot disponível no array de processos
    Processo *p = &sim->processos[sim->num_processos++];
    p->pid = sim->num_processos;  // Usa o número sequencial como PID
    p->tamanho = tamanho;
    
    // Calcula quantas páginas são necessárias para o processo
    // Usa divisão com arredondamento para cima
    p->num_paginas = (tamanho + sim->tamanho_pagina - 1) / sim->tamanho_pagina;
    
    // Aloca e inicializa a tabela de páginas (calloc inicializa com zeros)
    p->tabela_paginas = calloc(p->num_paginas, sizeof(Pagina));
    return p;
}

// Função para buscar um processo pelo seu PID
Processo *buscar_processo(Simulador *sim, int pid) {
    for (int i = 0; i < sim->num_processos; i++)
        if (sim->processos[i].pid == pid) return &sim->processos[i];
    return NULL;  // Retorna NULL se processo não for encontrado
}

// Função para imprimir o estado atual da memória física
void imprimir_memoria(Simulador *sim) {
    printf("Tempo t=%d\n", sim->tempo_atual);
    printf("Estado da Memória Física:\n");
    
    // Percorre todos os frames e mostra seu conteúdo
    for (int i = 0; i < sim->memoria.num_frames; i++) {
        printf("--------\n");
        if (sim->memoria.frames[i].pid != -1)
            // Frame ocupado: mostra PID e número da página
            printf("| P%d-%d |\n", sim->memoria.frames[i].pid, sim->memoria.frames[i].pagina);
        else
            // Frame vazio
            printf("| ---- |\n");
    }
    printf("--------\n");
}

// Função para extrair o número da página e deslocamento de um endereço virtual
void extrair_pagina_deslocamento(Simulador *sim, int endereco, int *pagina, int *deslocamento) {
    *pagina = endereco / sim->tamanho_pagina;      // Divisão inteira dá o número da página
    *deslocamento = endereco % sim->tamanho_pagina; // Resto da divisão dá o deslocamento
}

// Função para encontrar um frame vazio na memória física
int encontrar_frame_vazio(Simulador *sim) {
    for (int i = 0; i < sim->memoria.num_frames; i++)
        if (sim->memoria.frames[i].pid == -1) return i;  // Frame vazio encontrado
    return -1;  // Nenhum frame vazio disponível
}

// Função para escolher qual página substituir usando FIFO ou LRU
int escolher_pagina_para_substituir(Simulador *sim) {
    int escolhido = 0;  // Índice do frame escolhido para substituição
    int valor = sim->tempo_atual + 1;  // Valor inicial para comparação

    // Percorre todos os frames para encontrar o melhor candidato
    for (int i = 0; i < sim->memoria.num_frames; i++) {
        // Obtém o processo e página atual do frame
        Processo *p = buscar_processo(sim, sim->memoria.frames[i].pid);
        Pagina *pag = &p->tabela_paginas[sim->memoria.frames[i].pagina];

        // Define critério baseado no algoritmo:
        // FIFO (0): usa tempo de carregamento (mais antigo primeiro)
        // LRU (1): usa último acesso (menos recentemente usado primeiro)
        int criterio = sim->algoritmo == 0 ? pag->tempo_carga : pag->ultimo_acesso;

        // Escolhe o frame com menor valor do critério
        if (criterio < valor) {
            valor = criterio;
            escolhido = i;
        }
    }
    return escolhido;
}

// Função principal para registrar um acesso a uma página
void registrar_acesso(Simulador *sim, int pid, int pagina) {
    sim->total_acessos++;  // Incrementa contador de acessos
    Processo *p = buscar_processo(sim, pid);
    Pagina *pag = &p->tabela_paginas[pagina];

    // Verifica se a página não está presente na memória (PAGE FAULT)
    if (!pag->presente) {
        printf("Tempo t=%d: [PAGE FAULT] Página %d do Processo %d não está na memória física!\n", 
               sim->tempo_atual, pagina, pid);
        sim->page_faults++;

        // Tenta encontrar um frame vazio
        int frame = encontrar_frame_vazio(sim);

        if (frame == -1) {
            // Não há frames vazios - precisa substituir uma página
            frame = escolher_pagina_para_substituir(sim);
            
            // Remove a página antiga do frame
            int pid_ant = sim->memoria.frames[frame].pid;
            int pag_ant = sim->memoria.frames[frame].pagina;
            buscar_processo(sim, pid_ant)->tabela_paginas[pag_ant].presente = 0;

            printf("Tempo t=%d: Substituindo Página %d do Processo %d no Frame %d pela Página %d do Processo %d (%s)\n",
                   sim->tempo_atual, pag_ant, pid_ant, frame, pagina, pid,
                   sim->algoritmo == 0 ? "FIFO" : "LRU");
        } else {
            // Frame vazio encontrado - carregamento simples
            printf("Tempo t=%d: Carregando Página %d do Processo %d no Frame %d\n", 
                   sim->tempo_atual, pagina, pid, frame);
        }

        // Atualiza as estruturas com a nova página carregada
        sim->memoria.frames[frame].pid = pid;
        sim->memoria.frames[frame].pagina = pagina;
        sim->memoria.tempo_carga[frame] = sim->tempo_atual;

        // Atualiza a tabela de páginas do processo
        pag->frame = frame;
        pag->presente = 1;
        pag->tempo_carga = sim->tempo_atual;
    }

    // Atualiza o timestamp do último acesso (importante para LRU)
    pag->ultimo_acesso = sim->tempo_atual;
    
    // Mostra o estado atual da memória e avança o tempo
    imprimir_memoria(sim);
    sim->tempo_atual++;
}

// Função para traduzir um endereço virtual em endereço físico
void traduzir_endereco(Simulador *sim, int pid, int endereco_virtual) {
    Processo *p = buscar_processo(sim, pid);
    if (!p) return;  // Processo não encontrado

    // Verifica se o endereço está dentro dos limites do processo
    if (endereco_virtual >= p->tamanho) return;

    int pagina, deslocamento;
    // Extrai página e deslocamento do endereço virtual
    extrair_pagina_deslocamento(sim, endereco_virtual, &pagina, &deslocamento);

    // Registra o acesso (pode causar page fault)
    registrar_acesso(sim, pid, pagina);
    
    // Calcula o endereço físico final
    int frame = p->tabela_paginas[pagina].frame;
    int endereco_fisico = frame * sim->tamanho_pagina + deslocamento;

    // Mostra a tradução completa do endereço
    printf("Tempo t=%d: Endereço Virtual (P%d): %d -> Página: %d -> Frame: %d -> Endereço Físico: %d\n\n",
           sim->tempo_atual - 1, pid, endereco_virtual, pagina, frame, endereco_fisico);
}

// Função para imprimir estatísticas finais da simulação
void imprimir_estatisticas(Simulador *sim) {
    printf("======== ESTATÍSTICAS DA SIMULAÇÃO ========\n");
    printf("Total de acessos à memória: %d\n", sim->total_acessos);
    printf("Total de page faults: %d\n", sim->page_faults);
    
    // Calcula e mostra a taxa de page faults como porcentagem
    double taxa = sim->total_acessos > 0 ? (double)sim->page_faults / sim->total_acessos * 100 : 0;
    printf("Taxa de page faults: %.2f%%\n", taxa);
    printf("Algoritmo: %s\n", sim->algoritmo == 0 ? "FIFO" : "LRU");
}

// Função para liberar toda a memória alocada pelo simulador
void finalizar_simulador(Simulador *sim) {
    // Libera as tabelas de páginas de todos os processos
    for (int i = 0; i < sim->num_processos; i++)
        free(sim->processos[i].tabela_paginas);
    
    // Libera as estruturas da memória física
    free(sim->memoria.frames);
    free(sim->memoria.tempo_carga);
    
    // Libera o array de processos e a estrutura principal
    free(sim->processos);
    free(sim);
}

// Função principal - ponto de entrada do programa
int main() {
    int algoritmo;
    
    // Solicita ao usuário escolher o algoritmo de substituição
    printf("Simulador de Paginação - Escolha algoritmo:\n0 - FIFO\n1 - LRU\n");
    scanf("%d", &algoritmo);

    // Inicializa simulador com:
    // - Páginas de 4KB (4096 bytes)
    // - Memória física de 16KB (4 frames de 4KB cada)
    // - Algoritmo escolhido pelo usuário
    Simulador *sim = inicializar_simulador(4096, 4096 * 4, algoritmo);

    // Cria dois processos de teste:
    Processo *p1 = criar_processo(sim, 10000); // Processo 1: 10KB (3 páginas)
    Processo *p2 = criar_processo(sim, 15000); // Processo 2: 15KB (4 páginas)

    // Define uma sequência de acessos à memória para teste
    // Cada entrada contém [PID, endereço_virtual]
    int acessos[][2] = {
        {p1->pid, 1111}, {p1->pid, 5000}, {p2->pid, 2000}, {p2->pid, 10000},
        {p1->pid, 3000}, {p2->pid, 14000}, {p1->pid, 9000}, {p2->pid, 500},
        {p1->pid, 1000}, {p2->pid, 8000}
    };

    // Executa todos os acessos de memória definidos
    int n = sizeof(acessos) / sizeof(acessos[0]);
    for (int i = 0; i < n; i++)
        traduzir_endereco(sim, acessos[i][0], acessos[i][1]);

    // Mostra estatísticas finais e limpa a memória
    imprimir_estatisticas(sim);
    finalizar_simulador(sim);
    return 0;
}
