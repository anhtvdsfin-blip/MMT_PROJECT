#ifndef TCP_SERVER_DATABASE_H
#define TCP_SERVER_DATABASE_H

#include <time.h>
#include <stddef.h>

#include "../entity/entities.h"
// Database initialization and shutdown
int db_initialize(const char *db_path);
void db_shutdown(void);

// Account management functions
int db_fetch_account(const char *username, Account *out_account);
int db_fetch_accounts(Account accounts[], int max_users, int *out_count);
int db_create_account(const char *username, const char *password);

// Favorite management functions
int db_fetch_user_favorites(const char *owner, FavoritePlace favs[], int max_items, int *out_count);
int db_fetch_favorite_by_id(int fav_id, char *username ,FavoritePlace *out_fav);
int db_create_favorite(const char *owner, const char *name, const char *category, const char *location);
int db_update_favorite(int fav_id, const char *owner, const char *name, const char *category, const char *location);
int db_delete_favorite(int fav_id, const char *owner);
int db_fetch_tagged_favorites(const char *username, FavoritePlaceWithTags favs[], int max_items, int *out_count);
// Friend management functions
int db_fetch_user_friends(const char *username, FriendRel friends[], int max_items, int *out_count);
int db_fetch_user_requests(const char *username, FriendRequest requests[], int max_items, int *out_count);
int db_fetch_user_notifications(const char *username, Notification notifications[], int max_items, int *out_count);
int db_create_friend_request(const char *from, const char *to);
int db_get_friend_request(int request_id, FriendRequest *out_request);
int db_accept_friend_request(int request_id, const char *requestee);
int db_reject_friend_request(int request_id, const char *requestee);
int db_check_friendship(const char *user_a, const char *user_b);
int db_remove_friendship(const char *user_a, const char *user_b);
int db_tag_friend_to_favorite(int fav_id, const char *tagger, const char *tagged_users);

// Notification management functions
int db_mark_notification_seen(int notif_id);
int db_create_notification(const char *to, const char *from, int fav_id, const char *message);
int db_fetch_user_notifications(const char *username, Notification notifications[], int max_items, int *out_count);

#endif /* TCP_SERVER_DATABASE_H */
