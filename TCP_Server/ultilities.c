#include "ultilities.h"
#include "database.h"

#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>


extern pthread_mutex_t account_lock;
/**
 * @function writeLog: Write request and response information into log file.
 *
 * @param client_addr: Client IP:Port address
 * @param request: Request received from client
 * @param result: Result of processing
 */
void writeLog(int sockfd, const char *buff, const char *type) {
    char filename[64];
    snprintf(filename, sizeof(filename), "log_%s.txt", MSSV);

    FILE *f = fopen(filename, "a");
    if (!f) {
        perror("Cannot open log file");
        return;
    }
	char ip[INET_ADDRSTRLEN];
	struct sockaddr_in addr;
	socklen_t addr_len = sizeof(addr);
	char client_addr[32] = "UNKNOWN";

	if (getpeername(sockfd, (struct sockaddr*)&addr, &addr_len) == 0) {
		inet_ntop(AF_INET, &addr.sin_addr, ip, sizeof(ip));
		int port = ntohs(addr.sin_port);
		snprintf(client_addr, sizeof(client_addr), "%s:%d", ip, port);
	}

    time_t now = time(NULL);
    struct tm *t = localtime(&now);

    fprintf(f, "[%02d/%02d/%04d %02d:%02d:%02d]$%s$%s$%s\n",
            t->tm_mday,
            t->tm_mon + 1,
            t->tm_year + 1900,
            t->tm_hour,
            t->tm_min,
            t->tm_sec,
            client_addr ? client_addr : "",
            type ? type : "",
            buff ? buff : "");

    fclose(f);
}



// DATA HELPER FUNCTIONS
int init_data_store(const char *db_path) {
	const char *path = db_path ? db_path : "data/mmt.db";
	return db_initialize(path);
}

void shutdown_data_store(void) {
	db_shutdown();
}

// NETWORK COMMUNICATION FUNCTIONS
int send_request(int sockfd, const char *buf) {
	if (!buf) {
		errno = EINVAL;
		return -1;
	}
	ssize_t s = send(sockfd, buf, strlen(buf), 0);
	if (s == -1) {
		perror("send() error");
		return -1;
	}
	writeLog(sockfd, buf, "REQUEST");
	//usleep(2000);
	return (int)s;
}

int recv_response(int sockfd, char *buff, size_t size) {
	if (!buff || size == 0) {
		errno = EINVAL;
		return -1;
	}
	ssize_t r = recv(sockfd, buff, size - 1, 0);
	if (r <= 0) {
		if (r < 0) perror("recv() error");
		return (int)r;
	}
	buff[r] = '\0';
	writeLog(sockfd, buff, "RESPONSE");
	return (int)r;
}


// ACCOUNT MANAGEMENT FUNCTIONS
int get_accounts(Account accounts_buffer[], int max_users, int *out_count) {
	if (!accounts_buffer || !out_count || max_users <= 0) return -1;
	return db_fetch_accounts(accounts_buffer, max_users, out_count);
}

int get_account(const char *username, Account *out_account) {
	if (!username || !out_account) return -1;
	return db_fetch_account(username, out_account);
}

int create_account(const char *username, const char *password) {
	if (!username || !password) return -1;

	return db_create_account(username, password);
}





// FAVORITE MANAGEMENT FUNCTIONS
int get_user_favorites(const char *username, FavoritePlace favs[], int max, int *out_count) {
	if (!username || !out_count || max <= 0) return -1;
	if (!favs) {
		*out_count = 0;
		return 0;
	}
	return db_fetch_user_favorites(username, favs, max, out_count);
}

int create_favorite(const char *owner, const char *name, const char *category, const char *location) {
	if (!owner || !name || !category || !location) return -1;
	return db_create_favorite(owner, name, category, location);
}

int update_favorite(int fav_id, const char *owner, const char *name, const char *category, const char *location) {
	if (fav_id <= 0 || !owner || !name || !category || !location) return -1;
	return db_update_favorite(fav_id, owner, name, category, location);
}

int delete_favorite(int fav_id, const char *owner) {
	if(fav_id <= 0 || !owner) return -1;
	return db_delete_favorite(fav_id, owner);
}

int get_favorite_by_id(int fav_id,char *username, FavoritePlace *out_fav) {
	if (fav_id <= 0 || !out_fav || !username) return -1;
	return db_fetch_favorite_by_id(fav_id, username,out_fav);
}

int get_tagged_favorites(const char *username, FavoritePlaceWithTags favs[], int max, int *out_count) {
	if (!username || !out_count || max <= 0) return -1;
	if (!favs) {
		*out_count = 0;
		return 0;
	}
	return db_fetch_tagged_favorites(username, favs, max, out_count);
}

// FRIEND MANAGEMENT FUNCTIONS
int get_user_friends(const char *username, FriendRel frs[], int max, int *out_count) {
	if (!username || !out_count || max <= 0) return -1;
	if (!frs) {
		*out_count = 0;
		return 0;
	}
	return db_fetch_user_friends(username, frs, max, out_count);
}

int get_user_requests(const char *username, FriendRequest reqs[], int max, int *out_count) {
	if (!username || !out_count || max <= 0) return -1;
	if (!reqs) {
		*out_count = 0;
		return 0;
	}
	return db_fetch_user_requests(username, reqs, max, out_count);
}

int create_friend_request(const char *from, const char *to) {
	if (!from || !to) return -1;
	return db_create_friend_request(from, to);
}

int get_friend_request_by_id(int request_id, char *username, FriendRequest *out_request) {
	if (!out_request || request_id <= 0 || !username) return -1;
	return db_fetch_friend_request_by_id(request_id, username, out_request);
	return 0;
}

int accept_friend_request(int request_id, const char *requestee) {
	if (!requestee || request_id <= 0) return -1;
	return db_accept_friend_request(request_id, requestee);
}

int reject_friend_request(int request_id, const char *requestee) {
	if (!requestee || request_id <= 0) return -1;
	return db_reject_friend_request(request_id, requestee);
}

int remove_friendship(const char *user_a, const char *user_b) {
	if (!user_a || !user_b) return -1;
	return db_remove_friendship(user_a, user_b);
}

int check_friendship(const char *user_a, const char *user_b) {
	if (!user_a || !user_b) return -1;
	return db_check_friendship(user_a, user_b);
}

int tag_favorite(int fav_id, const char *tagger, const char *tagged_users) {
	if (fav_id <= 0 || !tagger || !tagged_users) return -1;
	return db_tag_friend_to_favorite(fav_id, tagger, tagged_users);
}


// NOTIFICATION MANAGEMENT FUNCTIONS
int get_user_notifications(const char *username, Notification notifs[], int max, int *out_count) {
	if (!username || !out_count || max <= 0) return -1;
	if (!notifs) {
		*out_count = 0;
		return 0;
	}
	return db_fetch_user_notifications(username, notifs, max, out_count);
}

int mark_notification_seen(int notif_id) {
	// implement later
	return 0;
}

int create_notification(const char *to, const char *from, int fav_id, const char *message) {
	// implement later
	return 0;
}
