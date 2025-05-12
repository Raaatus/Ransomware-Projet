#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 4242

unsigned char stored_key[32];
unsigned char stored_iv[16];
int key_ready = 0;

void save_key_iv_to_disk() {
    FILE *kf = fopen("key.bin", "wb");
    FILE *ivf = fopen("iv.bin", "wb");
    if (kf && ivf) {
        fwrite(stored_key, 1, 32, kf);
        fwrite(stored_iv, 1, 16, ivf);
        fclose(kf);
        fclose(ivf);
        printf("Clé et IV sauvegardés dans key.bin / iv.bin\n");
    } else {
        perror("Erreur");
    }
}

void load_key_iv_from_disk() {
    FILE *kf = fopen("key.bin", "rb");
    FILE *ivf = fopen("iv.bin", "rb");
    if (kf && ivf) {
        fread(stored_key, 1, 32, kf);
        fread(stored_iv, 1, 16, ivf);
        fclose(kf);
        fclose(ivf);
        key_ready = 1;
    }
}

void handle_client(int client_sock) {
    char buffer[1024] = {0};

    read(client_sock, buffer, 3);
    if (strncmp(buffer, "KEY", 3) == 0) {
        read(client_sock, stored_key, 32);
        read(client_sock, stored_iv, 16);
        key_ready = 1;
        save_key_iv_to_disk();
        printf("Clé et IV reçus\n");
        close(client_sock);
        return;
    }

    printf("client connecté. Attente des excuses...\n");
    int len = read(client_sock, buffer, sizeof(buffer) - 1);
    buffer[len] = '\0';

    if (len >= 20 && key_ready) {
        printf("Excuses OK : %s\n", buffer);
        write(client_sock, "OK", 2);
        write(client_sock, stored_key, 32);
        write(client_sock, stored_iv, 16);
    } else {
        printf("Excuses insuffisantes (%d caractères)\n", len);
        write(client_sock, "NO", 2);
    }

    close(client_sock);
}

int main() {
    int server_fd, client_sock;
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);

    load_key_iv_from_disk();

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    listen(server_fd, 3);
    printf("En attente de connexions sur le port %d...\n", PORT);

    while (1) {
        client_sock = accept(server_fd, (struct sockaddr *)&address, &addrlen);
        if (client_sock >= 0) {
            handle_client(client_sock);
        }
    }

    return 0;
}
