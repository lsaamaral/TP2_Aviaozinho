#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <stdatomic.h>

#include <sys/types.h>
#include <sys/socket.h>

atomic_bool game_in_flight = false;
atomic_bool bet_placed = false;
atomic_bool countdown_active = false;
_Atomic float last_multiplier = 1.0;
_Atomic float current_bet_value = 0.0;
char my_nickname[NICK_LEN + 1];

void usage(int argc, char **argv) {
    printf("usage: %s <server IP> <server port>", argv[0]);
    printf("example: %s 127.0.0.1 51511", argv[0]);
    exit(EXIT_FAILURE);
}

void cleaninput() {
    int c;
    do {
        c = getchar();
    } while (c != '\n' && c != EOF);
}

void *countdown_thread(void *data) {
    (void)data;

    for (int i = 9; i >= 0; i--) {
        if (!atomic_load(&countdown_active)) {
            break; 
        }

        sleep(1);

        if (!atomic_load(&countdown_active)) {
            break;
        }

        printf("\r> Rodada aberta! Digite o valor da aposta ou [Q] para sair (%d segundos restantes): ", i);
        fflush(stdout);
    }

    atomic_store(&countdown_active, false);
    return NULL;
}

void *server_listener_thread(void *data) {
    int s = *(int*)data;
    struct aviator_msg msg;

    while (recv(s, &msg, sizeof(msg), 0) > 0) {
        if (strcmp(msg.type, "start") == 0) {
            bet_placed = false;
            game_in_flight = false;

            printf("\nRodada aberta! Digite o valor da aposta ou [Q] para sair (%d segundos restantes): \n", (int)msg.value);
            fflush(stdout);

            atomic_store(&countdown_active, true);
            pthread_t countdown_tid;
            pthread_create(&countdown_tid, NULL, countdown_thread, NULL);
            pthread_detach(countdown_tid);

        } else if (strcmp(msg.type, "closed") == 0) {
            atomic_store(&countdown_active, false);

            printf("\nApostas encerradas! Nao e mais possivel apostar nesta rodada.\n");
            if (bet_placed) {
                printf("Digite [C] para sacar.\n");
                game_in_flight = true;
            }
        } else if (strcmp(msg.type, "multiplier") == 0) {
            atomic_store(&last_multiplier, msg.value);
            printf("\rMultiplicador atual: %.2fx", msg.value);
            fflush(stdout);
        } else if (strcmp(msg.type, "explode") == 0) {
            atomic_store(&countdown_active, false);
            game_in_flight = false;
            printf("\nAviãozinho explodiu em: %.2fx\n", msg.value);
            if (bet_placed) {
                printf("Você perdeu R$ %.2f. Tente novamente na próxima rodada! Aviãozinho tá pagando :)\n", atomic_load(&current_bet_value));
                bet_placed = false;
            }
        } else if (strcmp(msg.type, "payout") == 0) {
            printf("\nVocê sacou em %.2fx e ganhou R$ %.2f!\n", atomic_load(&last_multiplier), msg.value);
            bet_placed = false;
        } else if (strcmp(msg.type, "profit") == 0) {
            if (msg.player_id != 0) {
                printf("Profit atual: R$ %.2f\n", msg.player_profit);
            }
            if (msg.house_profit != 0) {
                printf("Profit da casa: R$ %.2f\n", msg.house_profit);
            }
        } else if (strcmp(msg.type, "bye") == 0) {
            atomic_store(&countdown_active, false);
            printf("\nO servidor caiu, mas sua esperança pode continuar de pé. Até breve!\n");
            close(s);
            exit(EXIT_SUCCESS);
        }
    }
    printf("\nDesconectado do servidor.\n");
    exit(EXIT_SUCCESS);
    return NULL;
}

int main(int argc, char **argv) {
    if (argc != 5) {
        printf("Error: Invalid number of arguments\n");
        usage(argc, argv);
    }
    if (strcmp(argv[3], "-nick") != 0) {
        printf("Error: Expected '-nick' argument\n");
        usage(argc, argv);
    }
    if (strlen(argv[4]) > NICK_LEN) {
        printf("Error: Nickname too long (max 13)\n");
        usage(argc, argv);
    }
    strncpy(my_nickname, argv[4], NICK_LEN);
    my_nickname[NICK_LEN] = '\0';

    struct sockaddr_storage storage;
    if (0 != addrparse(argv[1], argv[2], &storage)) {
        usage(argc, argv);
    }

    int s = 0;
    s = socket(storage.ss_family, SOCK_STREAM, 0);
    if (s == -1) {
        logexit("socket");
    }

    struct sockaddr *addr = (struct sockaddr *)(&storage);
    if ( 0 != connect(s, addr, sizeof(storage))) {
        logexit("connect");
    }

    printf("\nConectado ao servidor.\n");

    struct aviator_msg nick_msg;
    memset(&nick_msg, 0, sizeof(nick_msg));
    strcpy(nick_msg.type, "nick");
    // Armazena o nickname nos primeiros bytes do campo value
    memcpy(&nick_msg.value, my_nickname, strlen(my_nickname));
    send(s, &nick_msg, sizeof(nick_msg), 0);
    
    // Inicia a thread para ouvir o servidor
    pthread_t listener_tid;
    pthread_create(&listener_tid, NULL, server_listener_thread, &s);

    // Loop principal para entrada do usuário
    char line[BUFSZ];
    while (fgets(line, BUFSZ, stdin) != NULL) {
        struct aviator_msg msg_out;
        memset(&msg_out, 0, sizeof(msg_out));
        msg_out.player_id = 0;

        if (line[0] == 'Q' || line[0] == 'q') {
            strcpy(msg_out.type, "bye");
            send(s, &msg_out, sizeof(msg_out), 0);
            break; 
        } else if (!game_in_flight && !bet_placed) {
            float bet_val = strtof(line, NULL);
            if (bet_val > 0) {
                strcpy(msg_out.type, "bet");
                msg_out.value = bet_val;
                atomic_store(&current_bet_value, bet_val);
                if (send(s, &msg_out, sizeof(msg_out), 0) < 0) {
                    logexit("send bet");
                }
                printf("Aposta recebida: R$ %.2f\n", bet_val);
                bet_placed = true;
                continue;
            } else {
                printf("Error: Invalid bet value\n");
                continue;
            }
        } else if (game_in_flight && bet_placed && (line[0] == 'C' || line[0] == 'c')) {
            strcpy(msg_out.type, "cashout");
            msg_out.value = atomic_load(&last_multiplier);
            send(s, &msg_out, sizeof(msg_out), 0);
            printf("-> Pedido de cashout enviado.\n");
        } else {
            printf("Error: Invalid command\n");
        }
    }

    close(s);
    printf("Aposte com responsabilidade. A plataforma é nova e tá com horário bugado. Volte logo, %s.\n", my_nickname);
    exit(EXIT_SUCCESS);
}
