#ifndef TCP_SERVER_ULTILITIES_H
#define TCP_SERVER_ULTILITIES_H

#include <stddef.h>
#include "../entity/entities.h"
#define MSSV "20225690"
#define MAX_USER 3000
#define MAX_SESSION 3000
#define MAX_FAVS 128
#define MAX_FRIENDS 128
#define MAX_REQUESTS 128
#define MAX_NOTIFS 128
// DATA STORE MANAGEMENT FUNCTIONS
int init_data_store(const char *db_path);
void shutdown_data_store(void);
// NETWORK COMMUNICATION FUNCTIONS
int send_request(int sockfd, const char *buf);
int recv_response(int sockfd, char *buff, size_t size);
// ACCOUNT MANAGEMENT FUNCTIONS
int get_accounts(Account accounts[], int max_users, int *out_count);
int get_account(const char *username, Account *out_account);
int create_account(const char *username, const char *password);

// FAVORITE MANAGEMENT FUNCTIONS
int get_user_favorites(const char *username, FavoritePlace favs[], int max, int *out_count);
int create_favorite(const char *owner, const char *name, const char *category, const char *location);
int update_favorite(int fav_id, const char *owner, const char *name, const char *category, const char *location);
int delete_favorite(int fav_id, const char *owner);
int get_favorite_by_id(int fav_id, char *username ,FavoritePlace *out_fav);
int get_tagged_favorites(const char *username, FavoritePlaceWithTags favs[], int max, int *out_count);

// FRIEND MANAGEMENT FUNCTIONS
int get_user_friends(const char *username, FriendRel frs[], int max, int *out_count);
int get_user_requests(const char *username, FriendRequest reqs[], int max, int *out_count);
int create_friend_request(const char *from, const char *to);
int get_friend_request_by_id(int request_id, char * username ,FriendRequest *out_request);
int accept_friend_request(int request_id, const char *username);
int reject_friend_request(int request_id, const char *username);
int check_friendship(const char *user_a, const char *user_b);
int remove_friendship(const char *user_a, const char *user_b);
int tag_favorite(int fav_id, const char *tagger, const char *tagged_users);

// NOTIFICATION MANAGEMENT FUNCTIONS
int get_user_notifications(const char *username, Notification notifs[], int max, int *out_count);
int mark_notification_as_seen(int notif_id);
int create_notification(const char *to, const char *from, int fav_id, const char *message);

#endif 
