#pragma once

#include <stdlib.h>
#include <arpa/inet.h>

#define MSG_SIZE 256

/**
 * @brief Tipos de mensagens que sao possiveis entre a interacao do cliente e do servidor
 */
typedef enum { 
    MSG_REQUEST,
    MSG_RESPONSE,
    MSG_RESULT,
    MSG_PLAY_AGAIN_REQUEST,
    MSG_PLAY_AGAIN_RESPONSE,
    MSG_ERROR,
    MSG_END         
} MessageType;

/**
 * @brief Struct que armazena informacoes importantes para o funcionamento da logica da conversa (o que os dois jogadores fizeram, quem ganhou o jogo, total de vitorias)
 */
typedef struct { 
    int type;
    int client_action;
    int server_action;
    int result;
    int client_wins;
    int server_wins;
    char message[MSG_SIZE];
} GameMessage; 

/**
 * @brief Armazena o erro e encerra o programa. Retirada da videoaula
 * 
 * @param msg Mensagem de erro que precisa ser exibida
 */
void logexit(const char *msg);

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