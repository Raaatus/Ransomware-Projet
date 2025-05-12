#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <arpa/inet.h>
#include <openssl/evp.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 4242
#define PROJECT_PATH "TP/Projet"

int should_decrypt(const char *filename) {
    const char *ext = strrchr(filename, '.');
    return ext && strcmp(ext, ".enc") == 0;
}

void decrypt_file(const char *filepath, const unsigned char *key, const unsigned char *iv) {
    FILE *in = fopen(filepath, "rb");
    if (!in) return;

    char outpath[512];
    strncpy(outpath, filepath, strlen(filepath) - 4);
    outpath[strlen(filepath) - 4] = '\0';
    FILE *out = fopen(outpath, "wb");
    if (!out) { fclose(in); return; }

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv);

    unsigned char inbuf[1024], outbuf[1040];
    int inlen, outlen;
    while ((inlen = fread(inbuf, 1, 1024, in)) > 0) {
        EVP_DecryptUpdate(ctx, outbuf, &outlen, inbuf, inlen);
        fwrite(outbuf, 1, outlen, out);
    }
    EVP_DecryptFinal_ex(ctx, outbuf, &outlen);
    fwrite(outbuf, 1, outlen, out);

    EVP_CIPHER_CTX_free(ctx);
    fclose(in);
    fclose(out);
    remove(filepath);
}

int main() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        return 1;
    }
    
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr);
    
    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("connect");
        return 1;
    }
    
    char message[1024];
    printf("Rédigez vos excuses (min 20 caractères) :\n> ");
    fgets(message, sizeof(message), stdin);
    write(sock, message, strlen(message));
    
    // First read the response status (OK or NO)
    char status[3] = {0};
    int status_len = read(sock, status, 2);
    status[status_len] = '\0';
    
    if (strcmp(status, "OK") == 0) {
        printf("Excuses acceptées\n");
        
        // Now read the key and IV (32+16 bytes)
        unsigned char key[32], iv[16];
        int key_len = read(sock, key, 32);
        int iv_len = read(sock, iv, 16);
        
        if (key_len == 32 && iv_len == 16) {
            printf("Clé et IV reçus. Déchiffrement en cours...\n");
            
            DIR *pdir = opendir(PROJECT_PATH);
            struct dirent *entry;
            while ((entry = readdir(pdir))) {
                if (entry->d_type == DT_REG && should_decrypt(entry->d_name)) {
                    char filepath[512];
                    snprintf(filepath, sizeof(filepath), "%s/%s", PROJECT_PATH, entry->d_name);
                    decrypt_file(filepath, key, iv);
                }
            }
            closedir(pdir);
            printf("Déchiffrement terminé.\n");
        } else {
            printf("Erreur: Impossible de recevoir la clé et l'IV correctement.\n");
        }
    } else {
        printf("Réponse du serveur : %s\n", status);
        printf("Excuses refusées. Veuillez réessayer avec un message plus long.\n");
    }
    
    close(sock);
    return 0;
}
