#pragma once

#include <stdlib.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdbool.h>
 
#define STR_LEN  11
#define NICK_LEN 13
#define BUFSZ 1024
#define MAX_CLIENTS 15


struct aviator_msg {
    int32_t player_id;
    float value;
    char type[STR_LEN]; // tipo da mensagem
    float player_profit;
    float house_profit;
};

typedef enum {
    WAITING_FOR_PLAYERS,
    BETTING,
    IN_FLIGHT,
    EXPLODED
} game_state_t;

struct client_data {
    int csock;
    struct sockaddr_storage storage;
    int id;
    char nickname[NICK_LEN + 1];
    float current_bet;
    float total_profit;
    bool has_bet_this_round;
    bool has_cashed_out;
    pthread_t tid;
};

/**
 * @brief Armazena o erro e encerra o programa. Retirada da videoaula
 * 
 * @param msg Mensagem de erro que precisa ser exibida
 */
void logexit(const char *msg);

/**
 * @brief Calcula o ponto em que o aviaozinho ira explodir a partir da formula fornecida
 * 
 */
float explosion_point(int num_players, float total_apostado);

/**
 * @brief Converte um IP e porta para a struct sockaddr_storage
 * 
 * @param addrstr Str com o endereco IP
 * @param portstr Str com a porta
 * @param storage Struct que armazena o endereco
 * @return Retorna 0 se ocorreu tudo bem, -1 em caso de erro
 */
int addrparse(const char *addrstr, const char *portstr, struct sockaddr_storage *storage);

/**
 * @brief Converte um endereco de socket em str
 * 
 * @param addr Struct do endereco que vai ser convertido
 * @param str Buffer para armazenar a str do resultado
 * @param strsize Tamanho buffer
 */
void addrtostr(const struct sockaddr *addr, char *str, size_t strsize);

/**
 * @brief Inicializa a sockaddr_storage para o servidor
 * 
 * @param proto Str "v4" para IPv4 ou "v6" para IPv6 (protocolo)
 * @param portstr Str com a porta
 * @param storage Struct de armazenamento do endereco
 * @return Retorna 0 se ocorreu tudo bem, -1 em caso de erro
 */
int server_sockaddr_init(const char *proto, const char *portstr, struct sockaddr_storage *storage);