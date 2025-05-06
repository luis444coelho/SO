#!/bin/bash

# Caminho relativo do dataset, assumindo que o script está na pasta SO_44
DATASET_PATH="../Gdataset"

# Função para testar buscas
testar_search() {
    local politica=$1
    local nome=$2

    echo "======================================="
    echo " Testando search (serial vs paralelo) - Política: $nome"
    echo "======================================="

    # Limpeza
    rm -f /tmp/server_pipe
    rm -f metadados.txt
    rm -f server_output_$nome.txt

    # Inicia o servidor
    ./bin/dserver "$DATASET_PATH" 2 "$politica" > /dev/null 2>&1 &
    SERVER_PID=$!
    sleep 1

    # Inserções
    ./bin/dclient -a "Romeo and Juliet" "William Shakespeare" "1997" "8.txt"
    ./bin/dclient -a "Romeo and Juliet" "William Shakespeare" "1997" "1112.txt"
    ./bin/dclient -a "Romeo and Juliet" "William Shakespeare" "1997" "1112.txt"

    # Palavras a testar
    local palavras=("are" "window")
    local num_repeticoes=30
    local threads=(1 2 4 100)

    for palavra in "${palavras[@]}"; do
        echo -e "\n🔍 Palavra: '$palavra'"

        for t in "${threads[@]}"; do
            local total_time_ns=0

            for ((i = 0; i < num_repeticoes; i++)); do
                start_ns=$(date +%s%N)
                if [ "$t" -eq 1 ]; then
                    ./bin/dclient -s "$palavra" > /dev/null
                else
                    ./bin/dclient -s "$palavra" "$t" > /dev/null
                fi
                end_ns=$(date +%s%N)
                duration_ns=$((end_ns - start_ns))
                total_time_ns=$((total_time_ns + duration_ns))
            done

            avg_time_ms=$(echo "scale=3; $total_time_ns / $num_repeticoes / 1000000" | bc)

            if [ "$t" -eq 1 ]; then
                echo "[Tempo médio - SERIAL (1 thread) - '$palavra']: ${avg_time_ms} ms"
            else
                echo "[Tempo médio - PARALELO (${t} threads) - '$palavra']: ${avg_time_ms} ms"
            fi
        done
    done

    sleep 1
    kill $SERVER_PID
    wait $SERVER_PID 2>/dev/null
    echo ""
}

# Executar para cada política (podes descomentar as outras se quiseres)
testar_search 1 "LRU"
# testar_search 2 "FIFO"
# testar_search 3 "MRU"
