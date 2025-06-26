#include "common.h"

#include <stdlib.h>
#include <arpa/inet.h>

/**
 * @brief Exibe uma mensagem para auxiliar a criar um servidor ou cliente. Funcao retirada da videoaula do professor Italo Cunha
 * 
 * @param argc Numero de argumentos
 * @param argv Vetor de argumentos
 */
void usage(int argc, char **argv);

/**
 * @brief Retira o lixo de scanfs para entradas que dao problema
 */
void clean();

/**
 * @brief Loop principal, verifica se a conexao esta correta, recebe as mensagens do servidor e processa elas. Encerra quando recebe o fim do jogo.
 * 
 * @param argc Numero de argumentos
 * @param argv Vetor de argumentos
 * @return Retorna EXIT_SUCCESS quando o codigo nao teve nenhum problema
 */
int main(int argc, char **argv);