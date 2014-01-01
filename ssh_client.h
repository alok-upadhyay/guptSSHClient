#ifndef SSH_CLIENT
#define SSH_CLIENT

void sendUsername(char *);
void hash_pass(unsigned char *, unsigned char *);
void waitForServer();
void writeToFile(char *, char *);
void transferFile(char *);
void sendPassword();
void get_hidden_pass(char *);

#endif
