#include "command_handlers.h"
#include "ultilities.h"

#include <stdio.h>
#include <string.h>

static void handle_login(client_session_t *session, const char *payload);
static void handle_logout(client_session_t *session);
static void handle_register(client_session_t *session, const char *payload);
static void handle_add_favorite(client_session_t *session, const char *payload);
static void handle_add_friend(client_session_t *session, const char *payload);
static void handle_accept_friend(client_session_t *session, const char *payload);
static void handle_list_favorites(client_session_t *session, const char *payload);
static void handle_list_friends(client_session_t *session);
static void handle_list_friend_requests(client_session_t *session);
static void handle_remove_friend(client_session_t *session, const char *payload);
static void handle_reject_friend(client_session_t *session, const char *payload);
static void handle_delete_favorite(client_session_t *session, const char *payload);
static void handle_edit_favorite(client_session_t *session, const char *payload);
static void handle_list_tagged_favorites(client_session_t *session);
static void handle_tag_friend(client_session_t *session, const char *payload);
static void handle_not_implemented(client_session_t *session);

static void send_bad_request(client_session_t *session, const char *message) {
    char buff[128];
    snprintf(buff, sizeof(buff), "400 %s\r\n", message ? message : "Bad request");
    send_request(session->sockfd, buff);
}

void dispatch_command(client_session_t *session, const char *command) {
    if (!session || !command) {
        return;
    }
    if (strncmp(command, "LOGIN|", 6) == 0) {
        handle_login(session, command + 6);
    } else if (strncmp(command, "REGISTER|", 9) == 0) {
        handle_register(session, command + 9);
    } else if (strncmp(command, "LOGOUT", 7) == 0) {
        handle_logout(session);
    } else if (strncmp(command, "ADD_FAVORITE|", 13) == 0){
        handle_add_favorite(session, command + 13);
    } else if (strncmp(command, "LIST_FAVORITES", 14) == 0) {
        handle_list_favorites(session, command + 14);
    } else if (strncmp(command, "DEL_FAVORITE|", 12) == 0) {
        handle_delete_favorite(session, command + 12);  
    } else if (strncmp(command, "EDIT_FAVORITE|", 13) == 0 ){
        handle_edit_favorite(session, command + 13);
    } else if (strncmp(command, "LIST_TAGGED_FAVORITES", 21) == 0 ){
        handle_list_tagged_favorites(session);
    } else if(strncmp(command, "LIST_FRIEND_REQUESTS", 20) == 0 ){
        handle_list_friend_requests(session);
    }
    else if (strncmp(command, "ADD_FRIEND|", 11) == 0 ){
        handle_add_friend(session, command + 11);
    } else if (strncmp(command, "LIST_FRIENDS", 12) == 0 ){
        handle_list_friends(session);
    } else if (strncmp(command, "ACCEPT_FRIEND|", 14) == 0 ){
        handle_accept_friend(session, command + 14);
    } else if (strncmp(command, "LIST_REQUESTS", 13) == 0 ){
        handle_list_friend_requests(session);
    } else if (strncmp(command, "REJECT_FRIEND|", 14) == 0 ){
        handle_reject_friend(session, command + 14);
    } else if (strncmp(command, "REMOVE_FRIEND|", 14) == 0 ){
        handle_remove_friend(session, command + 14);
    } else if (strncmp(command, "TAG_FRIEND|", 11) == 0 ){
        handle_tag_friend(session, command + 11);
    } else if (strncmp(command, "LIST_NOTIFICATIONS", 19) == 0){
        handle_not_implemented(session);
    } else {
        send_bad_request(session, "Unknown command");
    }
    
}
// ACCOUNT COMMAND HANDLERS
static void handle_login(client_session_t *session, const char *payload) {
    char username[MAX_NAME_LEN];
    char password[MAX_PASS_LEN];
    if(session->logged_in){
        printf("402 Already logged in\n");
        send_request(session->sockfd, "406 Already logged in\r\n");
        return;
    }

    if (!payload || sscanf(payload, "%63[^|]|%127[^\r\n]", username, password) != 2) {
        printf("Invalid LOGIN format\n");
        send_bad_request(session, "Invalid LOGIN format");
        return;
    }

    Account acc;
    if (get_account(username, &acc) != 0) {
        printf("Account not found\n");
        send_request(session->sockfd, "401 Invalid username or password\r\n");
        return;
    }

    if (acc.is_logged_in) {
        printf("Account already logged in\n");
        send_request(session->sockfd, "411 Account already logged in by another user\r\n");
        return;
    }

    if (strcmp(acc.password, password) != 0) {
        printf("Invalid password\n");
        send_request(session->sockfd, "401 Invalid username or password\r\n");
        return;
    }

    db_update_logged_in_status(username, 1);
    session->logged_in = 1;
    strncpy(session->username, username, sizeof(session->username) - 1);
    session->username[sizeof(session->username) - 1] = '\0';

    printf("Login successful\n");
    send_request(session->sockfd, "200 Login successful\r\n");
}


static void handle_logout(client_session_t *session) {
    if (!session->logged_in) {
        printf("Not logged in\n");
        send_request(session->sockfd, "405 Not logged in\r\n");
        return;
    }
    db_update_logged_in_status(session->username, 0);
    session->logged_in = 0;
    session->username[0] = '\0';

    printf("Logout successful\n");
    send_request(session->sockfd, "200 Logout successful\r\n");
}

static void handle_register(client_session_t *session, const char *payload) {
    char username[MAX_NAME_LEN];
    char password[MAX_PASS_LEN];
    if(session->logged_in){
        printf("Already logged in\n");
        send_request(session->sockfd, "406 Already logged in\r\n");
        return;
    }

    if (!payload || sscanf(payload, "%63[^|]|%127[^\r\n]", username, password) != 2) {
        printf("Invalid REGISTER format\n");
        send_bad_request(session, "Invalid REGISTER format");
        return;
    }

    int result = create_account(username, password);
    if (result == 0) {
        printf("Register successful\n");
        send_request(session->sockfd, "200 Register successful\r\n");
    } else if (result == -2) {
        printf("Username already exists\n");
        send_request(session->sockfd, "404 Username already exists\r\n");
    } else if (result == -1) {
        printf("Server full, cannot register\n");
        send_request(session->sockfd, "500 Server full, cannot register\r\n");
    } else {
        printf("Internal server error during registration\n");
        send_request(session->sockfd, "500 Internal server error\r\n");
    }
}

// FAVORITE COMMAND HANDLERS
static void handle_add_favorite(client_session_t *session, const char *payload) {
    char owner[MAX_NAME_LEN];
    char name[MAX_TITLE_LEN];
    char category[MAX_CAT_LEN];
    char location[MAX_DESC_LEN];

    if(!session->logged_in){
        printf("[ADD_FAVORITE] Failed - Not logged in\n");
        send_request(session->sockfd, "405 Not logged in\r\n");
        return;
    }

    if (!payload || sscanf(payload, "%127[^|]|%63[^|]|%255[^\r\n]", name, category, location) != 3) {
        printf("[ADD_FAVORITE] Failed - Invalid format\n");
        send_bad_request(session, "Invalid ADD_FAVORITE format");
        return;
    }

    int result = create_favorite(session->username, name, category, location);
    if (result == 0) {
        printf("[ADD_FAVORITE] Success - owner:%s, name:%s, category:%s\n", session->username, name, category);
        send_request(session->sockfd, "200 Favorite added successfully\r\n");
    } else if (result == -1) {
        printf("[ADD_FAVORITE] Failed - Internal server error\n");
        send_request(session->sockfd, "500 Internal server error\r\n");
    } else if (result == -2) {
        printf("[ADD_FAVORITE] Failed - User not found: %s\n", owner);
        send_request(session->sockfd, "404 User not found\r\n");
    }
}

static void handle_list_favorites(client_session_t *session, const char *payload) {
    (void)payload;
    if (!session->logged_in) {
        printf("[LIST_FAVORITES] Failed - Not logged in\n");
        send_request(session->sockfd, "405 Not logged in\r\n");
        return;
    }

    FavoritePlace favs[MAX_FAVS];
    int fav_count = 0;
    int rc = get_user_favorites(session->username, favs, MAX_FAVS, &fav_count);
    if (rc == -2) {
        printf("[LIST_FAVORITES] Failed - User not found\n");
        send_request(session->sockfd, "404 User not found\r\n");
        return;
    } else if (rc == -1) {
        printf("[LIST_FAVORITES] Failed - Internal error\n");
        send_request(session->sockfd, "500 Internal server error\r\n");
        return;
    }

    printf("[LIST_FAVORITES] Found %d favorites\n", fav_count);
    char buff[8192];
    int offset = snprintf(buff, sizeof(buff), "200 %d favorites found\r\n", fav_count);
    for (int i = 0; i < fav_count; ++i) {
        offset += snprintf(buff + offset, sizeof(buff) - offset,
                           "%d|%s|%s|%s|%s|%ld\r\n",
                           favs[i].id,
                           favs[i].owner,
                           favs[i].name,
                           favs[i].category,
                           favs[i].location,
                           (long)favs[i].created_at);
        if (offset >= (int)sizeof(buff)) {
            break;
        }
    }
    offset += snprintf(buff + offset, sizeof(buff) - offset,
                   "END\r\n");

    printf("[LIST_FAVORITES] Sending response %s\n", buff);

    send_request(session->sockfd, buff);
}


static void handle_edit_favorite(client_session_t *session, const char *payload) {
    int fav_id = 0;
    char owner[MAX_NAME_LEN];
    char name[MAX_TITLE_LEN];
    char category[MAX_CAT_LEN];
    char location[MAX_DESC_LEN];

    if (!session->logged_in) {
        printf("[EDIT_FAVORITE] Failed - Not logged in\n");
        send_request(session->sockfd, "405 Not logged in\r\n");
        return;
    }

    if (!payload || sscanf(payload, "%d|%63[^|]|%127[^|]|%63[^|]|%255[^\r\n]", &fav_id, owner, name, category, location) != 5) {
        printf("[EDIT_FAVORITE] Failed - Invalid format\n");
        send_bad_request(session, "Invalid EDIT_FAVORITE format");
        return;
    }

    int rc = update_favorite(fav_id, owner, name, category, location);
    if (rc == 0) {
        printf("[EDIT_FAVORITE] Success - fav_id:%d, user:%s\n", fav_id, session->username);
        send_request(session->sockfd, "200 Favorite updated successfully\r\n");
    } else if (rc == -2) {
        printf("[EDIT_FAVORITE] Failed - Favorite not found: %d\n", fav_id);
        send_request(session->sockfd, "406 Favorite not exist\r\n");
    } else if (rc == -1) {
        printf("[EDIT_FAVORITE] Failed - Internal server error\n");
        send_request(session->sockfd, "500 Internal server error\r\n");
    } else if (rc == -3) {
        printf("[EDIT_FAVORITE] Failed - No changes made\n");
        send_request(session->sockfd, "407 No changes made to favorite\r\n");
    }
}

static void handle_delete_favorite(client_session_t *session, const char *payload) {
    int fav_id = 0;

    if (!session->logged_in) {
        printf("[DEL_FAVORITE] Failed - Not logged in\n");
        send_request(session->sockfd, "405 Not logged in\r\n");
        return;
    }

    if (!payload || sscanf(payload, "%d[^\r\n]", &fav_id) != 1) {
        printf("[DEL_FAVORITE] Failed - Invalid format\n");
        send_bad_request(session, "Invalid DEL_FAVORITE format");
        return;
    }

    int rc = delete_favorite(fav_id, session->username);
    if (rc == 0) {
        printf("[DEL_FAVORITE] Success - fav_id:%d, user:%s\n", fav_id, session->username);
        send_request(session->sockfd, "200 Favorite deleted successfully\r\n");
    } else if (rc == -2) {
        printf("[DEL_FAVORITE] Failed - Favorite not found: %d\n", fav_id);
        send_request(session->sockfd, "406 Favorite not exist\r\n");
    } else if (rc == -1) {
        printf("[DEL_FAVORITE] Failed - Internal server error\n");
        send_request(session->sockfd, "500 Internal server error\r\n");
    } 
}

static void handle_list_tagged_favorites(client_session_t *session) {
    if (!session->logged_in) {
        printf("[LIST_TAGGED_FAVORITES] Failed - Not logged in\n");
        send_request(session->sockfd, "405 Not logged in\r\n");
        return;
    }

    FavoritePlaceWithTags favs[MAX_FAVS];
    int fav_count = 0;
    int rc = get_tagged_favorites(session->username, favs, MAX_FAVS, &fav_count);
    if (rc == -2) {
        printf("[LIST_TAGGED_FAVORITES] Failed - User not found\n");
        send_request(session->sockfd, "406 User not exist\r\n");
        return;
    } else if (rc == -1) {
        printf("[LIST_TAGGED_FAVORITES] Failed - Internal server error\n");
        send_request(session->sockfd, "500 Internal server error\r\n");
        return;
    }

    char buff[8192];
    int offset = snprintf(buff, sizeof(buff), "200 %d favorites found\r\n", fav_count);
    for (int i = 0; i < fav_count; ++i) {
        offset += snprintf(buff + offset, sizeof(buff) - offset,
                           "%d|%s|%s|%s|%s|%ld|%s\r\n",
                           favs[i].id,
                           favs[i].owner,
                           favs[i].name,
                           favs[i].category,
                           favs[i].location,
                           (long)favs[i].created_at,
                           favs[i].tagger);
        if (offset >= (int)sizeof(buff)) {
            break;
        }
    }
    offset += snprintf(buff + offset, sizeof(buff) - offset,
                   "END\r\n");
    printf("[LIST_TAGGED_FAVORITES] Sending response %s\n", buff);
    send_request(session->sockfd, buff);
}
// FRIEND COMMAND HANDLERS
static void handle_add_friend(client_session_t *session, const char *payload) {
    if (!session->logged_in) {
        printf("[ADD_FRIEND] Failed - Not logged in\n");
        send_request(session->sockfd, "405 Not logged in\r\n");
        return;
    }

    char target[MAX_NAME_LEN];
    if (!payload || sscanf(payload, "%63[^\r\n]", target) != 1) {
        printf("[ADD_FRIEND] Failed - Invalid format\n");
        send_bad_request(session, "Invalid ADD_FRIEND format");
        return;
    }

    if (strcmp(target, session->username) == 0) {
        printf("[ADD_FRIEND] Failed - Cannot friend yourself\n");
        send_bad_request(session, "Cannot friend yourself");
        return;
    }

    Account target_acc;
    if (get_account(target, &target_acc) != 0) {
        printf("[ADD_FRIEND] Failed - User not found: %s\n", target);
        send_request(session->sockfd, "406 User not exist\r\n");
        return;
    }

    if(check_friendship(session->username, target) == 1 ) {
        printf("[ADD_FRIEND] Failed - Already friends\n");
        send_request(session->sockfd, "407 Already friends\r\n");
        return;
    }

    if(db_check_duplicate_friend_request(session->username, target) == 1){
        printf("[ADD_FRIEND] Failed - Friend request already sent\n");
        send_request(session->sockfd, "409 Friend request already sent\r\n");
        return;
    }

    int rc = create_friend_request(session->username, target);
    if (rc != 0) {
        printf("[ADD_FRIEND] Failed - Create request error\n");
        send_request(session->sockfd, "500 Internal server error\r\n");
        return;
    }
    printf("[ADD_FRIEND] Success - friend request sent to: %s\n", target);
    send_request(session->sockfd, "200 Friend request sent\r\n");
}

static void handle_accept_friend(client_session_t *session, const char *payload) {
    if (!session->logged_in) {
        printf("[ACCEPT_FRIEND] Failed - Not logged in\n");
        send_request(session->sockfd, "405 Not logged in\r\n");
        return;
    }

    int request_id = 0;
    if (!payload || sscanf(payload, "%d", &request_id) != 1 || request_id <= 0) {
        printf("[ACCEPT_FRIEND] Failed - Invalid format\n");
        send_bad_request(session, "Invalid ACCEPT_FRIEND format");
        return;
    }

    FriendRequest request;
    int rc = get_friend_request_by_id(request_id, session->username, &request);
    if (rc == -2) {
        printf("[ACCEPT_FRIEND] Failed - Request not found: %d\n", request_id);
        send_request(session->sockfd, "406 Request not exist\r\n");
        return;
    } else if (rc != 0) {
        printf("[ACCEPT_FRIEND] Failed - Fetch request error\n");
        send_request(session->sockfd, "500 Internal server error\r\n");
        return;
    }

    printf("Request to: %s, session user: %s\n", request.to, session->username);
    if (strcmp(request.to, session->username) != 0) {
        printf("[ACCEPT_FRIEND] Failed - Not authorized\n");
        send_request(session->sockfd, "403 Not authorized to accept this request\r\n");
        return;
    }

    if (request.status != 0) {
        printf("[ACCEPT_FRIEND] Failed - Request already accepted\n");
        send_request(session->sockfd, "409 Accept request already sent\r\n");
        return;
    }

    rc = accept_friend_request(request_id, session->username);;
    if (rc != 0) {
        printf("[ACCEPT_FRIEND] Failed - Accept error\n");
        send_request(session->sockfd, "500 Internal server error\r\n");
        return;
    }

    printf("[ACCEPT_FRIEND] Success - request_id:%d\n", request_id);
    send_request(session->sockfd, "200 Accept friend successful\r\n");
}

static void handle_tag_friend(client_session_t *session, const char *payload) {
    if (!session->logged_in) {
        printf("[TAG_FRIEND] Failed - Not logged in\n");
        send_request(session->sockfd, "405 Not logged in\r\n");
        return;
    }

    int fav_id = 0;
    char tagged_user[MAX_TAGGED_LEN];
    printf("Payload: %s\n", payload);
    if (!payload || sscanf(payload, "%d|%63[^\r\n]", &fav_id, tagged_user) != 2) {
        printf("[TAG_FRIEND] Failed - Invalid format\n");
        send_bad_request(session, "Invalid TAG_FRIEND format");
        return;
    }
    
    FavoritePlace fav;
    if(get_favorite_by_id(fav_id,session->username ,&fav) != 0){
        printf("[TAG_FRIEND] Failed - Favorite not found: %d\n", fav_id);
        send_request(session->sockfd, "406 Favorite not exist\r\n");
        return;
    }
    Account acc;
    if(get_account(tagged_user, &acc) != 0){
        printf("[TAG_FRIEND] Failed - User not found: %s\n", tagged_user);
        send_request(session->sockfd, "406 User not exist\r\n");
        return;
    }

    if(check_friendship(session->username, tagged_user) != 1 ) {
        printf("[TAG_FRIEND] Failed - Not friends with user: %s\n", tagged_user);
        send_request(session->sockfd, "410 Not friends with the user\r\n");
        return;
    }

    int rc = tag_favorite(fav_id, session->username, tagged_user);
    if (rc == -2) {
        printf("[TAG_FRIEND] Failed - Favorite not found: %d\n", fav_id);
        send_request(session->sockfd, "411 You already tagged them to this place\r\n");
        return;
    } else if (rc != 0) {
        printf("[TAG_FRIEND] Failed - Tag error\n");
        send_request(session->sockfd, "500 Internal server error\r\n");
        return;
    }

    printf("[TAG_FRIEND] Success - fav_id:%d, user:%s\n", fav_id, tagged_user);
    send_request(session->sockfd, "200 Tag friend successful\r\n");
}


static void handle_reject_friend(client_session_t *session, const char *payload) {
    if (!session->logged_in) {
        printf("[REJECT_FRIEND] Failed - Not logged in\n");
        send_request(session->sockfd, "405 Not logged in\r\n");
        return;
    }

    int request_id = 0;
    if (!payload || sscanf(payload, "%d", &request_id) != 1 || request_id <= 0) {
        printf("[REJECT_FRIEND] Failed - Invalid format\n");
        send_bad_request(session, "Invalid REJECT_FRIEND format");
        return;
    }

    FriendRequest request;
    int rc = get_friend_request_by_id(request_id,session->username ,&request);
    if (rc == -2) {
        printf("[REJECT_FRIEND] Failed - Request not found: %d\n", request_id);
        send_request(session->sockfd, "406 Request not exist\r\n");
        return;
    } else if (rc != 0) {
        printf("[REJECT_FRIEND] Failed - Fetch request error\n");
        send_request(session->sockfd, "500 Internal server error\r\n");
        return;
    }
    
    if (strcmp(request.to, session->username) != 0) {
        printf("[REJECT_FRIEND] Failed - Not authorized\n");
        send_request(session->sockfd, "403 Not authorized to reject this request\r\n");
        return;
    }

    if (request.status != 0) {
        printf("[REJECT_FRIEND] Failed - Request already processed\n");
        send_request(session->sockfd, "409 reject request already sent\r\n");
        return;
    }

    rc = reject_friend_request(request_id, session->username);;
    if (rc != 0) {
        printf("[REJECT_FRIEND] Failed - Reject error\n");
        send_request(session->sockfd, "500 Internal server error\r\n");
        return;
    }

    printf("[REJECT_FRIEND] Success - request_id:%d\n", request_id);
    send_request(session->sockfd, "200 Reject friend successful\r\n");
}

static void handle_remove_friend(client_session_t *session, const char *payload) {
    if (!session->logged_in) {
        printf("[REMOVE_FRIEND] Failed - Not logged in\n");
        send_request(session->sockfd, "405 Not logged in\r\n");
        return;
    }

    char target[MAX_NAME_LEN];
    if (!payload || sscanf(payload, "%63[^\r\n]", target) != 1) {
        printf("[REMOVE_FRIEND] Failed - Invalid format\n");
        send_bad_request(session, "Invalid REMOVE_FRIEND format");
        return;
    }

    int rc = remove_friendship(session->username, target);
    if (rc == -2) {
        printf("[REMOVE_FRIEND] Failed - User not found: %s\n", target);
        send_request(session->sockfd, "406 User not exist\r\n");
        return;
    } else if (rc != 0) {
        printf("[REMOVE_FRIEND] Failed - Remove error\n");
        send_request(session->sockfd, "500 Internal server error\r\n");
        return;
    }

    printf("[REMOVE_FRIEND] Success - removed friend: %s\n", target);
    send_request(session->sockfd, "200 Remove friend successful\r\n");
}

static void handle_list_friends(client_session_t *session) {
    printf("[LIST_FRIENDS] Command received from user: %s\n", session->username);
    if (!session->logged_in) {
        printf("[LIST_FRIENDS] Failed - Not logged in\n");
        send_request(session->sockfd, "405 Not logged in\r\n");
        return;
    }

    FriendRel friends[MAX_FRIENDS];
    int friend_count = 0;
    int rc = get_user_friends(session->username, friends, MAX_FRIENDS, &friend_count);
    if (rc == -2) {
        printf("[LIST_FRIENDS] Failed - User not found\n");
        send_request(session->sockfd, "406 User not exist\r\n");

        return;
    } else if (rc == -1) {
        printf("[LIST_FRIENDS] Failed - Fetch error\n");
        send_request(session->sockfd, "500 Internal server error\r\n");
        return;
    }

    printf("[LIST_FRIENDS] Found %d friends\n", friend_count);
    char buff[8192];
    int offset = snprintf(buff, sizeof(buff), "200 List friend successful, %d friends\r\n", friend_count);
    for (int i = 0; i < friend_count; ++i) {
        offset += snprintf(buff + offset, sizeof(buff) - offset,
                           "%s|%s|%ld\r\n",
                           friends[i].user_a,
                           friends[i].user_b,
                           (long)friends[i].since);
        if (offset >= (int)sizeof(buff)) {
            break;
        }
    }
    offset += snprintf(buff + offset, sizeof(buff) - offset,
                   "END\r\n");


    printf("[LIST_FRIENDS] Sending response with %d items\n", friend_count);
    send_request(session->sockfd, buff);
}

static void handle_list_friend_requests(client_session_t *session) {
    printf("[LIST_REQUESTS] Command received from user: %s\n", session->username);
    if (!session->logged_in) {
        printf("[LIST_REQUESTS] Failed - Not logged in\n");
        send_request(session->sockfd, "405 Not logged in\r\n");
        return;
    }

    FriendRequest requests[MAX_REQUESTS];
    int req_count = 0;
    int rc = get_user_requests(session->username, requests, MAX_REQUESTS, &req_count);
    if (rc == -2) {
        printf("[LIST_REQUESTS] Failed - User not found\n");
        send_request(session->sockfd, "406 User not exist\r\n");
        return;
    } else if (rc == -1) {
        printf("[LIST_REQUESTS] Failed - Fetch error\n");
        send_request(session->sockfd, "500 Internal server error\r\n");
        return;
    }

    printf("[LIST_REQUESTS] Found %d requests\n", req_count);
    char buff[8192];
    int offset = snprintf(buff, sizeof(buff), "200 List request successful, %d requests\r\n", req_count);
    for (int i = 0; i < req_count; ++i) {
        offset += snprintf(buff + offset, sizeof(buff) - offset,
                           "%d|%s|%s|%d|%ld\r\n",
                           requests[i].id,
                           requests[i].from,
                           requests[i].to,
                           requests[i].status,
                           (long)requests[i].created_at);
        if (offset >= (int)sizeof(buff)) {
            break;
        }
    }
    offset += snprintf(buff + offset, sizeof(buff) - offset,
                   "END\r\n");

    send_request(session->sockfd, buff);
}

static void handle_not_implemented(client_session_t *session) {
    send_request(session->sockfd, "501 Not implemented\r\n");
}
