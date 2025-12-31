#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include <libgen.h>
#include <time.h>
#include <signal.h>
#include <ctype.h>
#include "entity/entities.h"

#define BUFF_SIZE 4096
#define MAX 3000



typedef enum {
    RESP_NONE,
    RESP_FAVORITES,
    RESP_FRIENDS,
    RESP_REQUESTS,
    RESP_TAGGED
} ResponseType;
int clientfd = -1;
char client_username[MAX] = {0};

void handle_list_friend_requests(int sock) ;

int split_fields(char *str, char **fields, int max_fields) {
    int count = 0;
    char *p = str;

    fields[count++] = p;

    while (*p && count < max_fields) {
        if (*p == '|') {
            *p = '\0';
            fields[count++] = p + 1;
        }
        p++;
    }

    return count;
}
void format_time(time_t t, char *out, size_t size) {
    struct tm *tm_info = localtime(&t);
    strftime(out, size, "%Y-%m-%d %H:%M:%S", tm_info);
}


void print_favorite_header() {
    printf("+----+------------+--------------------+------------------+---------------------------------+--------------------------------+\n");
    printf("| ID | Owner      | Name               | Category         | Location                        | Created At                     |\n");
    printf("+----+------------+--------------------+------------------+---------------------------------+--------------------------------+\n");
}

void print_favorite_row(FavoritePlace *f) {
    char timebuf[32];
    format_time(f->created_at, timebuf, sizeof(timebuf));

    printf("| %-2d | %-10s | %-18s | %-16s | %-31s | %-30s |\n",
           f->id, f->owner, f->name, f->category, f->location, timebuf);
}

void print_friend_header() {
    printf("+--------------------+---------------------+\n");
    printf("| Friends            | Since               |\n");
    printf("+--------------------+---------------------+\n");
}

void print_friend_row(FriendRel *f) {
    char timebuf[32];
    format_time(f->since, timebuf, sizeof(timebuf));

    printf("| %-18s | %-19s |\n", f->user_a, timebuf);
}

void print_request_header() {
    printf("+----+--------------------+----------+---------------------+\n");
    printf("| ID | From               | Status   | Created At          |\n");
    printf("+----+--------------------+----------+---------------------+\n");
}

void print_request_row(FriendRequest *r) {
    char timebuf[32];
    format_time(r->created_at, timebuf, sizeof(timebuf));

    printf("| %-2d | %-18s | %-8s | %-19s |\n",
           r->id,
           r->from,
           r->status == 0 ? "PENDING" : "ACCEPTED",
           timebuf);
}
void print_tagged_header() {
    printf("+----+------------+--------------------+------------------+---------------------------------+--------------------------------+------------------+\n");
    printf("| ID | Owner      | Name               | Category         | Location                        | Created At                     | Tagger           |\n");
    printf("+----+------------+--------------------+------------------+---------------------------------+--------------------------------+------------------+\n");
}

void print_tagged_row(FavoritePlaceWithTags *t) {
    char timebuf[32];
    format_time(t->created_at, timebuf, sizeof(timebuf));

    printf("| %-2d | %-10s | %-18s | %-16s | %-31s | %-30s | %-16s |\n",
           t->id, t->owner, t->name, t->category, t->location, timebuf, t->tagger);
}


/**
 * @function send_request: Send a message to the server.
 */
void send_request(int sock, const char *buf) {
    ssize_t s = send(sock, buf, strlen(buf), 0);
    if (s == -1) {
        perror("send() error");
    }
}

/**
 * @function recv_response: Receive a message from the server.
 */
int recv_response(int sock, char *buff, size_t size) {
    ssize_t rcv = recv(sock, buff, size - 1, 0);
    if (rcv <= 0) {
        if (rcv == -1) perror("recv() error");
        return (int)rcv;
    }
    buff[rcv] = '\0';
    return (int)rcv;
}

/**
 * @function print_header: Print a formatted header
 */
void print_header(const char *title) {
    printf("\n");
    printf("╔════════════════════════════════════════╗\n");
    printf("║ %-38s ║\n", title);
    printf("╠════════════════════════════════════════╣\n");
}

void print_footer() {
    printf("╚════════════════════════════════════════╝\n\n");
}


void read_lines_from_server(int sock, ResponseType type) {
    char buff[BUFF_SIZE];
    char *message = malloc(BUFF_SIZE * 4); 
    if (!message) { perror("malloc() error"); return; };
    int printed_header = 0;
    int rcvBytes ;

    message[0] = '\0';

    while (1) {
        memset(buff, 0, sizeof(buff));
        rcvBytes = recv_response(sock, buff, sizeof(buff));
        if (rcvBytes <= 0) break;
        buff[rcvBytes] = '\0';
        //printf("\n[DEBUG] recvBytes=%d\n", rcvBytes);
        //printf("[DEBUG] buff=[%s]\n", buff);
        //printf("[DEBUG] message=[%s]\n", message);

        strcat(message, buff);

        if (strstr(message, "\r\n") == NULL)
            continue;

        char *line = strtok(message, "\r\n");
        while (line !=  NULL) {
            //printf("\n[DEBUG] line = [%s]\n", line);

             

            if (strcmp(line, "END") == 0) {
                goto DONE;
            }


            if (!strchr(line, '|')) {
                printf("\n%s\n", line);

                if (isdigit(line[0])) {
                    int status = atoi(line);

                    if (status >= 400) {
                        goto DONE;
                    }

                    if (type == RESP_NONE) {
                        goto DONE;
                    }
                }
                line = strtok(NULL, "\r\n");
                continue;
            }

            char temp[BUFF_SIZE];
            strcpy(temp, line);

            char *fields[8];
            int n = split_fields(temp, fields, 8);
            //printf("[DEBUG] split n=%d\n", n);

            if (type == RESP_FAVORITES && n == 6) {
                FavoritePlace f;
                f.id = atoi(fields[0]);
                strcpy(f.owner, fields[1]);
                strcpy(f.name, fields[2]);
                strcpy(f.category, fields[3]);
                strcpy(f.location, fields[4]);
                f.created_at = atol(fields[5]);

                if (!printed_header) {
                    print_favorite_header();
                    printed_header = 1;
                }
                print_favorite_row(&f);
            }

            else if (type == RESP_FRIENDS && n == 3) {
                FriendRel fr;
                //printf("Comparing: %s and %s with client_username: %s\n", fields[0], fields[1], client_username);
                if(strcmp(fields[0], client_username) == 0)
                    strcpy(fr.user_a, fields[1]);
                else
                    strcpy(fr.user_a, fields[0]);
                fr.since = atol(fields[2]);

                if (!printed_header) {
                    print_friend_header();
                    printed_header = 1;
                }
                print_friend_row(&fr);
                
            }

            else if (type == RESP_REQUESTS && n == 5) {
                FriendRequest r;
                r.id = atoi(fields[0]);
                strcpy(r.from, fields[1]);
                strcpy(r.to, fields[2]);
                r.status = atoi(fields[3]);
                r.created_at = atol(fields[4]);

                if (!printed_header) {
                    print_request_header();
                    printed_header = 1;
                }
                print_request_row(&r);
            
            }

            else if (type == RESP_TAGGED && n == 7) {
                FavoritePlaceWithTags t;
                t.id = atoi(fields[0]);
                strcpy(t.owner, fields[1]);
                strcpy(t.name, fields[2]);
                strcpy(t.category, fields[3]);
                strcpy(t.location, fields[4]);
                t.created_at = atol(fields[5]);
                strcpy(t.tagger, fields[6]);
                
                if (!printed_header) {
                    print_tagged_header();
                    printed_header = 1;
                }
                print_tagged_row(&t);
            
            }
            line = strtok(NULL, "\r\n");

           
        }
        
    }


DONE:
    if (printed_header) {
        if(type == RESP_FAVORITES) printf("+----+------------+--------------------+------------------+---------------------------------+--------------------------------+\n");
        else if(type == RESP_FRIENDS) printf("+--------------------+---------------------+\n");
        else if(type == RESP_REQUESTS) printf("+----+--------------------+----------+---------------------+\n");
        else if(type == RESP_TAGGED) printf("+----+------------+--------------------+------------------+---------------------------------+--------------------------------+------------------+\n");
    }
    free(message);
    return;
}






    
/**
 * @function display_main_menu: Display main menu after login
 */
void display_main_menu() {
    
    printf("\n");
    printf("╔════════════════════════════════════════╗\n");
    printf("║ SparkPlace Main Menu                   ║\n");
    printf("╠════════════════════════════════════════╣\n");
    printf("║ AUTHENTICATION:                        ║\n");
    printf("║   1. Login                             ║\n");
    printf("║   2. Register                          ║\n");
    printf("║ FAVORITES:                             ║\n");
    printf("║   3. List Favorites                    ║\n");
    printf("║   4. Add Favorite                      ║\n");
    printf("║   5. Edit Favorite                     ║\n");
    printf("║   6. Delete Favorite                   ║\n");
    printf("║   7. List Tagged Favorites             ║\n");
    printf("║                                        ║\n");
    printf("║ FRIENDS:                               ║\n");
    printf("║   8. List Friends                      ║\n");
    printf("║   9. Add Friend                        ║\n");
    printf("║  10. View Friend Requests              ║\n");
    printf("║  11. Accept Friend Request             ║\n");
    printf("║  12. Reject Friend Request             ║\n");
    printf("║  13. Remove Friend                     ║\n");
    printf("║  14. Tag Friend                        ║\n");
    printf("║                                        ║\n");
    printf("║ OTHER:                                 ║\n");
    printf("║  15. Logout                            ║\n");
    print_footer();
}

int handle_login(int sock, char *buff) {
    char username[128], password[128];
    print_header("Login");
    printf("║ Username: ");
    fgets(username, sizeof(username), stdin);
    username[strcspn(username, "\n")] = '\0';
    strcpy(client_username, username);
    printf("║ Password: ");
    fgets(password, sizeof(password), stdin);
    password[strcspn(password, "\n")] = '\0';
    print_footer();
    snprintf(buff, BUFF_SIZE, "LOGIN|%s|%s\r\n", username, password);
    send_request(sock, buff);
    read_lines_from_server(sock,RESP_NONE);
    return 1;
}

int handle_register(int sock, char *buff) {
    char username[128], password[128];
    print_header("Register");
    printf("║ Username: ");
    fgets(username, sizeof(username), stdin);
    username[strcspn(username, "\n")] = '\0';
    printf("║ Password: ");
    fgets(password, sizeof(password), stdin);
    password[strcspn(password, "\n")] = '\0';
    print_footer();
    snprintf(buff, BUFF_SIZE, "REGISTER|%s|%s\r\n", username, password);
    send_request(sock, buff);
    read_lines_from_server(sock, RESP_NONE);
    return 0;
}

/**
 * @function handle_logout: Handle logout
 */
int handle_logout(int sock) {
    send_request(sock, "LOGOUT\r\n");
    read_lines_from_server(sock, RESP_NONE);
    return 0;
}

/**
 * @function handle_list_favorites: Handle list favorites
 */
void handle_list_favorites(int sock) {
    send_request(sock, "LIST_FAVORITES\r\n");
    read_lines_from_server(sock, RESP_FAVORITES);
}

/**
 * @function handle_add_favorite: Handle add favorite
 */
void handle_add_favorite(int sock, char *buff) {
    char name[128], category[128], location[256];
    print_header("Add Favorite Place");
    printf("║ Name: ");
    fgets(name, sizeof(name), stdin);
    name[strcspn(name, "\n")] = '\0';
    printf("║ Category: ");
    fgets(category, sizeof(category), stdin);
    category[strcspn(category, "\n")] = '\0';
    printf("║ Location: ");
    fgets(location, sizeof(location), stdin);
    location[strcspn(location, "\n")] = '\0';
    print_footer();
    snprintf(buff, BUFF_SIZE, "ADD_FAVORITE|%s|%s|%s\r\n", name, category, location);
    send_request(sock, buff);
    read_lines_from_server(sock, RESP_NONE);
    sleep(1);
}

void handle_edit_favorite(int sock, char *buff) {
    char id[16], name[128], category[128], location[256];
    printf("Your Favorite Places:\n");
    handle_list_favorites(sock);
    print_header("Edit Favorite Place");
    printf("║ Favorite ID: ");
    fgets(id, sizeof(id), stdin);
    id[strcspn(id, "\n")] = '\0';
    printf("║ New Name: ");
    fgets(name, sizeof(name), stdin);
    name[strcspn(name, "\n")] = '\0';
    printf("║ New Category: ");
    fgets(category, sizeof(category), stdin);
    category[strcspn(category, "\n")] = '\0';
    printf("║ New Location: ");
    fgets(location, sizeof(location), stdin);
    location[strcspn(location, "\n")] = '\0';
    print_footer();
    snprintf(buff, BUFF_SIZE, "EDIT_FAVORITE|%s|%s|%s|%s\r\n", id, name, category, location);
    send_request(sock, buff);
    read_lines_from_server(sock, RESP_NONE);
    sleep(1);
}

void handle_delete_favorite(int sock, char *buff) {
    char id[16];
    printf("Your Favorite Places:\n");
    handle_list_favorites(sock);
    print_header("Delete Favorite Place");
    printf("║ Favorite ID: ");
    fgets(id, sizeof(id), stdin);
    id[strcspn(id, "\n")] = '\0';
    print_footer();
    snprintf(buff, BUFF_SIZE, "DELETE_FAVORITE|%s\r\n", id);
    send_request(sock, buff);
    read_lines_from_server(sock, RESP_NONE);
    sleep(1);
}

void handle_list_tagged_favorites(int sock) {
    send_request(sock, "LIST_TAGGED_FAVORITES\r\n");
    read_lines_from_server(sock, RESP_TAGGED);
}
/**
 * @function handle_list_friends: Handle list friends
 */
void handle_list_friends(int sock) {
    send_request(sock, "LIST_FRIENDS\r\n");
    read_lines_from_server(sock, RESP_FRIENDS);
}

/**
 * @function handle_add_friend: Handle add friend
 */
void handle_add_friend(int sock, char *buff) {
    char target[128];
    print_header("Add Friend");
    printf("║ Friend username: ");
    fgets(target, sizeof(target), stdin);
    target[strcspn(target, "\n")] = '\0';
    print_footer();
    snprintf(buff, BUFF_SIZE, "ADD_FRIEND|%s\r\n", target);
    send_request(sock, buff);
    read_lines_from_server(sock, RESP_NONE);
    sleep(1);
}

void handle_accept_friend(int sock, char *buff) {
    char request_id_str[16];
    printf("Your friend requests:\n");
    handle_list_friend_requests(sock);
    print_header("Accept Friend Request");
    printf("║ Request ID: ");
    fgets(request_id_str, sizeof(request_id_str), stdin);
    request_id_str[strcspn(request_id_str, "\n")] = '\0';
    print_footer();
    snprintf(buff, BUFF_SIZE, "ACCEPT_FRIEND|%s\r\n", request_id_str);
    send_request(sock, buff);
    read_lines_from_server(sock, RESP_NONE);
    sleep(1);
}

void handle_reject_friend(int sock, char *buff) {
    char request_id_str[16];
    printf("Your friend requests:\n");
    handle_list_friend_requests(sock);
    print_header("Reject Friend Request");
    printf("║ Request ID: ");
    fgets(request_id_str, sizeof(request_id_str), stdin);
    request_id_str[strcspn(request_id_str, "\n")] = '\0';
    print_footer();
    snprintf(buff, BUFF_SIZE, "REJECT_FRIEND|%s\r\n", request_id_str);
    send_request(sock, buff);
    read_lines_from_server(sock, RESP_NONE);
    sleep(1);
}


void handle_remove_friend(int sock, char *buff) {
    char target[128];
    printf("Your Friends:\n");
    handle_list_friends(sock);
    print_header("Remove Friend");
    printf("║ Friend username: ");
    fgets(target, sizeof(target), stdin);
    target[strcspn(target, "\n")] = '\0';
    print_footer();
    snprintf(buff, BUFF_SIZE, "REMOVE_FRIEND|%s\r\n", target);
    send_request(sock, buff);
    read_lines_from_server(sock, RESP_NONE);
    sleep(1);
}

void handle_tag_friend_to_favorite(int sock, char *buff) {
    char friend_username[128];
    int fav_id;
    printf("Your Favorite Places:\n");
    handle_list_favorites(sock);
    printf("Your Friends:\n");
    handle_list_friends(sock);
    print_header("Tag Friend");
    printf("║ Friend username: ");
    fgets(friend_username, sizeof(friend_username), stdin);
    friend_username[strcspn(friend_username, "\n")] = '\0';
    printf("║ Location id: ");
    char fav_id_str[16];
    fgets(fav_id_str, sizeof(fav_id_str), stdin);
    fav_id_str[strcspn(fav_id_str, "\n")] = '\0';
    fav_id = atoi(fav_id_str);
    print_footer();
    snprintf(buff, BUFF_SIZE, "TAG_FRIEND|%d|%s\r\n",fav_id, friend_username);
    send_request(sock, buff);
    read_lines_from_server(sock, RESP_NONE);
    sleep(1);
}

void handle_list_friend_requests(int sock) {
    send_request(sock, "LIST_REQUESTS\r\n");
    read_lines_from_server(sock, RESP_REQUESTS);
}


void signal_handler(int sig) {
    handle_logout(clientfd);
    exit(0);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <domain_or_ip> <port>\n", argv[0]);
        return 1;
    }

    char *ip = argv[1];
    char *port = argv[2];
    char buff[BUFF_SIZE];

    struct sockaddr_in serverAddr;

    if((clientfd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        perror("Error: ");
        return 0;
    }

    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(ip);
    serverAddr.sin_port = htons(atoi(port));

    if(connect(clientfd, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) == -1){
        perror("connect() error.");
        return 0;
    }

    memset(buff, 0, BUFF_SIZE);
    int rcvBytes = recv(clientfd, buff, BUFF_SIZE, 0);
    if (rcvBytes <= 0) {
        printf("Failed to receive welcome message\n");
        close(clientfd);
        return 1;
    }
    buff[rcvBytes] = '\0';
    printf("%s", buff);

    signal(SIGINT, signal_handler);   
    signal(SIGTERM, signal_handler);  
    
    while (1) {
        char choice_str[16];
        int choice;
        display_main_menu();
        printf("Enter your choice: ");
        fgets(choice_str, sizeof(choice_str), stdin);
        choice = atoi(choice_str);

        switch (choice) {
            case 1:
                handle_login(clientfd, buff);
                break;
            case 2:
                handle_register(clientfd, buff);
                break;
            case 3:
                handle_list_favorites(clientfd);
                break;
            case 4:
                handle_add_favorite(clientfd, buff);
                break;
            case 5:
                handle_edit_favorite(clientfd, buff);
                break;
            case 6:
                handle_delete_favorite(clientfd, buff);
                break;
            case 7:
                handle_list_tagged_favorites(clientfd);
                break;
            case 8:
                handle_list_friends(clientfd);
                break;
            case 9:
                handle_add_friend(clientfd, buff);
                break;
            case 10:
                handle_list_friend_requests(clientfd);
                break;
            case 11:
                handle_accept_friend(clientfd, buff);
                break;
            case 12:
                handle_reject_friend(clientfd, buff);
                break;
            case 13:
                handle_remove_friend(clientfd, buff);
                break;
            case 14:
                handle_tag_friend_to_favorite(clientfd, buff);
                break;
            case 15:
                handle_logout(clientfd);
                break;
            default:
                printf("Invalid choice. Please try again.\n");
                break;
        }
        
    
    }

    

    close(clientfd);
    return 0;
}
