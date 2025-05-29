# So2G
Integrantes:
Nome: Gabriel de Santana RA: 10420595

Descrição:
Este código implementa um simulador de paginação de memória em C, capaz de operar com os algoritmos FIFO e LRU. Ele permite criar processos com tamanho variável, gerando tabelas de páginas para cada um. A tradução de endereços virtuais em físicos considera o tamanho da página e registra acessos à memória, detectando e tratando page faults. Quando necessário, o simulador substitui páginas com base no algoritmo escolhido. A memória física é representada por frames, e o estado dela é exibido a cada acesso. Ao final, são exibidas estatísticas como o total de acessos, page faults e taxa de falhas. O código inclui limpeza de memória e permite simulação interativa com entrada via scanf.

Compilar e Executar(Terminal):

cd (caminho até o diretório)

gcc paginacao.c -o paginacao

./paginacao
