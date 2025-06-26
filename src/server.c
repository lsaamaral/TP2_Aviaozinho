#include "common.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>

#include <sys/types.h>
#include <sys/socket.h>

struct client_data *clients[MAX_CLIENTS] = {0};
game_state_t current_state = WAITING_FOR_PLAYERS;
float house_profit = 0.0;
int next_player_id = 1;
float last_multiplier = 1.0;

time_t betting_start_time;
pthread_mutex_t state_mutex = PTHREAD_MUTEX_INITIALIZER;

void usage(int argc, char **argv) {
    printf("usage: %s <v4|v6> <server port>\n", argv[0]);
    printf("example: %s v4 51511\n", argv[0]);
    exit(EXIT_FAILURE);
}

void broadcast_message(struct aviator_msg *msg) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i]) {
            send(clients[i]->csock, msg, sizeof(struct aviator_msg), 0);
        }
    }
}

void *game(void *data) {
    (void)data;

    while(1) {
        pthread_mutex_lock(&state_mutex);
        int active_clients = 0;
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i]) {
                active_clients++;
            }
        }

        if (active_clients == 0) {
            current_state = WAITING_FOR_PLAYERS;
            pthread_mutex_unlock(&state_mutex);
            sleep(1);
            continue;
        }

        current_state = BETTING;
        betting_start_time = time(NULL);
        printf("event=start | id=* | N=%d\n", active_clients);

        struct aviator_msg start_msg;
        memset(&start_msg, 0, sizeof(start_msg));
        strcpy(start_msg.type, "start");
        start_msg.value = 10;

        broadcast_message(&start_msg);
        pthread_mutex_unlock(&state_mutex);

        for (int i = 0; i < 100; i++) {
            usleep(100000);
        }

        usleep(100000);
        
        pthread_mutex_lock(&state_mutex);

        current_state = IN_FLIGHT;
        int players_in_round = 0;
        float total_bet_value = 0.0;
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i] && clients[i]->has_bet_this_round) {
                players_in_round++;
                total_bet_value += clients[i]->current_bet;
            }
        }

        printf("event=closed | id=* | N=%d | V=%.2f\n", players_in_round, total_bet_value);
        struct aviator_msg closed_msg;
        memset(&closed_msg, 0, sizeof(closed_msg));
        strcpy(closed_msg.type, "closed");
        broadcast_message(&closed_msg);

        float me = explosion_point(players_in_round, total_bet_value);
        float multiplier = 1.00;
        last_multiplier = multiplier;
        pthread_mutex_unlock(&state_mutex);

        while (multiplier < me) {
            usleep(100000);
            
            pthread_mutex_lock(&state_mutex);

            if(current_state != IN_FLIGHT){
                pthread_mutex_unlock(&state_mutex);
                break;
            }

            multiplier += 0.01;
            last_multiplier = multiplier;

            struct aviator_msg mult_msg;
            memset(&mult_msg, 0, sizeof(mult_msg));
            strcpy(mult_msg.type, "multiplier");
            mult_msg.value = multiplier;
            broadcast_message(&mult_msg);
            printf("event=multiplier | id=* | m=%.2f\n", multiplier);
            pthread_mutex_unlock(&state_mutex);
        }

        pthread_mutex_lock(&state_mutex);
        current_state = EXPLODED;
        printf("event=explode | id=* | m=%.2f\n", last_multiplier);
        struct aviator_msg explode_msg;
        memset(&explode_msg, 0, sizeof(explode_msg));
        strcpy(explode_msg.type, "explode");
        explode_msg.value = last_multiplier;
        broadcast_message(&explode_msg);
        
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i] && clients[i]->has_bet_this_round) {
                if (!clients[i]->has_cashed_out) {
                    clients[i]->total_profit -= clients[i]->current_bet;
                    house_profit += clients[i]->current_bet;
                    
                    printf("event=explode | id=%d | m=%.2f\n", clients[i]->id, last_multiplier);
                    printf("event=profit | id=%d | player_profit=%.2f\n", clients[i]->id, clients[i]->total_profit);
                    
                    struct aviator_msg profit_msg;
                    memset(&profit_msg, 0, sizeof(profit_msg));
                    strcpy(profit_msg.type, "profit");
                    profit_msg.player_id = clients[i]->id;
                    profit_msg.player_profit = clients[i]->total_profit;
                    send(clients[i]->csock, &profit_msg, sizeof(profit_msg), 0);
                }
            }
        }

        printf("event=profit | id=* | house_profit=%.2f\n", house_profit);
        struct aviator_msg house_profit_msg;
        memset(&house_profit_msg, 0, sizeof(house_profit_msg));
        strcpy(house_profit_msg.type, "profit");
        house_profit_msg.house_profit = house_profit;
        broadcast_message(&house_profit_msg);

        for (int i = 0; i < MAX_CLIENTS; i++) {
            if(clients[i]) {
                clients[i]->has_bet_this_round = false;
                clients[i]->has_cashed_out = false;
                clients[i]->current_bet = 0.0;
            }
        }
        
        pthread_mutex_unlock(&state_mutex);
        sleep(5);
    }
    return NULL;
}

void remove_client(int client_id) {
    pthread_mutex_lock(&state_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] && clients[i]->id == client_id) {
            close(clients[i]->csock);
            printf("event=bye | id=%d\n", clients[i]->id);
            free(clients[i]);
            clients[i] = NULL;
            break;
        }
    }
    pthread_mutex_unlock(&state_mutex);
}

void * client_thread(void *data) {
    struct  client_data *cdata = (struct client_data *)data;
    struct aviator_msg msg;

    char nick_buffer[NICK_LEN + 1] = {0};
    if (recv(cdata->csock, nick_buffer, NICK_LEN, 0) <= 0) {
        remove_client(cdata->id);
        return NULL;
    }
    strncpy(cdata->nickname, nick_buffer, NICK_LEN);
    printf("[log] client %d is %s\n", cdata->id, cdata->nickname);

    pthread_mutex_lock(&state_mutex);

    if (current_state == BETTING) {
        struct aviator_msg start_msg;
        memset(&start_msg, 0, sizeof(start_msg));
        strcpy(start_msg.type, "start");

        time_t now = time(NULL);
        int time_elapsed = now - betting_start_time;
        int time_remaining = 10 - time_elapsed;

        if (time_remaining < 0) {
            time_remaining = 0;
        }
        start_msg.value = time_remaining;
        send(cdata->csock, &start_msg, sizeof(start_msg), 0);
    }
    pthread_mutex_unlock(&state_mutex);

    while (recv(cdata->csock, &msg, sizeof(msg), 0) > 0) {
        pthread_mutex_lock(&state_mutex);

        if (strcmp(msg.type, "bet") == 0) {
            if (current_state == BETTING && !cdata->has_bet_this_round) {
                cdata->has_bet_this_round = true;
                cdata->current_bet = msg.value;
                
                int n_players = 0;
                float total_v = 0;
                for(int i = 0; i < MAX_CLIENTS; i++) {
                    if(clients[i] && clients[i]->has_bet_this_round) {
                        n_players++;
                        total_v += clients[i]->current_bet;
                    }
                }
                
                printf("event=bet | id=%d | bet=%.2f | N=%d | V=%.2f\n", cdata->id, cdata->current_bet, n_players, total_v);
            }

        } else if (strcmp(msg.type, "cashout") == 0) {
            if (current_state == IN_FLIGHT && cdata->has_bet_this_round && !cdata->has_cashed_out) {
                cdata->has_cashed_out = true;
                
                float server_multiplier = last_multiplier;
                float payout = cdata->current_bet * server_multiplier;
                float profit_this_round = payout - cdata->current_bet;
                cdata->total_profit += profit_this_round;
                house_profit -= profit_this_round;

                printf("event=cashout | id=%d | m=%.2f\n", cdata->id, server_multiplier);
                printf("event=payout | id=%d | payout=%.2f\n", cdata->id, payout);
                printf("event=profit | id=%d | player_profit=%.2f\n", cdata->id, cdata->total_profit);

                struct aviator_msg payout_msg;
                memset(&payout_msg, 0, sizeof(payout_msg));
                strcpy(payout_msg.type, "payout");
                payout_msg.value = payout;
                payout_msg.player_profit = server_multiplier; 
                send(cdata->csock, &payout_msg, sizeof(payout_msg), 0);

                struct aviator_msg profit_msg;
                memset(&profit_msg, 0, sizeof(profit_msg));
                strcpy(profit_msg.type, "profit");
                profit_msg.player_profit = cdata->total_profit;
                profit_msg.player_id = cdata->id;
                send(cdata->csock, &profit_msg, sizeof(profit_msg), 0);
            }
        } else if (strcmp(msg.type, "bye") == 0) {
            break;
        }
        pthread_mutex_unlock(&state_mutex);
    }

    remove_client(cdata->id);
    pthread_exit(EXIT_SUCCESS);

}

int main(int argc, char **argv) {
    signal(SIGPIPE, SIG_IGN);

    if (argc < 3) {
        usage(argc, argv);
    }

    struct sockaddr_storage storage;
    if (0 != server_sockaddr_init(argv[1], argv[2], &storage)) {
        usage(argc, argv);
    }

    int s = 0;
    s = socket(storage.ss_family, SOCK_STREAM, 0);
    if (s == -1) {
        logexit("socket");
    }

    int enable = 1;
    if (0 != setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int))) {
        logexit("setsockopt");
    }


    struct sockaddr *addr = (struct sockaddr *)(&storage);
    if (0 != bind(s, addr, sizeof(storage))) {
        logexit("bind");
    }

    if (0 != listen(s, 10)) {
        logexit("listen");
    }

    char addrstr[BUFSZ];
    addrtostr(addr, addrstr, BUFSZ);
    printf("Servidor escutando em %s\n", addrstr);

    pthread_t game_thread_id;
    if (pthread_create(&game_thread_id, NULL, game, NULL) != 0) {
        logexit("pthread_create for game loop");
    }

    while(1) {
        struct sockaddr_storage cstorage;
        struct sockaddr *caddr = (struct sockaddr *)(&cstorage);
        socklen_t caddrlen = sizeof(cstorage);

        int csock = accept(s, caddr, &caddrlen);
        if (csock == -1) {
            logexit("accept");
        }

        pthread_mutex_lock(&state_mutex);
        int client_slot = -1;
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i] == NULL) {
                client_slot = i;
                break;
            }
        }

        if (client_slot == -1) {
            printf("[log] max clients reached, connection rejected\n");
            close(csock);
            pthread_mutex_unlock(&state_mutex);
            continue;
        }

        struct client_data *cdata = calloc(1, sizeof(*cdata));
        if (!cdata) {
            logexit("malloc");
        }
        cdata->csock= csock;
        memcpy(&(cdata->storage), &cstorage, sizeof(cstorage));

        cdata->id = next_player_id++;
        cdata->total_profit = 0;
        cdata->has_bet_this_round = false;
        cdata->has_cashed_out = false;
        cdata->current_bet = 0;
        
        clients[client_slot] = cdata;
        pthread_create(&cdata->tid, NULL, client_thread, cdata);
        pthread_mutex_unlock(&state_mutex);
    }

    exit(EXIT_SUCCESS);
}