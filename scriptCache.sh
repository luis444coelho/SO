#!/bin/bash

# Caminho completo do dataset
DATASET_PATH="../Gdataset"

# Função para testar uma política
testar_politica() {
    local politica=$1
    local nome=$2

    echo "======================================="
    echo " Testando política: $nome"
    echo "======================================="

    # Limpeza
    rm -f /tmp/server_pipe
    rm -f metadados.txt
    rm -f server_output_$nome.txt

    # Inicia o servidor com política selecionada (1 = LRU, 2 = FIFO, 3 = MRU)
    ./bin/dserver "$DATASET_PATH" 2 "$politica" > server_output_$nome.txt &
    SERVER_PID=$!
    sleep 1

    # Inserções
    ./bin/dclient -a "Romeo and Juliet" "William Shakespeare" "1997" "1112.txt"
    ./bin/dclient -a "Romeo and Juliet" "William Shakespeare" "1997" "1112.txt"
    ./bin/dclient -a "Romeo and Juliet" "William Shakespeare" "1997" "1112.txt"

    # IDs atribuídos esperados: consecutivos (ex: 1, 2, 3)
    local ids=(1 2 3)

    local total_time_ns=0
    local num_consultas=100

    for ((i = 0; i < num_consultas; i++)); do
        id=${ids[$((i % 3))]}
        start_ns=$(date +%s%N)
        ./bin/dclient -c "$id" > /dev/null
        end_ns=$(date +%s%N)
        duration_ns=$((end_ns - start_ns))
        total_time_ns=$((total_time_ns + duration_ns))
    done

    # Tempo médio em milissegundos com 3 casas decimais
    avg_time_ms=$(echo "scale=3; $total_time_ns / $num_consultas / 1000000" | bc)

    sleep 1
    kill $SERVER_PID
    wait $SERVER_PID 2>/dev/null

    echo -e "\n[Tempo médio de consulta - $nome]: ${avg_time_ms} ms\n"
}

# Executar para cada política
testar_politica 1 "LRU"
testar_politica 2 "FIFO"
testar_politica 3 "MRU"
