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
 * @brief Loop principal, verifica se a conexao esta correta, prepara o socket e atende os clientes. Repete o jogo para novos clientes em um loop infinito
 * 
 * @param argc Numero de argumentos
 * @param argv Vetor de argumento
 * @return Retorna EXIT_SUCCESS quando o codigo nao teve nenhum problema
 */
int main(int argc, char **argv);