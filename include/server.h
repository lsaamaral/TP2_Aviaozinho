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
 * @brief Envia uma mensagem para todos os clientes conectados
 *
 * @param msg Ponteiro para a struct aviator_msg que vai ser transmitida
 */
void broadcast_message(struct aviator_msg *msg);

/**
 * @brief Thread principal do jogo, gerencia cada rodada. Espera jogadores, inicia apostas, executa o voo, calcula a explosao e separa os resultados
 *
 * @return Nao retorna, loop infinito
 */
void *game(void *data);


/**
 * @brief Remove o cliente
 *
 * @param client_id Identificador de qual cliente precisa ser removido
 * @return Nao retorna nada
 */
void remove_client(int client_id);

/**
 * @brief Responsavel por lidar com a comunicacao feita com um unico cliente
 * @details Depois de receber o nickname, a thread entra em um loop para receber as mensagens que podem ser bet, cashout ou bye
 *
 * @param data Ponteiro para a struct client_data do cliente selecionado
 * @return Retorna NULL quando a conexao com o cliente termina
 */
void * client_thread(void *data);

/**
 * @brief Loop principal, verifica se a conexao esta correta, prepara o socket e atende os clientes. Repete o jogo para novos clientes em um loop infinito
 * 
 * @param argc Numero de argumentos
 * @param argv Vetor de argumento
 * @return Retorna EXIT_SUCCESS quando o codigo nao teve nenhum problema
 */
int main(int argc, char **argv);